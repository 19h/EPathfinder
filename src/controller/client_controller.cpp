#include "controller/client_controller.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>

ClientController::ClientController(QObject* const parent)
    : QObject(parent)
{
    qRegisterMetaType<VehicleStatus>();
    readSetting();
}

ClientController::~ClientController()
{
    if (server_ != nullptr) {
        server_->close();
    }
}

void ClientController::Init()
{
    initServer();
}

quint16 ClientController::serverPort() const noexcept { return port_; }
void ClientController::setServerPort(const quint16 port) noexcept { port_ = port; }

void ClientController::initServer()
{
    if (server_ != nullptr) {
        return;
    }
    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection,
            this, &ClientController::newConnection);
    server_->listen(address_, port_);
}

void ClientController::newConnection()
{
    while (server_ != nullptr && server_->hasPendingConnections()) {
        QTcpSocket* const socket = server_->nextPendingConnection();
        buffers_.insert(socket, {});
        connect(socket, &QTcpSocket::readyRead,
                this, &ClientController::readClient);
        connect(socket, &QTcpSocket::disconnected,
                this, &ClientController::clientDisconnected);
    }
}

void ClientController::readClient()
{
    auto* const socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == nullptr) {
        return;
    }
    QByteArray& buffer = buffers_[socket];
    buffer.append(socket->readAll());
    parseBuffer(buffer, socket);
}

void ClientController::clientDisconnected()
{
    auto* const socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == nullptr) {
        return;
    }
    buffers_.remove(socket);
    for (auto it = cookieSenders_.begin(); it != cookieSenders_.end();) {
        if (it.value() == socket) {
            it = cookieSenders_.erase(it);
        } else {
            ++it;
        }
    }
    socket->deleteLater();
}

void ClientController::parseBuffer(QByteArray& data, QTcpSocket* const senderSocket)
{
    int closingBrace = data.indexOf('}');
    while (closingBrace > 0) {
        const int candidateSize = closingBrace + 1;
        QJsonParseError error;
        const QJsonDocument document =
            QJsonDocument::fromJson(data.left(candidateSize), &error);
        if (error.error == QJsonParseError::NoError) {
            clientMessageHandler(document, senderSocket);
            data.remove(0, candidateSize);
            closingBrace = data.indexOf('}');
        } else {
            closingBrace = data.indexOf('}', candidateSize);
        }
    }
}

void ClientController::parseBuffer(QByteArray& data)
{
    parseBuffer(data, nullptr);
}

void ClientController::tabletLinkDataReceived(const QByteArray& message)
{
    tabletBuffer_.append(message);
    parseBuffer(tabletBuffer_);
}

void ClientController::clientMessageHandler(const QJsonDocument& message,
                                             QTcpSocket* const senderSocket)
{
    const QJsonObject object = message.object();
    const QString cookie = object.value(QStringLiteral("cookie")).toString();
    if (cookie.isEmpty()) {
        sendError(senderSocket, {}, 2, QStringLiteral("cookie is empty"));
        return;
    }
    cookieSenders_.insert(cookie, senderSocket);
    const int messageType = object.value(QStringLiteral("messageType")).toInt();
    dispatch(message, messageType);
}

void ClientController::dispatch(const QJsonDocument& message,
                                const int messageType)
{
    switch (messageType) {
    case 0: emit setNodesMessage(message); return;
    case 1: emit launchMessage(message); return;
    case 2: emit getStatusMessage(message); return;
    case 3: emit getExtStatusMessage(message); return;
    case 4: emit getParamsMessage(message); return;
    case 5: emit unlaunchMessage(message); return;
    case 6: emit testControlMessage(message); return;
    case 7: emit getNodesMessage(message); return;
    case 8: emit setParamsMessage(message); return;
    case 9: emit getCameraScreenshotMessage(message); return;
    case 10: emit setCodeMessage(message); return;
    case 11: emit uploadSoftwareMessage(message); return;
    case 12: emit uploadVisionMessage(message); return;
    case 13: emit applySoftwareMessage(message); return;
    case 14: emit applyVisionMessage(message); return;
    case 15: emit setVisionEnabledMessage(message); return;
    case 17: emit rebootFC(message); return;
    case 18: emit startMagnitometerCalibration(message); return;
    case 19: emit acceptMagnitometerCalibration(message); return;
    case 20: emit cancelMagnitometerCalibration(message); return;
    case 23:
        pendingLogCookies_.insert(
            message.object().value(QStringLiteral("cookie")).toString());
        emit getLogMessage(message);
        return;
    case 24: emit startLevelCalibration(message); return;
    case 25: emit clientVersionMessage(message); return;
    case 26: emit setTelemetryParamsMessage(message); return;
    case 27: emit getMapBounds(message); return;
    case 28: emit getMapCRC(message); return;
    case 29: emit poweroff(message); return;
    case 30: emit getVehicleTypesMessage(message); return;
    case 31: emit getMavLogListMessage(message); return;
    case 32: emit getMavLogMessage(message); return;
    case 33: emit setZoomCameraParams(message); return;
    case 100: emit setLedState(VehicleLed::Type::Type0, VehicleLed::State::Off); return;
    case 101: emit setLedState(VehicleLed::Type::Type0, VehicleLed::State::On); return;
    case 102: emit setLedState(VehicleLed::Type::Type0,
                               VehicleLed::State::BlinkInitiallyOn); return;
    case 110: emit setLedState(VehicleLed::Type::Type1, VehicleLed::State::Off); return;
    case 111: emit setLedState(VehicleLed::Type::Type1, VehicleLed::State::On); return;
    case 112: emit setLedState(VehicleLed::Type::Type1,
                               VehicleLed::State::BlinkInitiallyOn); return;
    case 333: emit targetMessage(message); return;
    case 500: emit setControlParams(message); return;
    case 501: emit takeoff(); return;
    case 502:
        emit arm(message.object().value(QStringLiteral("arm")).toBool());
        return;
    case 503: emit setMode(10); return;
    case 504: emit setMode(5); return;
    case 555: emit set5GMessage(message); return;
    case 601:
    case 602: emit homingMessage(message); return;
    case 777: emit resetPlanHandlerMessage(message); return;
    case 800: {
        const int exampleId =
            message.object().value(QStringLiteral("exampleId")).toInt();
        if (exampleId == -1) {
            emit stopCurrentTarget();
        } else {
            emit startTestTargetExample();
        }
        return;
    }
    case 999: emit stopProcMessage(); return;
    default:
        break;
    }

    const QString cookie =
        message.object().value(QStringLiteral("cookie")).toString();
    sendError(cookieSenders_.value(cookie), cookie, 3,
              QStringLiteral("unknown message type %1").arg(messageType));
}

void ClientController::sendError(QTcpSocket* const socket,
                                 const QString& cookie, const int result,
                                 const QString& text)
{
    QJsonObject object;
    if (!cookie.isEmpty()) {
        object.insert(QStringLiteral("cookie"), cookie);
    }
    object.insert(QStringLiteral("result"), result);
    object.insert(QStringLiteral("text"), text);
    const QByteArray bytes = QJsonDocument(object).toJson();
    if (socket != nullptr) {
        sendToTCP(socket, bytes);
    }
}

void ClientController::sendMessage(const QJsonDocument& message)
{
    const QString cookie =
        message.object().value(QStringLiteral("cookie")).toString();
    const QByteArray bytes = message.toJson();
    if (!cookie.isEmpty()) {
        QTcpSocket* const socket = cookieSenders_.value(cookie);
        if (socket != nullptr) {
            sendToTCP(socket, bytes);
            cookieSenders_.remove(cookie);
        }
        pendingLogCookies_.remove(cookie);
        return;
    }

    for (QTcpSocket* const socket : buffers_.keys()) {
        sendToTCP(socket, bytes);
    }
}

void ClientController::sendToTCP(QTcpSocket* const socket,
                                 const QByteArray& message)
{
    if (socket == nullptr || socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    socket->write(message);
    socket->flush();
}

void ClientController::updateVehicleStatus(const VehicleStatus& status)
{
    status_ = status;
    sendVehicleStatus();
}

void ClientController::sendVehicleStatus() {}
void ClientController::ckeckSerialLink() {}

void ClientController::readSetting()
{
    const QString prefix = QStringLiteral("settings=");
    const QStringList arguments = QCoreApplication::arguments();
    for (const QString& argument : arguments) {
        if (argument.startsWith(prefix)) {
            settingsPath_ = argument.mid(prefix.size());
        }
    }

    QFile file(settingsPath_);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }
    const int port = document.object().value(QStringLiteral("tcp_port")).toInt();
    if (port >= 1025 && port <= 49150) {
        port_ = static_cast<quint16>(port);
    }
}
