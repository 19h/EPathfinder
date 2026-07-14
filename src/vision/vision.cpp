#include "vision/vision.hpp"

#include "camera/camera_t205.hpp"
#include "camera/camera_zr10.hpp"

#include <QDateTime>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QUdpSocket>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kCalibrationColumns = 9;
constexpr int kCalibrationRows = 9;

} // namespace

EVision::EVision(const int port,
                 const float thunderGazeVersion,
                 QObject* const parent)
    : QObject(parent)
    , port_(port)
    , thunderGazeVersion_(thunderGazeVersion)
{
    initConnect();
    thunderGaze_ =
        ThunderGazeFactory::create(thunderGazeVersion_, this, socket_);
    receiveBuffer_.resize(thunderGaze_->messageSize());

    connect(thunderGaze_, &ThunderGaze::emptyDTP,
            this, &EVision::emptyDTP);
    connect(thunderGaze_, &ThunderGaze::setTargets,
            this, &EVision::setTargets);
    connect(thunderGaze_, &ThunderGaze::preprocessingScaleChanged,
            this, &EVision::preprocessingScaleChanged);
    connect(thunderGaze_, &ThunderGaze::dtpDebugInfo,
            this, &EVision::dtpDebugInfo);
    connect(thunderGaze_, &ThunderGaze::roadScreenParts,
            this, &EVision::roadScreenParts);
    connect(thunderGaze_, &ThunderGaze::roadDetected,
            this, &EVision::roadDetected);
    connect(thunderGaze_, &ThunderGaze::rtspEnabled,
            this, &EVision::rtspEnabled);
    connect(thunderGaze_, &ThunderGaze::rtspDisabled,
            this, &EVision::rtspDisabled);
    connect(thunderGaze_, &ThunderGaze::objectDetectorEnabled,
            this, &EVision::objectDetectorEnabled);
    connect(thunderGaze_, &ThunderGaze::pathfinderEnabled,
            this, &EVision::pathfinderEnabled);
    connect(thunderGaze_, &ThunderGaze::videoSaverEnabled,
            this, &EVision::videoSaverEnabled);
    connect(thunderGaze_, &ThunderGaze::frameBlur,
            this, &EVision::frameBlur);
    loadCalibration();
}

bool EVision::isCalibrated() const { return calibrated_; }

bool EVision::isConnected() const
{
    return socket_ != nullptr && socket_->state() == QAbstractSocket::BoundState;
}

void EVision::setCameraOffset(const float horizontalDegrees,
                              const float verticalDegrees)
{
    cameraOffsetX_ = horizontalDegrees;
    cameraOffsetY_ = verticalDegrees;
}

void EVision::setPTZCalibration(const float distanceMetres,
                                const float horizontalOffsetMillimetres,
                                const float verticalOffsetMillimetres)
{
    if (distanceMetres == 0.0F) {
        ptzCalibrationX_ = 0.0F;
        ptzCalibrationY_ = 0.0F;
    } else {
        ptzCalibrationX_ = static_cast<float>(
            std::atan(horizontalOffsetMillimetres
                      / (distanceMetres * 1000.0F))
            * 57.29577951308232);
        ptzCalibrationY_ = static_cast<float>(
            std::atan(verticalOffsetMillimetres
                      / (distanceMetres * 1000.0F))
            * 57.29577951308232);
    }
    thunderGaze_->setPTZOffset(ptzCalibrationX_, ptzCalibrationY_);
}

float EVision::cameraCalibrationWidthAngle() const
{
    return camera_ == nullptr ? 0.0F : camera_->calibrationWidthAngle();
}

float EVision::cameraCalibrationHeightAngle() const
{
    return camera_ == nullptr ? 0.0F : camera_->calibrationHeightAngle();
}

void EVision::setUSBCamera(const QString& device)
{
    camera_ = new CameraT205(device, this);
    connectCameraSignals();
    thunderGaze_->setCamera(camera_);
}

void EVision::setIPCamera(const QString& address,
                          const int localPort,
                          const int remotePort,
                          const bool externalGimbalControl)
{
    camera_ = new CameraZR10(address,
                             localPort,
                             remotePort,
                             externalGimbalControl,
                             this);
    connectCameraSignals();
    thunderGaze_->setCamera(camera_);
}

bool EVision::cameraIsEnabled() const
{
    return camera_ != nullptr && camera_->deviceIsEnabled();
}

void EVision::connectToCamera()
{
    if (camera_ != nullptr && cameraIsEnabled()) {
        camera_->connect();
    }
}

void EVision::setGimbalStabRoll(const bool enabled)
{
    if (camera_ != nullptr) {
        camera_->setGimbalStabRoll(enabled);
    }
}

void EVision::pointAngles(const int pixelX,
                          const int pixelY,
                          float& horizontalDegrees,
                          float& verticalDegrees) const
{
    horizontalDegrees = cameraOffsetX_;
    verticalDegrees = cameraOffsetY_;
    if (!calibrated_ || calibrationPoints_.size()
            != kCalibrationColumns * kCalibrationRows) {
        return;
    }
    const float columnPosition = static_cast<float>(pixelX)
        / static_cast<float>(calibrationStepX_);
    const float rowPosition = static_cast<float>(pixelY)
        / static_cast<float>(calibrationStepY_);
    const int column = std::clamp(static_cast<int>(std::floor(columnPosition)),
                                  0,
                                  kCalibrationColumns - 2);
    const int row = std::clamp(static_cast<int>(std::floor(rowPosition)),
                               0,
                               kCalibrationRows - 2);
    const float xFraction = std::clamp(columnPosition - column, 0.0F, 1.0F);
    const float yFraction = std::clamp(rowPosition - row, 0.0F, 1.0F);
    const auto& topLeft = calibrationPoints_.at(row * 9 + column);
    const auto& topRight = calibrationPoints_.at(row * 9 + column + 1);
    const auto& bottomLeft = calibrationPoints_.at((row + 1) * 9 + column);
    const auto& bottomRight =
        calibrationPoints_.at((row + 1) * 9 + column + 1);
    const float topHorizontal = topLeft.horizontalDegrees
        + xFraction * (topRight.horizontalDegrees
                       - topLeft.horizontalDegrees);
    const float bottomHorizontal = bottomLeft.horizontalDegrees
        + xFraction * (bottomRight.horizontalDegrees
                       - bottomLeft.horizontalDegrees);
    const float topVertical = topLeft.verticalDegrees
        + xFraction * (topRight.verticalDegrees - topLeft.verticalDegrees);
    const float bottomVertical = bottomLeft.verticalDegrees
        + xFraction * (bottomRight.verticalDegrees
                       - bottomLeft.verticalDegrees);
    horizontalDegrees +=
        topHorizontal + yFraction * (bottomHorizontal - topHorizontal);
    verticalDegrees += topVertical + yFraction * (bottomVertical - topVertical);
}

void EVision::setDebugDTPEnabled(const bool enabled)
{
    thunderGaze_->setDebugDTPEnabled(enabled);
}

void EVision::setDetTypes(const QVector<unsigned short>& types)
{
    for (int index = 0; index < 4; ++index) {
        thunderGaze_->setDetTypeEnabled(index, false);
    }
    bool selected = false;
    for (const unsigned short type : types) {
        if (type < 4) {
            thunderGaze_->setDetTypeEnabled(type, true);
            selected = true;
        }
    }
    if (!selected) {
        for (int index = 0; index < 4; ++index) {
            thunderGaze_->setDetTypeEnabled(index, true);
        }
    }
}

void EVision::setRoadsEnabled(const bool enabled)
{
    thunderGaze_->setRoadsEnabled(enabled);
}

void EVision::setRoadReduceBorderEnabled(const bool enabled)
{
    thunderGaze_->setReduceBorderEnabled(enabled);
}

void EVision::setIsRailway(const unsigned char value)
{
    thunderGaze_->setIsRailway(value);
}

void EVision::setTelemetryParams(const QJsonDocument& params)
{
    thunderGaze_->setTelemetryParams(params);
}

void EVision::setGimbal(const Gimbal& gimbal)
{
    if (camera_ != nullptr) {
        camera_->setGimbal(gimbal);
    }
}

void EVision::gimbalValues(const float roll,
                           const float pitch,
                           const float yaw,
                           const bool isCenter,
                           const qlonglong time)
{
    if (camera_ != nullptr) {
        camera_->gimbalValues(roll, pitch, yaw, isCenter, time);
    }
}

void EVision::gimbalRollMagValue(const qlonglong time,
                                 const unsigned char componentId,
                                 const float roll,
                                 const float pitch,
                                 const float yaw,
                                 const float magRoll)
{
    if (camera_ != nullptr) {
        camera_->gimbalRollMagValue(
            time, componentId, roll, pitch, yaw, magRoll);
    }
}

void EVision::gimbalPitchMagValue(const qlonglong time,
                                  const unsigned char componentId,
                                  const float roll,
                                  const float pitch,
                                  const float yaw,
                                  const float magPitch)
{
    if (camera_ != nullptr) {
        camera_->gimbalPitchMagValue(
            time, componentId, roll, pitch, yaw, magPitch);
    }
}

void EVision::gimbalYawMagValue(const qlonglong time,
                                const unsigned char componentId,
                                const float roll,
                                const float pitch,
                                const float yaw,
                                const float magYaw)
{
    if (camera_ != nullptr) {
        camera_->gimbalYawMagValue(
            time, componentId, roll, pitch, yaw, magYaw);
    }
}

void EVision::gimbalMagValues(const qlonglong time,
                              const unsigned char componentId,
                              const float roll,
                              const float pitch,
                              const float yaw,
                              const float magRoll,
                              const float magPitch,
                              const float magYaw)
{
    if (camera_ != nullptr) {
        camera_->gimbalMagValues(time,
                                 componentId,
                                 roll,
                                 pitch,
                                 yaw,
                                 magRoll,
                                 magPitch,
                                 magYaw);
    }
}

void EVision::setGimbalAHRS(qlonglong time,
                            int,
                            int,
                            unsigned int,
                            const float gimbalRoll,
                            const float gimbalPitch,
                            const float gimbalYaw,
                            unsigned char)
{
    if (camera_ != nullptr) {
        camera_->setGimbalAHRS(time, gimbalRoll, gimbalPitch, gimbalYaw);
    }
}

void EVision::setZoom(const float percent)
{
    if (camera_ != nullptr) {
        camera_->setZoom(percent);
    }
}

void EVision::setFocus(const float distance,
                       const float zoomPercent,
                       const int timeoutMSec)
{
    if (camera_ != nullptr) {
        camera_->setFocus(distance, zoomPercent, timeoutMSec);
    }
}

void EVision::setPreprocessingScale(const float value)
{
    thunderGaze_->setPreprocessingScale(value);
}

void EVision::setRTSPEnabled(const bool enabled)
{
    thunderGaze_->setRTSPEnabled(enabled);
}

void EVision::enableRTSP() { setRTSPEnabled(true); }
void EVision::disableRTSP() { setRTSPEnabled(false); }

void EVision::setRoadSimpleMode(const bool enabled)
{
    thunderGaze_->setRoadSimpleMode(enabled);
}

void EVision::setFrameText(const QStringList& lines)
{
    thunderGaze_->setFrameText(lines);
}

void EVision::setObjectDetectorEnabled(const bool enabled)
{
    thunderGaze_->setObjectDetectorEnabled(enabled);
}

void EVision::setPathfinderEnabled(const bool enabled)
{
    thunderGaze_->setPathfinderEnabled(enabled);
}

void EVision::setVideoSaverEnabled(const bool enabled)
{
    thunderGaze_->setVideoSaverEnabled(enabled);
}

void EVision::sendImageTarget(const qlonglong time,
                              const short pixelX,
                              const short pixelY,
                              const unsigned char type)
{
    if (camera_ == nullptr) {
        return;
    }
    Target target;
    target.pixelX = pixelX;
    target.pixelY = pixelY;
    target.secondPixelX = pixelX;
    target.thirdPixelX = pixelX;
    target.auxiliaryPixelY = pixelY;
    target.lowerPixelY = pixelY;
    target.confidence = 1.0F;
    target.type = type;
    target.time = time - 40;
    const CameraParams params = camera_->currentParams();
    setTargetAngles(target, params);
    emit imageTargetDetected(params, target);
}

void EVision::setTestTarget(const int pixelX, const int pixelY)
{
    Target target;
    target.pixelX = pixelX;
    target.pixelY = pixelY;
    target.confidence = 1.0F;
    target.type = 3;
    target.time = QDateTime::currentMSecsSinceEpoch() - 133;
    QList<Target> targets{target};
    setTargets(targets);
}

Camera* EVision::camera() const { return camera_; }
ThunderGaze* EVision::thunderGaze() const { return thunderGaze_; }

void EVision::readPendingDatagrams()
{
    while (socket_ != nullptr && socket_->hasPendingDatagrams()) {
        QHostAddress sender;
        quint16 senderPort = 0;
        receiveBuffer_.resize(static_cast<qsizetype>(
            socket_->pendingDatagramSize()));
        const qint64 size = socket_->readDatagram(receiveBuffer_.data(),
                                                  receiveBuffer_.size(),
                                                  &sender,
                                                  &senderPort);
        if (size > 0) {
            receiveBuffer_.resize(size);
            thunderGaze_->parseMessage(receiveBuffer_,
                                       QDateTime::currentMSecsSinceEpoch(),
                                       sender,
                                       senderPort);
        }
    }
}

void EVision::upConnect()
{
    if (socket_ != nullptr && socket_->state() != QAbstractSocket::BoundState) {
        socket_->bind(QHostAddress::AnyIPv4,
                      static_cast<quint16>(port_),
                      QUdpSocket::ShareAddress
                          | QUdpSocket::ReuseAddressHint);
    }
}

void EVision::setTargets(QList<Target>& targets)
{
    CameraParams params;
    if (camera_ != nullptr) {
        const qlonglong time = targets.isEmpty() ? 0 : targets.first().time;
        params = camera_->currentParams();
        params.zoom = camera_->zoom(time);
        params.gimbal = camera_->gimbal(time);
        for (Target& target : targets) {
            setTargetAngles(target, params);
        }
    } else {
        for (Target& target : targets) {
            pointAngles(target.pixelX,
                        target.pixelY,
                        target.angleX,
                        target.angleY);
            pointAngles(target.secondPixelX,
                        target.lowerPixelY,
                        target.secondAngleX,
                        target.secondAngleY);
            pointAngles(target.thirdPixelX,
                        target.lowerPixelY,
                        target.thirdAngleX,
                        target.thirdAngleY);
        }
    }
    emit targetDetected(params, targets);
}

void EVision::frameBlur(const float value)
{
    if (camera_ != nullptr) {
        camera_->setFrameBlur(value);
    }
}

void EVision::initConnect()
{
    socket_ = new QUdpSocket(this);
    connect(socket_, &QUdpSocket::readyRead,
            this, &EVision::readPendingDatagrams);
    upConnect();
}

void EVision::loadCalibration()
{
    QFile file(QStringLiteral("vision_calibration.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    const int frameWidth = root.value(QStringLiteral("frame_width")).toInt();
    const int frameHeight = root.value(QStringLiteral("frame_height")).toInt();
    calibrationStepX_ = frameWidth / (kCalibrationColumns - 1);
    calibrationStepY_ = frameHeight / (kCalibrationRows - 1);
    calibrationPoints_.clear();
    const QJsonArray points = root.value(QStringLiteral("points")).toArray();
    calibrationPoints_.reserve(points.size());
    for (const QJsonValue& value : points) {
        const QJsonObject point = value.toObject();
        CalibrationPoint item;
        item.pixelX = point.value(QStringLiteral("p_x")).toDouble();
        item.pixelY = point.value(QStringLiteral("p_y")).toDouble();
        item.horizontalDegrees =
            static_cast<float>(point.value(QStringLiteral("yaw")).toDouble());
        item.verticalDegrees =
            static_cast<float>(point.value(QStringLiteral("pitch")).toDouble());
        item.valid = true;
        calibrationPoints_.append(item);
    }
    calibrated_ = calibrationPoints_.size()
        == kCalibrationColumns * kCalibrationRows;
    thunderGaze_->setFrameDelay(
        root.value(QStringLiteral("frame_delay")).toInt(156));
    thunderGaze_->clearIgnores();
    for (const QJsonValue& value :
         root.value(QStringLiteral("ignores")).toArray()) {
        const QJsonObject object = value.toObject();
        thunderGaze_->addIgnoreBound(
            IgnoreBound{object.value(QStringLiteral("lower_x")).toDouble(),
                        object.value(QStringLiteral("lower_y")).toDouble(),
                        object.value(QStringLiteral("upper_x")).toDouble(),
                        object.value(QStringLiteral("upper_y")).toDouble()});
    }
}

void EVision::connectCameraSignals()
{
    connect(camera_, &Camera::paramsChanged,
            this, &EVision::cameraParamsChanged);
    connect(camera_, &Camera::setPTZ,
            this, &EVision::setPTZ);
    connect(camera_, &Camera::ahrsByCamera,
            this, &EVision::ahrsByCamera);
}

void EVision::setTargetAngles(Target& target,
                              const CameraParams& params) const
{
    targetAnglesForPixel(target.pixelX,
                         target.pixelY,
                         params,
                         target.angleX,
                         target.angleY);
    targetAnglesForPixel(target.secondPixelX,
                         target.lowerPixelY,
                         params,
                         target.secondAngleX,
                         target.secondAngleY);
    targetAnglesForPixel(target.thirdPixelX,
                         target.lowerPixelY,
                         params,
                         target.thirdAngleX,
                         target.thirdAngleY);
}

void EVision::targetAnglesForPixel(const int pixelX,
                                   const int pixelY,
                                   const CameraParams& params,
                                   float& horizontalDegrees,
                                   float& verticalDegrees) const
{
    if (params.frameWidth == 0 || params.frameHeight == 0) {
        horizontalDegrees = ptzCalibrationX_;
        verticalDegrees = ptzCalibrationY_;
        return;
    }
    const float normalizedX = static_cast<float>(pixelX)
            / static_cast<float>(params.frameWidth)
        - 0.5F;
    const float normalizedY = static_cast<float>(pixelY)
            / static_cast<float>(params.frameHeight)
        - 0.5F;
    float horizontalFov = params.zoom.widthAngle;
    float verticalFov = params.zoom.heightAngle;
    const float horizontalCorrection =
        std::min(1.1F,
                 std::max(1.0F,
                          2.0F * std::abs(normalizedY)
                              * params.zoom.scaleX));
    const float verticalCorrection =
        std::min(1.1F,
                 std::max(1.0F,
                          2.0F * std::abs(normalizedX)
                              * params.zoom.scaleY));
    if (params.zoom.scaleX > 1.0F) {
        horizontalFov *= horizontalCorrection;
    }
    if (params.zoom.scaleY > 1.0F) {
        verticalFov *= verticalCorrection;
    }
    horizontalDegrees = ptzCalibrationX_ + normalizedX * horizontalFov;
    verticalDegrees = ptzCalibrationY_ - normalizedY * verticalFov;
}

