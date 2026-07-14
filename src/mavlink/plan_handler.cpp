#include "mavlink/plan_handler.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>
#include <cmath>

namespace {

constexpr char kPlanFile[] = "plan.json";
constexpr std::uint8_t kMissionSystem = 1U;
constexpr std::uint8_t kMissionComponent = 1U;

PlanItem dummyPlanItem()
{
    PlanItem item;
    item.msl = 0.0;
    item.radius = 1000;
    item.number = -1;
    item.reservedIndex0 = -1;
    item.reservedIndex1 = -1;
    return item;
}

} // namespace

void Plan::clear()
{
    items.clear();
    roadPoints.clear();
    roadLinks.clear();
    targetImages.clear();
}

PlanHandler::PlanHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    transactionTimer_.setTimerType(Qt::CoarseTimer);
    connect(&transactionTimer_,
            &QTimer::timeout,
            this,
            &PlanHandler::transactionTimerEvent);
}

PlanWind PlanHandler::globalWind() const
{
    return globalWind_;
}

Plan PlanHandler::currentPlan() const
{
    return plan_;
}

RoadMap PlanHandler::currentRoadMap() const
{
    return roadMap_;
}

bool PlanHandler::transactionInProgress() const
{
    return transactionInProgress_;
}

void PlanHandler::setSaveLastPlan(const bool value)
{
    saveLastPlan_ = value;
}

void PlanHandler::setPlanVersion(const int version)
{
    planVersion_ = version;
}

void PlanHandler::initIntervals()
{
}

void PlanHandler::reset()
{
    transactionInProgress_ = false;
}

void PlanHandler::setArmDisarm(const bool value)
{
    mavlink_message_t message{};
    mavlink_msg_command_long_pack(communicator_->systemId(),
                                  MAV_COMP_ID_ONBOARD_COMPUTER,
                                  &message,
                                  communicator_->systemId(),
                                  communicator_->componentId(),
                                  MAV_CMD_COMPONENT_ARM_DISARM,
                                  6U,
                                  value ? 1.0F : 0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::sendSetCount()
{
    const auto count = static_cast<std::uint16_t>(
        fixedTwoItemUpload_ ? 2 : plan_.items.size());
    mavlink_message_t message{};
    mavlink_msg_mission_count_pack(communicator_->systemId(),
                                   communicator_->componentId(),
                                   &message,
                                   kMissionSystem,
                                   kMissionComponent,
                                   count,
                                   MAV_MISSION_TYPE_MISSION,
                                   0U);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::startWritePlan()
{
    sendSetCount();
}

void PlanHandler::sendWPRadius(const int radius)
{
    mavlink_message_t message{};
    mavlink_msg_param_set_pack(communicator_->systemId(),
                               MAV_COMP_ID_ONBOARD_COMPUTER,
                               &message,
                               communicator_->systemId(),
                               communicator_->componentId(),
                               "WP_RADIUS",
                               static_cast<float>(radius),
                               MAV_PARAM_TYPE_INT32);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::sendGetCount()
{
    mavlink_message_t message{};
    mavlink_msg_mission_request_list_pack(communicator_->systemId(),
                                          communicator_->componentId(),
                                          &message,
                                          kMissionSystem,
                                          kMissionComponent,
                                          MAV_MISSION_TYPE_MISSION);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::sendGetItem(const int sequence)
{
    mavlink_message_t message{};
    mavlink_msg_mission_request_int_pack(
        communicator_->systemId(),
        communicator_->componentId(),
        &message,
        kMissionSystem,
        kMissionComponent,
        static_cast<std::uint16_t>(sequence),
        MAV_MISSION_TYPE_MISSION);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::startReadPlan()
{
    reservedTransactionState_ = 0U;
    missionCount_ = 0U;
    receivedItemCount_ = 0U;
    plan_.clear();
    sendGetCount();
}

void PlanHandler::sendAnswer(const int result,
                             const QString& text,
                             const QString& cookie)
{
    QJsonObject object;
    object.insert(QStringLiteral("cookie"),
                  cookie.isEmpty() ? cookie_ : cookie);
    object.insert(QStringLiteral("result"), result);
    if (!text.isEmpty()) {
        object.insert(QStringLiteral("text"), text);
    }
    emit writePlanAnswer(QJsonDocument(object));
}

bool PlanHandler::checkMissionType(const std::uint8_t missionType)
{
    if (missionType == MAV_MISSION_TYPE_MISSION) {
        return true;
    }
    sendAnswer(3, QStringLiteral("mission type error"));
    transactionInProgress_ = false;
    return false;
}

void PlanHandler::restartTransactionTimer()
{
    transactionTimer_.stop();
    const int itemCount = plan_.items.size();
    const int interval = std::clamp(itemCount * transactionPerItemMs_,
                                    transactionMinimumMs_,
                                    transactionMaximumMs_);
    transactionTimer_.start(interval);
}

void PlanHandler::transactionTimerEvent()
{
    if (transactionInProgress_) {
        sendAnswer(4, QStringLiteral("transaction timeout"));
        transactionInProgress_ = false;
    }
}

void PlanHandler::handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t count{};
    mavlink_msg_mission_count_decode(&message, &count);
    missionCount_ = count.count;
    receivedItemCount_ = 0U;
    // The original logs count.mission_type but does not call
    // _checkMissionType here (RVA 0x18d1b8).
    sendGetItem(0);
}

void PlanHandler::handleMissionItem(const mavlink_message_t& message)
{
    mavlink_mission_item_int_t mission{};
    mavlink_msg_mission_item_int_decode(&message, &mission);

    PlanItem item = dummyPlanItem();
    item.latitude = static_cast<double>(mission.x) / 10000000.0;
    item.longitude = static_cast<double>(mission.y) / 10000000.0;
    item.altitude = mission.z;
    item.timeout = 1000;
    item.number = 0;
    item.reservedIndex0 = -1;
    item.reservedIndex1 = -1;
    plan_.items.append(item);

    ++receivedItemCount_;
    if (receivedItemCount_ < missionCount_) {
        sendGetItem(receivedItemCount_);
        return;
    }

    mavlink_message_t acknowledgement{};
    mavlink_msg_mission_ack_pack(communicator_->systemId(),
                                 communicator_->componentId(),
                                 &acknowledgement,
                                 kMissionSystem,
                                 kMissionComponent,
                                 MAV_MISSION_ACCEPTED,
                                 MAV_MISSION_TYPE_MISSION,
                                 0U);
    communicator_->sendMessageOnMainLink(acknowledgement);
    emit writePlanAnswer(planAnswer(cookie_));
    transactionInProgress_ = false;
}

void PlanHandler::sendSetItem(const int sequence)
{
    const PlanItem& item = plan_.items[sequence];
    const float parameter2 = sequence == 1 ? 14.0F : 0.0F;
    const std::uint8_t frame = sequence == 0
        ? MAV_FRAME_GLOBAL
        : MAV_FRAME_GLOBAL_RELATIVE_ALT;

    mavlink_message_t message{};
    // x/y are literal constants at RVA 0x18f58c: 50.4303588 degrees N,
    // 30.5038419 degrees E. Only altitude is read from the selected item.
    mavlink_msg_mission_item_int_pack(
        communicator_->systemId(),
        communicator_->componentId(),
        &message,
        kMissionSystem,
        kMissionComponent,
        static_cast<std::uint16_t>(sequence),
        frame,
        MAV_CMD_NAV_WAYPOINT,
        0U,
        1U,
        0.0F,
        parameter2,
        0.0F,
        0.0F,
        504303588,
        305038419,
        static_cast<float>(item.altitude),
        MAV_MISSION_TYPE_MISSION);
    communicator_->sendMessageOnMainLink(message);
}

void PlanHandler::handleMissionRequest(const mavlink_message_t& message)
{
    std::uint16_t sequence = 0U;
    std::uint8_t missionType = MAV_MISSION_TYPE_MISSION;
    if (message.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT) {
        mavlink_mission_request_int_t request{};
        mavlink_msg_mission_request_int_decode(&message, &request);
        sequence = request.seq;
        missionType = request.mission_type;
    } else {
        mavlink_mission_request_t request{};
        mavlink_msg_mission_request_decode(&message, &request);
        sequence = request.seq;
        missionType = request.mission_type;
    }
    if (checkMissionType(missionType)) {
        sendSetItem(sequence);
    }
}

void PlanHandler::saveLastPlan()
{
    QFile file(QString::fromLatin1(kPlanFile));
    (void)file.open(QIODevice::WriteOnly);
    (void)file.write(lastPlan_.toJson());
    file.close();
}

void PlanHandler::handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t acknowledgement{};
    mavlink_msg_mission_ack_decode(&message, &acknowledgement);
    // RVA 0x1947a8 validates mission_type but does not inspect type.
    if (checkMissionType(acknowledgement.mission_type)) {
        sendWPRadius(20);
        sendAnswer(0, QString());
        emit planWrited();
        if (saveLastPlan_) {
            saveLastPlan();
        }
    }
    transactionInProgress_ = false;
}

void PlanHandler::processMessage(const mavlink_message_t& message)
{
    if (!transactionInProgress_) {
        return;
    }
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_REQUEST:
    case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
        handleMissionRequest(message);
        break;
    case MAVLINK_MSG_ID_MISSION_COUNT:
        handleMissionCount(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ACK:
        handleMissionAck(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ITEM_INT:
        handleMissionItem(message);
        break;
    default:
        break;
    }
}

QJsonDocument PlanHandler::planAnswer(const QString& cookie) const
{
    QJsonObject result;
    result.insert(QStringLiteral("cookie"), cookie);
    result.insert(QStringLiteral("count"),
                  std::max(static_cast<int>(plan_.items.size()) - 1, 0));

    QJsonArray nodes;
    for (int index = 1; index < plan_.items.size(); ++index) {
        const PlanItem& item = plan_.items[index];
        QJsonObject node;
        node.insert(QStringLiteral("number"), index - 1);
        node.insert(QStringLiteral("lat"), item.latitude);
        node.insert(QStringLiteral("lon"), item.longitude);
        node.insert(QStringLiteral("alt"), item.altitude);
        nodes.append(node);
    }
    result.insert(QStringLiteral("nodes"), nodes);
    return QJsonDocument(result);
}

void PlanHandler::getPlan(const QJsonDocument& request)
{
    const QString requestCookie =
        request.object().value(QStringLiteral("cookie")).toString();
    if (transactionInProgress_) {
        sendAnswer(1,
                   QStringLiteral(
                       "read plan called while prev transaction in progress"),
                   requestCookie);
        return;
    }

    if (plan_.items.isEmpty()) {
        transactionInProgress_ = true;
        restartTransactionTimer();
        cookie_ = requestCookie;
        startReadPlan();
        return;
    }

    cookie_ = requestCookie;
    QJsonObject result = planAnswer(cookie_).object();
    QJsonArray nodes;
    for (int index = 1; index < plan_.items.size(); ++index) {
        const PlanItem& item = plan_.items[index];
        QJsonObject node;
        node.insert(QStringLiteral("number"), index - 1);
        node.insert(QStringLiteral("lat"), item.latitude);
        node.insert(QStringLiteral("lon"), item.longitude);
        node.insert(QStringLiteral("alt"), item.altitude);
        node.insert(QStringLiteral("msl"), item.msl);
        node.insert(QStringLiteral("type"), static_cast<int>(item.type));
        if (item.type == PlanItemType::Search) {
            node.insert(QStringLiteral("radius"), item.radius);
            node.insert(QStringLiteral("timeout"), item.timeout);
        }
        node.insert(QStringLiteral("road_type"),
                    static_cast<int>(item.roadType));
        node.insert(QStringLiteral("trust_gps"), item.trustGps ? 1 : 0);
        QJsonObject wind;
        wind.insert(QStringLiteral("direction"), item.windDirection);
        wind.insert(QStringLiteral("speed"), item.windSpeed);
        // The binary constructs this wind object but omits the insertion into
        // node before appending it (RVA 0x18e4f0..0x18e590).
        nodes.append(node);
    }
    result.insert(QStringLiteral("nodes"), nodes);
    emit writePlanAnswer(QJsonDocument(result));
}

void PlanHandler::parsePlanItems(const QJsonDocument& document)
{
    plan_.clear();
    plan_.version = planVersion_;
    plan_.hasRailway = false;
    cookie_ = document.object().value(QStringLiteral("cookie")).toString();

    const QJsonValue dataValue =
        document.object().value(QStringLiteral("data"));
    if (!dataValue.isUndefined() && !dataValue.isNull()) {
        const QJsonObject data = dataValue.toObject();
        const bool windAttack =
            data.value(QStringLiteral("wind_attack")).toInt() > 0;
        const QJsonArray nodes = data.value(QStringLiteral("nodes")).toArray();
        for (const QJsonValue& nodeValue : nodes) {
            const QJsonObject node = nodeValue.toObject();
            PlanItem item;
            item.number = node.value(QStringLiteral("number")).toInt();
            item.latitude = node.value(QStringLiteral("lat")).toDouble();
            item.longitude = node.value(QStringLiteral("lon")).toDouble();
            item.altitude = node.value(QStringLiteral("alt")).toDouble();
            item.msl = 136.0;
            item.radius =
                std::max(node.value(QStringLiteral("radius")).toInt(1000),
                         500);
            item.timeout =
                std::max(node.value(QStringLiteral("timeout")).toInt(), 0);
            item.activate =
                node.value(QStringLiteral("activate")).toInt() > 0;
            item.type = static_cast<PlanItemType>(
                node.value(QStringLiteral("type")).toInt());
            item.roadType = static_cast<PlanRoadType>(
                node.value(QStringLiteral("road_type")).toInt());
            item.windAttack = windAttack;
            plan_.hasRailway = plan_.hasRailway
                || item.roadType == PlanRoadType::Railway;

            if (planVersion_ > 1) {
                item.trustGps =
                    node.value(QStringLiteral("trust_gps")).toInt() > 0;
                const QJsonObject wind =
                    node.value(QStringLiteral("wind")).toObject();
                item.windDirection = static_cast<std::int16_t>(
                    wind.value(QStringLiteral("direction")).toInt());
                item.windSpeed = static_cast<float>(
                    wind.value(QStringLiteral("speed")).toInt());
            }
            plan_.items.append(item);
        }

        if (planVersion_ <= 1) {
            const QJsonObject wind =
                data.value(QStringLiteral("wind")).toObject();
            globalWind_.direction = static_cast<std::uint16_t>(
                wind.value(QStringLiteral("direction")).toInt());
            globalWind_.speed = static_cast<float>(
                wind.value(QStringLiteral("speed")).toDouble());
        } else {
            const QJsonArray roadNodes =
                data.value(QStringLiteral("road_nodes")).toArray();
            for (const QJsonValue& pointValue : roadNodes) {
                const QJsonObject point = pointValue.toObject();
                plan_.roadPoints.append(PlanRoadPoint{
                    point.value(QStringLiteral("number")).toInt() + 1,
                    0,
                    point.value(QStringLiteral("lat")).toDouble(),
                    point.value(QStringLiteral("lng")).toDouble()});
            }

            const QJsonArray roadLinks =
                data.value(QStringLiteral("road_links")).toArray();
            for (const QJsonValue& linkValue : roadLinks) {
                const QJsonObject link = linkValue.toObject();
                plan_.roadLinks.append(PlanRoadLink{
                    link.value(QStringLiteral("from")).toInt() + 1,
                    link.value(QStringLiteral("to")).toInt() + 1,
                    link.value(QStringLiteral("category")).toInt()});
            }

            const QJsonArray images =
                data.value(QStringLiteral("target_images")).toArray();
            for (const QJsonValue& imageValue : images) {
                const QJsonObject object = imageValue.toObject();
                PlanTargetImage image;
                image.image = QByteArray::fromBase64(
                    object.value(QStringLiteral("img"))
                        .toVariant()
                        .toByteArray());
                image.name = object.value(QStringLiteral("name")).toString();
                image.crc = object.value(QStringLiteral("crc")).toString();
                image.x = static_cast<std::int16_t>(
                    object.value(QStringLiteral("x")).toInt());
                image.y = static_cast<std::int16_t>(
                    object.value(QStringLiteral("y")).toInt());
                image.direction = static_cast<std::int16_t>(
                    object.value(QStringLiteral("dir")).toInt());
                image.height = object.value(QStringLiteral("h")).toInt();

                const QString calculated = QString::fromLatin1(
                    QCryptographicHash::hash(image.image,
                                             QCryptographicHash::Md5)
                        .toHex());
                if (calculated != image.crc) {
                    throw QStringLiteral("image '") + image.name
                        + QStringLiteral("' crc error:") + calculated
                        + QLatin1Char(':') + image.crc;
                }
                plan_.targetImages.append(image);
            }
        }
    }

    if (plan_.items.isEmpty()) {
        throw QStringLiteral("plan is empty or invalid");
    }
    plan_.items.prepend(dummyPlanItem());
}

void PlanHandler::roadMapFromPlan()
{
    roadMap_ = {};
    for (const PlanRoadPoint& point : plan_.roadPoints) {
        RoadPoint roadPoint;
        roadPoint.number = point.number;
        roadPoint.coordinate =
            QGeoCoordinate(point.latitude, point.longitude);
        roadMap_.points.append(roadPoint);
    }
    for (const PlanRoadLink& link : plan_.roadLinks) {
        RoadItem item;
        item.index = roadMap_.items.size();
        item.planItemIndex = link.category;
        item.point0 = link.from;
        item.point1 = link.to;
        roadMap_.items.append(item);
        roadMap_.outgoing[link.from].append(link.to);
        roadMap_.incoming[link.to].append(link.from);
    }
    searchNeighbours();
}

void PlanHandler::searchNeighbours()
{
    for (int i = 0; i < roadMap_.items.size(); ++i) {
        RoadItem& item = roadMap_.items[i];
        for (int j = 0; j < roadMap_.items.size(); ++j) {
            if (i == j) {
                continue;
            }
            RoadItem& candidate = roadMap_.items[j];
            if (item.topology0 < 0
                && (item.point0 == candidate.point0
                    || item.point0 == candidate.point1)) {
                item.topology0 = j;
            }
            if (item.topology1 < 0
                && (item.point1 == candidate.point0
                    || item.point1 == candidate.point1)) {
                item.topology1 = j;
            }
        }
    }
}

void PlanHandler::writePlan(const QJsonDocument& document)
{
    const QString requestCookie =
        document.object().value(QStringLiteral("cookie")).toString();
    if (transactionInProgress_) {
        sendAnswer(1,
                   QStringLiteral(
                       "write plan called while prev transaction in progress"),
                   requestCookie);
        return;
    }

    transactionInProgress_ = true;
    try {
        parsePlanItems(document);
    } catch (const QString& error) {
        transactionInProgress_ = false;
        sendAnswer(2, error);
        return;
    }
    lastPlan_ = document;
    restartTransactionTimer();
    transactionInProgress_ = false;
    sendAnswer(0, QString());
    emit planWrited();
}

void PlanHandler::removePlanFile()
{
    QTimer::singleShot(60000, this, &PlanHandler::removePlan);
}

void PlanHandler::removePlan()
{
    (void)QFile::remove(QString::fromLatin1(kPlanFile));
}

void PlanHandler::loadLastPlan()
{
    QFile file(QString::fromLatin1(kPlanFile));
    (void)file.open(QIODevice::ReadOnly);
    QJsonParseError error{};
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error == QJsonParseError::NoError) {
        writePlan(document);
    }
}
