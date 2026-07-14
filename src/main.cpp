#include "controller/client_controller.hpp"
#include "controller/vehicle_controller.hpp"
#include "logging/elogger.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);

    QString applicationName;
    QFile versionsFile(QStringLiteral("versions.json"));
    if (versionsFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument document =
            QJsonDocument::fromJson(versionsFile.readAll());
        const QString version =
            document.object().value(QStringLiteral("epathfinder")).toString();
        if (!version.isEmpty()) {
            applicationName = QStringLiteral("EPathfinder_%1").arg(version);
        }
    }
    if (applicationName.isEmpty()) {
        applicationName = QDir(QCoreApplication::applicationDirPath()).dirName();
    }
    QCoreApplication::setApplicationName(applicationName);

    ELogger* const logger = ELogger::getInstance();
    logger->init(applicationName);
    qInfo() << "___START___" << QCoreApplication::applicationFilePath();
    QFile releaseDate(QStringLiteral("release_date"));
    if (releaseDate.open(QIODevice::ReadOnly)) {
        qInfo() << "version:" << releaseDate.readAll();
    }

    VehicleController vehicle;
    QObject::connect(&vehicle, &VehicleController::armed,
                     logger, &ELogger::setEnabled);
    QObject::connect(&vehicle, &VehicleController::disarmed,
                     logger, &ELogger::setDisabled);
    QObject::connect(&vehicle, &VehicleController::flushLog,
                     logger, &ELogger::flushLog);
    vehicle.Init();

    ClientController client;
    QObject::connect(&client, &ClientController::getStatusMessage,
                     &vehicle, &VehicleController::getStatusMessage);
    QObject::connect(&client, &ClientController::getExtStatusMessage,
                     &vehicle, &VehicleController::getExtStatusMessage);
    QObject::connect(&client, &ClientController::getParamsMessage,
                     &vehicle, &VehicleController::getParamsMessage);
    QObject::connect(&client, &ClientController::setParamsMessage,
                     &vehicle, &VehicleController::setParamsMessage);
    QObject::connect(&client, &ClientController::setNodesMessage,
                     &vehicle, &VehicleController::setNodesMessage);
    QObject::connect(&client, &ClientController::getVehicleTypesMessage,
                     &vehicle, &VehicleController::getVehicleTypesMessage);
    QObject::connect(&client, &ClientController::getNodesMessage,
                     &vehicle, &VehicleController::getNodesMessage);
    QObject::connect(&client, &ClientController::launchMessage,
                     &vehicle, &VehicleController::launchMessage);
    QObject::connect(&client, &ClientController::unlaunchMessage,
                     &vehicle, &VehicleController::unlaunchMessage);
    QObject::connect(&client, &ClientController::set5GMessage,
                     &vehicle, &VehicleController::set5GMessage);
    QObject::connect(&client, &ClientController::resetPlanHandlerMessage,
                     &vehicle, &VehicleController::resetPlanHandlerMessage);
    QObject::connect(&client, &ClientController::setLedState,
                     &vehicle, &VehicleController::setLedState);
    QObject::connect(&client, &ClientController::setControlParams,
                     &vehicle, &VehicleController::setControlParams);
    QObject::connect(&client, &ClientController::takeoff,
                     &vehicle, &VehicleController::takeoff);
    QObject::connect(&client, &ClientController::arm,
                     &vehicle, &VehicleController::arm);
    QObject::connect(&client, &ClientController::setMode,
                     &vehicle, &VehicleController::setMode);
    QObject::connect(&client, &ClientController::testControlMessage,
                     &vehicle, &VehicleController::testControlMessage);
    QObject::connect(&client, &ClientController::getCameraScreenshotMessage,
                     &vehicle, &VehicleController::getCameraScreenshotMessage);
    QObject::connect(&client, &ClientController::setCodeMessage,
                     &vehicle, &VehicleController::setCodeMessage);
    QObject::connect(&client, &ClientController::uploadSoftwareMessage,
                     &vehicle, &VehicleController::uploadSoftwareMessage);
    QObject::connect(&client, &ClientController::uploadVisionMessage,
                     &vehicle, &VehicleController::uploadVisionMessage);
    QObject::connect(&client, &ClientController::applySoftwareMessage,
                     &vehicle, &VehicleController::applySoftwareMessage);
    QObject::connect(&client, &ClientController::applyVisionMessage,
                     &vehicle, &VehicleController::applyVisionMessage);
    QObject::connect(&client, &ClientController::setVisionEnabledMessage,
                     &vehicle, &VehicleController::setVisionEnabledMessage);
    QObject::connect(&client, &ClientController::setZoomCameraParams,
                     &vehicle, &VehicleController::setZoomCameraParams);
    QObject::connect(&client, &ClientController::rebootFC,
                     &vehicle, &VehicleController::rebootFC);
    QObject::connect(&client, &ClientController::startMagnitometerCalibration,
                     &vehicle, &VehicleController::startMagnitometerCalibration);
    QObject::connect(&client, &ClientController::acceptMagnitometerCalibration,
                     &vehicle, &VehicleController::acceptMagnitometerCalibration);
    QObject::connect(&client, &ClientController::cancelMagnitometerCalibration,
                     &vehicle, &VehicleController::cancelMagnitometerCalibration);
    QObject::connect(&client, &ClientController::startLevelCalibration,
                     &vehicle, &VehicleController::startLevelCalibration);
    QObject::connect(&client, &ClientController::clientVersionMessage,
                     &vehicle, &VehicleController::clientVersionMessage);
    QObject::connect(&client, &ClientController::setTelemetryParamsMessage,
                     &vehicle, &VehicleController::setTelemetryParamsMessage);
    QObject::connect(&client, &ClientController::getMapBounds,
                     &vehicle, &VehicleController::getMapBounds);
    QObject::connect(&client, &ClientController::getMapCRC,
                     &vehicle, &VehicleController::getMapCRC);
    QObject::connect(&client, &ClientController::poweroff,
                     &vehicle, &VehicleController::poweroff);
    QObject::connect(&client, &ClientController::stopProcMessage,
                     &application, &QCoreApplication::quit);
    QObject::connect(&vehicle, &VehicleController::sendAnswer,
                     &client, &ClientController::sendMessage);
    QObject::connect(&vehicle, &VehicleController::updateVehicleStatus,
                     &client, &ClientController::updateVehicleStatus);

    client.Init();
    return QCoreApplication::exec();
}
