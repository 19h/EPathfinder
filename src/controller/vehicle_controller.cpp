#include "controller/vehicle_controller.hpp"

#include "pathfinder/pathfinder.hpp"
#include "scout/escout.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>

VehicleController::VehicleController(QObject* const parent)
    : QObject(parent)
{
    qRegisterMetaType<VehicleStatus>();
}

void VehicleController::Init()
{
    if (initialized_) {
        return;
    }
    initialized_ = true;
    scout_ = new EScout(this);
    EPathfinderParams parameters;
    if (params_.contains(QStringLiteral("vehicleVersion"))) {
        parameters.setVehicleVersion(
            params_.value(QStringLiteral("vehicleVersion")).toInt());
    }
    pathfinder_ = new EPathfinder(scout_, parameters);
    connect(pathfinder_, &EPathfinder::testEnded,
            this, &VehicleController::testEnded);
    connect(pathfinder_, &EPathfinder::landed,
            this, &VehicleController::landed);
    connect(pathfinder_, &EPathfinder::takeoffed,
            this, &VehicleController::takeoffed);
    connect(pathfinder_, &EPathfinder::mavlinkConnected,
            this, &VehicleController::mavlinkConnected);
    connect(pathfinder_, &EPathfinder::mavlinkDisconnected,
            this, &VehicleController::mavlinkDisconnected);
    connect(pathfinder_, &EPathfinder::elinkConnected,
            this, &VehicleController::elinkConnected);
    connect(pathfinder_, &EPathfinder::elinkDisconnected,
            this, &VehicleController::elinkDisconnected);
    connect(pathfinder_, &EPathfinder::flushLog,
            this, &VehicleController::flushLog);
    generateUniqueId();
    emit updateVehicleStatus(status_);
}

EScout* VehicleController::scout() const noexcept { return scout_; }
EPathfinder* VehicleController::pathfinder() const noexcept { return pathfinder_; }
VehicleStatus VehicleController::status() const noexcept { return status_; }

QJsonObject VehicleController::payload(const QJsonDocument& message)
{
    const QJsonObject object = message.object();
    const QJsonObject nested = object.value(QStringLiteral("params")).toObject();
    return nested.isEmpty() ? object : nested;
}

void VehicleController::send(const QJsonDocument& request, const int result,
                             const QString& text, const QJsonObject& values)
{
    QJsonObject answer = values;
    const QString cookie =
        request.object().value(QStringLiteral("cookie")).toString();
    if (!cookie.isEmpty()) {
        answer.insert(QStringLiteral("cookie"), cookie);
    }
    answer.insert(QStringLiteral("result"), result);
    if (!text.isEmpty()) {
        answer.insert(QStringLiteral("text"), text);
    }
    emit sendAnswer(QJsonDocument(answer));
}

void VehicleController::getStatusMessage(const QJsonDocument& message)
{
    QJsonObject values;
    values.insert(QStringLiteral("camera"), status_.camera);
    values.insert(QStringLiteral("satellites"), status_.satellites);
    values.insert(QStringLiteral("plan"), status_.plan);
    values.insert(QStringLiteral("armed"), status_.armed);
    values.insert(QStringLiteral("launched"), status_.launched);
    values.insert(QStringLiteral("vnav"), status_.vnav);
    values.insert(QStringLiteral("mavlink"), mavlinkConnected_);
    values.insert(QStringLiteral("elink"), elinkConnected_);
    values.insert(QStringLiteral("battery"), static_cast<int>(batteryVoltage_));
    values.insert(QStringLiteral("mode"), currentMode_);
    values.insert(QStringLiteral("air_speed"),
                  pathfinder_ != nullptr ? pathfinder_->airSpeedLastValue() : 0.0F);
    send(message, 0, {}, values);
}

void VehicleController::getExtStatusMessage(const QJsonDocument& message)
{
    QJsonObject values;
    values.insert(QStringLiteral("initialized"), initialized_);
    values.insert(QStringLiteral("valid_ardupilot"), validArdupilotVersion_);
    values.insert(QStringLiteral("compass"), compassEnabled_);
    values.insert(QStringLiteral("5g"), fiveGEnabled_);
    values.insert(QStringLiteral("vehicle_role"),
                  pathfinder_ != nullptr ? pathfinder_->vehicleRole() : 0);
    values.insert(QStringLiteral("unique_id"), uniqueId_);
    send(message, 0, {}, values);
}

void VehicleController::getParamsMessage(const QJsonDocument& message)
{
    send(message, 0, {}, QJsonObject{{QStringLiteral("params"), params_}});
}

void VehicleController::setParamsMessage(const QJsonDocument& message)
{
    const QJsonObject values = payload(message);
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (it.key() != QStringLiteral("cookie")
            && it.key() != QStringLiteral("messageType")) {
            params_.insert(it.key(), it.value());
        }
    }
    updatePathfinderParams();
    send(message, 0);
}

void VehicleController::setNodesMessage(const QJsonDocument& message)
{
    nodes_ = payload(message);
    nodes_.remove(QStringLiteral("cookie"));
    nodes_.remove(QStringLiteral("messageType"));
    send(message, 0);
}

void VehicleController::getNodesMessage(const QJsonDocument& message)
{
    send(message, 0, {}, QJsonObject{{QStringLiteral("nodes"), nodes_}});
}

void VehicleController::getVehicleTypesMessage(const QJsonDocument& message)
{
    QJsonArray types;
    if (pathfinder_ != nullptr) {
        for (const int type : pathfinder_->vehicleTypes()) {
            types.append(type);
        }
    }
    send(message, 0, {}, QJsonObject{{QStringLiteral("vehicleTypes"), types}});
}

void VehicleController::launchMessage(const QJsonDocument& message)
{
    status_.launched = true;
    if (pathfinder_ != nullptr) {
        pathfinder_->setLaunchReady(true);
        pathfinder_->setCurrentMode(PathfinderMode::Launch);
    }
    emit updateVehicleStatus(status_);
    send(message, 0);
}

void VehicleController::unlaunchMessage(const QJsonDocument& message)
{
    status_.launched = false;
    if (pathfinder_ != nullptr) {
        pathfinder_->setLaunchReady(false);
        pathfinder_->setCurrentMode(PathfinderMode::Disabled);
    }
    emit updateVehicleStatus(status_);
    send(message, 0);
}

void VehicleController::set5GMessage(const QJsonDocument& message)
{
    fiveGEnabled_ = payload(message).value(QStringLiteral("enabled")).toBool();
    send(message, 0);
}

void VehicleController::resetPlanHandlerMessage(const QJsonDocument& message)
{
    status_.plan = 0;
    emit updateVehicleStatus(status_);
    send(message, 0);
}

void VehicleController::testControlMessage(const QJsonDocument& message)
{
    if (pathfinder_ != nullptr) {
        const QJsonObject object = payload(message);
        pathfinder_->setTestControlParams(
            object.value(QStringLiteral("roll")).toInt(),
            object.value(QStringLiteral("pitch")).toInt(),
            object.value(QStringLiteral("throttle")).toInt(),
            object.value(QStringLiteral("throttle2")).toInt(),
            object.value(QStringLiteral("cameraRoll")).toInt(),
            object.value(QStringLiteral("cameraPitch")).toInt(),
            object.value(QStringLiteral("cameraYaw")).toInt(),
            object.value(QStringLiteral("cameraZoom")).toInt(),
            object.value(QStringLiteral("cameraCenter")).toBool());
    }
    send(message, 0);
}

void VehicleController::testEnded() {}
void VehicleController::landed() { status_.launched = false; emit updateVehicleStatus(status_); }

void VehicleController::setControlParams(const QJsonDocument& message)
{
    if (pathfinder_ != nullptr) {
        const QJsonObject object = payload(message);
        emit pathfinder_->setControlParams(
            object.value(QStringLiteral("roll")).toInt(),
            object.value(QStringLiteral("yaw")).toInt(),
            object.value(QStringLiteral("pitch")).toInt(),
            object.value(QStringLiteral("throttle")).toInt());
    }
    send(message, 0);
}

void VehicleController::getCameraScreenshotMessage(const QJsonDocument& message)
{
    send(message, 0, {}, QJsonObject{{QStringLiteral("data"), QString()}});
}

void VehicleController::setCodeMessage(const QJsonDocument& message)
{
    code_ = payload(message).value(QStringLiteral("code")).toString();
    send(message, 0);
}

void VehicleController::uploadSoftwareMessage(const QJsonDocument& message) { send(message, 0); }
void VehicleController::uploadVisionMessage(const QJsonDocument& message) { send(message, 0); }
void VehicleController::applySoftwareMessage(const QJsonDocument& message) { send(message, 0); }
void VehicleController::applyVisionMessage(const QJsonDocument& message) { send(message, 0); }

void VehicleController::setVisionEnabledMessage(const QJsonDocument& message)
{
    if (pathfinder_ != nullptr) {
        const bool enabled = payload(message).value(QStringLiteral("enabled")).toBool();
        pathfinder_->objectDetectorVisionModuleEnabled(enabled);
        pathfinder_->pathfinderVisionModuleEnabled(enabled);
        pathfinder_->updateVisionModuleActivities();
    }
    send(message, 0);
}

void VehicleController::rebootFC(const QJsonDocument& message) { send(message, 0); }
void VehicleController::startMagnitometerCalibration(const QJsonDocument& message) { send(message, 0); }
void VehicleController::acceptMagnitometerCalibration(const QJsonDocument& message) { send(message, 0); }
void VehicleController::cancelMagnitometerCalibration(const QJsonDocument& message) { send(message, 0); }
void VehicleController::startLevelCalibration(const QJsonDocument& message) { send(message, 0); }

void VehicleController::clientVersionMessage(const QJsonDocument& message)
{
    clientVersion_ = payload(message).value(QStringLiteral("version")).toString();
    send(message, 0);
}

void VehicleController::setTelemetryParamsMessage(const QJsonDocument& message)
{
    const QJsonObject values = payload(message);
    params_.insert(QStringLiteral("telemetry"), values);
    send(message, 0);
}

void VehicleController::setZoomCameraParams(const QJsonDocument& message)
{
    if (pathfinder_ != nullptr) {
        pathfinder_->setZoom(
            static_cast<float>(payload(message).value(QStringLiteral("zoom")).toDouble()));
    }
    send(message, 0);
}

void VehicleController::getMapBounds(const QJsonDocument& message)
{
    send(message, 0, {}, QJsonObject{{QStringLiteral("bounds"), QJsonArray()}});
}

void VehicleController::getMapCRC(const QJsonDocument& message)
{
    send(message, 0, {}, QJsonObject{{QStringLiteral("crc"), QString()}});
}

void VehicleController::poweroff(const QJsonDocument& message) { send(message, 0); }
void VehicleController::takeoff() { if (pathfinder_ != nullptr) pathfinder_->launchAcceleration(); }

void VehicleController::arm(const bool value)
{
    if (status_.armed == value) {
        return;
    }
    status_.armed = value;
    emit updateVehicleStatus(status_);
    if (value) {
        emit armed();
    } else {
        emit disarmed();
    }
}

void VehicleController::setMode(const int mode)
{
    currentMode_ = mode;
    if (pathfinder_ != nullptr) {
        pathfinder_->setCurrentMode(static_cast<PathfinderMode>(mode));
    }
}

void VehicleController::setLedState(const VehicleLed::Type type,
                                    const VehicleLed::State state)
{
    Q_UNUSED(type)
    Q_UNUSED(state)
}

void VehicleController::setIsValidArdupilotVersion() { validArdupilotVersion_ = true; }
void VehicleController::readPlaneId() {}
void VehicleController::planWrited() { status_.plan = 1; emit updateVehicleStatus(status_); }
void VehicleController::vnavCheckPlanResult(const bool validVNav, const bool validVNav2) { status_.vnav = validVNav || validVNav2; emit updateVehicleStatus(status_); }
void VehicleController::satelliteCountChanged() { emit updateVehicleStatus(status_); }
void VehicleController::mavlinkConnected() { mavlinkConnected_ = true; status_.connected = true; emit updateVehicleStatus(status_); }
void VehicleController::mavlinkDisconnected() { mavlinkConnected_ = false; status_.connected = false; emit updateVehicleStatus(status_); }
void VehicleController::reconnectMavlink() { mavlinkConnected_ = false; }
void VehicleController::elinkConnected() { elinkConnected_ = true; emit updateVehicleStatus(status_); }
void VehicleController::elinkDisconnected() { elinkConnected_ = false; emit updateVehicleStatus(status_); }
void VehicleController::updateGreenLed() {}
void VehicleController::takeoffed() { status_.launched = true; emit updateVehicleStatus(status_); }
void VehicleController::startThunderGaze() {}
void VehicleController::elinkButtonStateChanged(const bool pressed) { Q_UNUSED(pressed) }
void VehicleController::disconnectArmTest() {}
void VehicleController::checkArmReady() {}
void VehicleController::checkMavlink() {}
void VehicleController::checkElink() {}
void VehicleController::checkCameraDev() {}
void VehicleController::checkThunderGaze() {}
void VehicleController::readPTZServoValues() {}
void VehicleController::armedSysStatus() { arm(true); }
void VehicleController::disarmedSysStatus() { arm(false); }
void VehicleController::compassStatusChanged(const bool enabled) { compassEnabled_ = enabled; }
void VehicleController::vnavStatusChanged(const bool active) { status_.vnav = active; emit updateVehicleStatus(status_); }
void VehicleController::mavlinkParamValue(const QString& name, const float value) { params_.insert(name, value); }

void VehicleController::updatePathfinderParams()
{
    if (pathfinder_ != nullptr && params_.contains(QStringLiteral("vehicleVersion"))) {
        pathfinder_->setVehicleVersion(
            params_.value(QStringLiteral("vehicleVersion")).toInt());
    }
}

void VehicleController::updateParamsFile() {}
void VehicleController::screenshotStart() {}
void VehicleController::screenshotStartThunderGaze() {}
void VehicleController::screenshotFinished(const int exitCode, const QProcess::ExitStatus exitStatus) { Q_UNUSED(exitCode) Q_UNUSED(exitStatus) }
void VehicleController::logListEvent(const QList<LogItem>& items) { Q_UNUSED(items) }
void VehicleController::getLogListError(const int code) { Q_UNUSED(code) }
void VehicleController::logEvent(const QByteArray& data) { Q_UNUSED(data) }
void VehicleController::getLogError(const int code) { Q_UNUSED(code) }
void VehicleController::cameraParamsChanged(const CameraParams& params) { if (pathfinder_ != nullptr) pathfinder_->cameraParamsChanged(params); }
void VehicleController::batteryVoltageChanged(const unsigned int voltage) { batteryVoltage_ = voltage; if (pathfinder_ != nullptr) pathfinder_->setBatteryVoltage(static_cast<int>(voltage)); }
void VehicleController::testLed() {}
void VehicleController::dtpDebugInfo(const QString& text) { Q_UNUSED(text) }
void VehicleController::removeOtherVersions() {}
void VehicleController::rebootSystem() {}
void VehicleController::disableNTPService() {}
void VehicleController::startMagCalAck(const unsigned char result) { Q_UNUSED(result) }
void VehicleController::acceptMagCalAck(const unsigned char result) { Q_UNUSED(result) }
void VehicleController::cancelMagCalAck(const unsigned char result) { Q_UNUSED(result) }
void VehicleController::magCalCompletionPercentage(const unsigned char value) { Q_UNUSED(value) }
void VehicleController::magCalStatus(const unsigned char value) { Q_UNUSED(value) }
void VehicleController::levelCalAck(const unsigned char result) { Q_UNUSED(result) }
void VehicleController::mgCfgStatus(const unsigned char status) { Q_UNUSED(status) }
void VehicleController::mgReseted(const unsigned char resultCode) { Q_UNUSED(resultCode) }
void VehicleController::initWithoutLaunch() { Init(); }
void VehicleController::updateVehicleTypeConnects() {}

void VehicleController::generateUniqueId()
{
    const QByteArray seed = QCoreApplication::applicationFilePath().toUtf8();
    uniqueId_ = QString::fromLatin1(
        QCryptographicHash::hash(seed, QCryptographicHash::Sha256).toHex());
}

void VehicleController::vnavMapCRCProcessFinished(const int exitCode,
                                                   const QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
}

void VehicleController::vnavMapCRCProcessReadOutput() {}
