#pragma once

#include "controller/controller_types.hpp"
#include "led/vehicle_led.hpp"

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QSet>

class QTcpServer;
class QTcpSocket;
struct ClientControllerTestProbe;

class ClientController final : public QObject {
    Q_OBJECT

public:
    explicit ClientController(QObject* parent = nullptr);
    ~ClientController() override;

    void Init();
    quint16 serverPort() const noexcept;
    void setServerPort(quint16 port) noexcept;

signals:
    void setNodesMessage(const QJsonDocument& message);
    void getNodesMessage(const QJsonDocument& message);
    void getStatusMessage(const QJsonDocument& message);
    void getExtStatusMessage(const QJsonDocument& message);
    void getParamsMessage(const QJsonDocument& message);
    void setParamsMessage(const QJsonDocument& message);
    void getVehicleTypesMessage(const QJsonDocument& message);
    void launchMessage(const QJsonDocument& message);
    void unlaunchMessage(const QJsonDocument& message);
    void set5GMessage(const QJsonDocument& message);
    void resetPlanHandlerMessage(const QJsonDocument& message);
    void startExampleControl(const QJsonDocument& message);
    void homingMessage(const QJsonDocument& message);
    void targetMessage(const QJsonDocument& message);
    void testControlMessage(const QJsonDocument& message);
    void getCameraScreenshotMessage(const QJsonDocument& message);
    void setCodeMessage(const QJsonDocument& message);
    void uploadSoftwareMessage(const QJsonDocument& message);
    void uploadVisionMessage(const QJsonDocument& message);
    void applySoftwareMessage(const QJsonDocument& message);
    void applyVisionMessage(const QJsonDocument& message);
    void setVisionEnabledMessage(const QJsonDocument& message);
    void getLogMessage(const QJsonDocument& message);
    void getMavLogListMessage(const QJsonDocument& message);
    void getMavLogMessage(const QJsonDocument& message);
    void setZoomCameraParams(const QJsonDocument& message);
    void rebootFC(const QJsonDocument& message);
    void startMagnitometerCalibration(const QJsonDocument& message);
    void acceptMagnitometerCalibration(const QJsonDocument& message);
    void cancelMagnitometerCalibration(const QJsonDocument& message);
    void startLevelCalibration(const QJsonDocument& message);
    void clientVersionMessage(const QJsonDocument& message);
    void setTelemetryParamsMessage(const QJsonDocument& message);
    void getMapBounds(const QJsonDocument& message);
    void getMapCRC(const QJsonDocument& message);
    void poweroff(const QJsonDocument& message);
    void stopCurrentTarget();
    void startTestTargetExample();
    void setLedState(VehicleLed::Type type, VehicleLed::State state);
    void setControlParams(const QJsonDocument& message);
    void takeoff();
    void arm(bool value);
    void setMode(int mode);
    void setExampleMode(int exampleId);
    void stopProcMessage();

public slots:
    void sendMessage(const QJsonDocument& message);
    void updateVehicleStatus(const VehicleStatus& status);

private slots:
    void initServer();
    void newConnection();
    void readClient();
    void clientDisconnected();
    void sendVehicleStatus();
    void tabletLinkDataReceived(const QByteArray& message);
    void parseBuffer(QByteArray& data, QTcpSocket* sender);
    void parseBuffer(QByteArray& data);
    void ckeckSerialLink();

private:
    friend struct ClientControllerTestProbe;
    void clientMessageHandler(const QJsonDocument& message, QTcpSocket* sender);
    void dispatch(const QJsonDocument& message, int messageType);
    void sendError(QTcpSocket* sender, const QString& cookie,
                   int result, const QString& text);
    void sendToTCP(QTcpSocket* socket, const QByteArray& message);
    void readSetting();

    QTcpServer* server_{};
    QHostAddress address_{QHostAddress::AnyIPv4};
    quint16 port_{8052};
    QMap<QTcpSocket*, QByteArray> buffers_;
    QMap<QString, QTcpSocket*> cookieSenders_;
    QSet<QString> pendingLogCookies_;
    QByteArray tabletBuffer_;
    VehicleStatus status_;
    QString settingsPath_{QStringLiteral("settings.json")};
};
