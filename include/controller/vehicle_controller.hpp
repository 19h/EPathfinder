#pragma once

#include "camera/camera_types.hpp"
#include "controller/controller_types.hpp"
#include "led/vehicle_led.hpp"
#include "mavlink/log_handler.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>

class EPathfinder;
class EScout;

class VehicleController final : public QObject {
    Q_OBJECT

public:
    explicit VehicleController(QObject* parent = nullptr);
    ~VehicleController() override = default;

    void Init();
    EScout* scout() const noexcept;
    EPathfinder* pathfinder() const noexcept;
    VehicleStatus status() const noexcept;

signals:
    void sendAnswer(const QJsonDocument& answer);
    void updateVehicleStatus(const VehicleStatus& status);
    void armed();
    void disarmed();
    void flushLog();

public slots:
    void getStatusMessage(const QJsonDocument& message);
    void getExtStatusMessage(const QJsonDocument& message);
    void getParamsMessage(const QJsonDocument& message);
    void setParamsMessage(const QJsonDocument& message);
    void setNodesMessage(const QJsonDocument& message);
    void getNodesMessage(const QJsonDocument& message);
    void getVehicleTypesMessage(const QJsonDocument& message);
    void launchMessage(const QJsonDocument& message);
    void unlaunchMessage(const QJsonDocument& message);
    void set5GMessage(const QJsonDocument& message);
    void resetPlanHandlerMessage(const QJsonDocument& message);
    void testControlMessage(const QJsonDocument& message);
    void testEnded();
    void landed();
    void setControlParams(const QJsonDocument& message);
    void getCameraScreenshotMessage(const QJsonDocument& message);
    void setCodeMessage(const QJsonDocument& message);
    void uploadSoftwareMessage(const QJsonDocument& message);
    void uploadVisionMessage(const QJsonDocument& message);
    void applySoftwareMessage(const QJsonDocument& message);
    void applyVisionMessage(const QJsonDocument& message);
    void setVisionEnabledMessage(const QJsonDocument& message);
    void rebootFC(const QJsonDocument& message);
    void startMagnitometerCalibration(const QJsonDocument& message);
    void acceptMagnitometerCalibration(const QJsonDocument& message);
    void cancelMagnitometerCalibration(const QJsonDocument& message);
    void startLevelCalibration(const QJsonDocument& message);
    void clientVersionMessage(const QJsonDocument& message);
    void setTelemetryParamsMessage(const QJsonDocument& message);
    void setZoomCameraParams(const QJsonDocument& message);
    void getMapBounds(const QJsonDocument& message);
    void getMapCRC(const QJsonDocument& message);
    void poweroff(const QJsonDocument& message);
    void takeoff();
    void arm(bool value);
    void setMode(int mode);
    void setLedState(VehicleLed::Type type, VehicleLed::State state);

private slots:
    void setIsValidArdupilotVersion();
    void readPlaneId();
    void planWrited();
    void vnavCheckPlanResult(bool validVNav, bool validVNav2);
    void satelliteCountChanged();
    void mavlinkConnected();
    void mavlinkDisconnected();
    void reconnectMavlink();
    void elinkConnected();
    void elinkDisconnected();
    void updateGreenLed();
    void takeoffed();
    void startThunderGaze();
    void elinkButtonStateChanged(bool pressed);
    void disconnectArmTest();
    void checkArmReady();
    void checkMavlink();
    void checkElink();
    void checkCameraDev();
    void checkThunderGaze();
    void readPTZServoValues();
    void armedSysStatus();
    void disarmedSysStatus();
    void compassStatusChanged(bool enabled);
    void vnavStatusChanged(bool active);
    void mavlinkParamValue(const QString& name, float value);
    void updatePathfinderParams();
    void updateParamsFile();
    void screenshotStart();
    void screenshotStartThunderGaze();
    void screenshotFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void logListEvent(const QList<LogItem>& items);
    void getLogListError(int code);
    void logEvent(const QByteArray& data);
    void getLogError(int code);
    void cameraParamsChanged(const CameraParams& params);
    void batteryVoltageChanged(unsigned int voltage);
    void testLed();
    void dtpDebugInfo(const QString& text);
    void removeOtherVersions();
    void rebootSystem();
    void disableNTPService();
    void startMagCalAck(unsigned char result);
    void acceptMagCalAck(unsigned char result);
    void cancelMagCalAck(unsigned char result);
    void magCalCompletionPercentage(unsigned char value);
    void magCalStatus(unsigned char value);
    void levelCalAck(unsigned char result);
    void mgCfgStatus(unsigned char status);
    void mgReseted(unsigned char resultCode);
    void initWithoutLaunch();
    void updateVehicleTypeConnects();
    void generateUniqueId();
    void vnavMapCRCProcessFinished(int exitCode,
                                   QProcess::ExitStatus exitStatus);
    void vnavMapCRCProcessReadOutput();

private:
    void send(const QJsonDocument& request, int result,
              const QString& text = {}, const QJsonObject& values = {});
    static QJsonObject payload(const QJsonDocument& message);

    EScout* scout_{};
    EPathfinder* pathfinder_{};
    VehicleStatus status_;
    QJsonObject params_;
    QJsonObject nodes_;
    QString code_;
    QString clientVersion_;
    QString uniqueId_;
    unsigned int batteryVoltage_{};
    int currentMode_{};
    bool initialized_{};
    bool mavlinkConnected_{};
    bool elinkConnected_{};
    bool compassEnabled_{};
    bool validArdupilotVersion_{};
    bool fiveGEnabled_{};
};
