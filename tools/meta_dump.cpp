#include "camera/camera.hpp"
#include "camera/camera_t205.hpp"
#include "camera/camera_usb.hpp"
#include "camera/camera_v4l.hpp"
#include "camera/camera_zr10.hpp"
#include "controller/client_controller.hpp"
#include "controller/vehicle_controller.hpp"
#include "core/kalman.hpp"
#include "elink/elink_abstract_handler.hpp"
#include "elink/elink_arm_handler.hpp"
#include "elink/elink_attitude_handler.hpp"
#include "elink/elink_communicator.hpp"
#include "elink/elink_flower_handler.hpp"
#include "elink/elink_gimbal_handler.hpp"
#include "elink/elink_interceptor_handler.hpp"
#include "elink/elink_movement_handler.hpp"
#include "elink/elink_position_handler.hpp"
#include "elink/elink_shumodav_handler.hpp"
#include "elink/elink_status_handler.hpp"
#include "elink/elink_telemetry.hpp"
#include "interception/einterceptor.hpp"
#include "led/vehicle_led.hpp"
#include "lidar/interceptor_lidar.hpp"
#include "lidar/interceptor_livox.hpp"
#include "link/abstract_link.hpp"
#include "link/serial_link.hpp"
#include "link/tcp_link.hpp"
#include "link/udp_link.hpp"
#include "logging/elogger.hpp"
#include "mavlink/abstract_handler.hpp"
#include "mavlink/attitude_handler.hpp"
#include "mavlink/control_handler.hpp"
#include "mavlink/gps_handler.hpp"
#include "mavlink/heartbeat_handler.hpp"
#include "mavlink/log_handler.hpp"
#include "mavlink/mavlink_communicator.hpp"
#include "mavlink/plan_handler.hpp"
#include "mavlink/rc_channels_handler.hpp"
#include "mavlink/time_sync_handler.hpp"
#include "mavlink/wind_handler.hpp"
#include "pathfinder/pathfinder.hpp"
#include "remote/remote_controller.hpp"
#include "road/road_runner.hpp"
#include "road/roads.hpp"
#include "scout/escout.hpp"
#include "vision/thunder_gaze.hpp"
#include "vision/vision.hpp"
#include "vnav/vnav.hpp"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaMethod>

#include <cstdio>

namespace {

QString methodKind(const QMetaMethod::MethodType type)
{
    switch (type) {
    case QMetaMethod::Signal: return QStringLiteral("signal");
    case QMetaMethod::Slot: return QStringLiteral("slot");
    case QMetaMethod::Constructor: return QStringLiteral("constructor");
    case QMetaMethod::Method: return QStringLiteral("method");
    }
    return QStringLiteral("unknown");
}

QString accessKind(const QMetaMethod::Access access)
{
    switch (access) {
    case QMetaMethod::Private: return QStringLiteral("private");
    case QMetaMethod::Protected: return QStringLiteral("protected");
    case QMetaMethod::Public: return QStringLiteral("public");
    }
    return QStringLiteral("unknown");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const QMetaObject* const objects[] = {
        &AbstractHandler::staticMetaObject,
        &AbstractLink::staticMetaObject,
        &AttitudeHandler::staticMetaObject,
        &Camera::staticMetaObject,
        &CameraT205::staticMetaObject,
        &CameraUSB::staticMetaObject,
        &CameraV4L::staticMetaObject,
        &CameraZR10::staticMetaObject,
        &ClientController::staticMetaObject,
        &ControlHandler::staticMetaObject,
        &EInterceptor::staticMetaObject,
        &ELinkAbstractHandler::staticMetaObject,
        &ELinkArmHandler::staticMetaObject,
        &ELinkAttitudeHandler::staticMetaObject,
        &ELinkCommunicator::staticMetaObject,
        &ELinkFlowerHandler::staticMetaObject,
        &ELinkGimbalHandler::staticMetaObject,
        &ELinkInterceptorHandler::staticMetaObject,
        &ELinkMovemenetHandler::staticMetaObject,
        &ELinkPositionHandler::staticMetaObject,
        &ELinkShumodavHandler::staticMetaObject,
        &ELinkStatusHandler::staticMetaObject,
        &ELinkTelemetry::staticMetaObject,
        &ELogger::staticMetaObject,
        &EPathfinder::staticMetaObject,
        &EScout::staticMetaObject,
        &EVision::staticMetaObject,
        &GPIOController::staticMetaObject,
        &GpsHandler::staticMetaObject,
        &HeartbeatHandler::staticMetaObject,
        &InterceptorLidar::staticMetaObject,
        &InterceptorLivox::staticMetaObject,
        &Kalman::staticMetaObject,
        &LogHandler::staticMetaObject,
        &MavLinkCommunicator::staticMetaObject,
        &PlanHandler::staticMetaObject,
        &RcChannelsHandler::staticMetaObject,
        &RemoteController::staticMetaObject,
        &RoadAnalyzer::staticMetaObject,
        &RoadAnalyzerV1::staticMetaObject,
        &RoadAnalyzerV2::staticMetaObject,
        &RoadNetworkAnalyzer::staticMetaObject,
        &RoadRunner::staticMetaObject,
        &SerialLink::staticMetaObject,
        &TcpLink::staticMetaObject,
        &ThunderGaze::staticMetaObject,
        &ThunderGazeV1::staticMetaObject,
        &ThunderGazeV2::staticMetaObject,
        &ThunderGazeV3::staticMetaObject,
        &ThunderGazeV4::staticMetaObject,
        &ThunderGazeV5::staticMetaObject,
        &TimeSyncHandler::staticMetaObject,
        &UdpLink::staticMetaObject,
        &VNav::staticMetaObject,
        &VehicleController::staticMetaObject,
        &VehicleLed::staticMetaObject,
        &WindHandler::staticMetaObject,
    };

    QJsonArray classes;
    for (const QMetaObject* const object : objects) {
        QJsonObject classObject;
        classObject.insert(QStringLiteral("class_name"), object->className());
        QJsonArray methods;
        for (int index = object->methodOffset(); index < object->methodCount(); ++index) {
            const QMetaMethod method = object->method(index);
            QJsonObject methodObject;
            methodObject.insert(QStringLiteral("signature"),
                                QString::fromLatin1(method.methodSignature()));
            methodObject.insert(QStringLiteral("kind"), methodKind(method.methodType()));
            methodObject.insert(QStringLiteral("access"), accessKind(method.access()));
            methodObject.insert(QStringLiteral("return_type"),
                                QString::fromLatin1(method.typeName()));
            methods.append(methodObject);
        }
        classObject.insert(QStringLiteral("methods"), methods);
        classes.append(classObject);
    }

    const QByteArray output = QJsonDocument(classes).toJson(QJsonDocument::Compact);
    std::fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    std::fputc('\n', stdout);
    return 0;
}
