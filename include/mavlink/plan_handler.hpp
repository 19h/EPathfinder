#pragma once

#include "mavlink/abstract_handler.hpp"
#include "road/road_types.hpp"

#include <QByteArray>
#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QVector>

#include <cstddef>
#include <cstdint>

enum class PlanItemType : std::int32_t {
    Waypoint = 0,
    MunitionArm = 1,
    Search = 2,
    Boom = 3,
    Spy1 = 4,
    Spy2 = 5,
    Land = 6,
};

enum class PlanRoadType : std::int32_t {
    None = 0,
    Across = 1,
    Along = 2,
    Search = 3,
    VNav = 4,
    Railway = 5,
};

// Layout recovered from QList<PlanItem>::append at RVA 0x195300 and every
// field access in PlanHandler::parsePlanItems at RVA 0x18f8f8.
struct PlanItem {
    double latitude{};
    double longitude{};
    double altitude{};
    double msl{136.0};
    std::int32_t radius{1000};
    bool windAttack{};
    std::uint8_t reserved37[3]{};
    std::int32_t timeout{};
    bool activate{};
    std::uint8_t reserved45[3]{};
    PlanItemType type{PlanItemType::Waypoint};
    PlanRoadType roadType{PlanRoadType::None};
    std::int32_t number{-1};
    std::int32_t reservedIndex0{-1};
    std::int32_t reservedIndex1{-1};
    bool trustGps{};
    std::uint8_t reserved69[3]{};
    float windSpeed{};
    std::int16_t windDirection{};
    std::uint16_t reserved78{};
};

static_assert(offsetof(PlanItem, msl) == 24U);
static_assert(offsetof(PlanItem, radius) == 32U);
static_assert(offsetof(PlanItem, timeout) == 40U);
static_assert(offsetof(PlanItem, type) == 48U);
static_assert(offsetof(PlanItem, roadType) == 52U);
static_assert(offsetof(PlanItem, number) == 56U);
static_assert(offsetof(PlanItem, trustGps) == 68U);
static_assert(offsetof(PlanItem, windSpeed) == 72U);
static_assert(offsetof(PlanItem, windDirection) == 76U);
static_assert(sizeof(PlanItem) == 80U);

struct PlanRoadPoint {
    std::int32_t number{};
    std::int32_t reserved{};
    double latitude{};
    double longitude{};
};

static_assert(sizeof(PlanRoadPoint) == 24U);

struct PlanRoadLink {
    std::int32_t from{};
    std::int32_t to{};
    std::int32_t category{};
};

static_assert(sizeof(PlanRoadLink) == 12U);

struct PlanTargetImage {
    QByteArray image;
    QString name;
    QString crc;
    std::int16_t x{};
    std::int16_t y{};
    std::int16_t direction{};
    std::uint16_t reserved30{};
    std::int32_t height{};
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(PlanTargetImage, x) == 24U);
static_assert(offsetof(PlanTargetImage, direction) == 28U);
static_assert(offsetof(PlanTargetImage, height) == 32U);
static_assert(sizeof(PlanTargetImage) == 40U);
#endif

struct Plan {
    QList<PlanItem> items;
    std::int32_t version{1};
    std::int32_t reserved12{};
    QList<PlanRoadPoint> roadPoints;
    QList<PlanRoadLink> roadLinks;
    QList<PlanTargetImage> targetImages;
    std::uint8_t reserved40{};
    bool hasRailway{};
    std::uint8_t reserved42[6]{};

    void clear();
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(Plan, version) == 8U);
static_assert(offsetof(Plan, roadPoints) == 16U);
static_assert(offsetof(Plan, roadLinks) == 24U);
static_assert(offsetof(Plan, targetImages) == 32U);
static_assert(offsetof(Plan, hasRailway) == 41U);
static_assert(sizeof(Plan) == 48U);
#endif

struct PlanWind {
    float speed{};
    std::uint16_t direction{};
    std::uint16_t reserved{};
};

static_assert(sizeof(PlanWind) == 8U);

class PlanHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit PlanHandler(MavLinkCommunicator* communicator);

    PlanWind globalWind() const;
    Plan currentPlan() const;
    RoadMap currentRoadMap() const;
    bool transactionInProgress() const;

    void setSaveLastPlan(bool value);
    void setPlanVersion(int version);
    void startReadPlan();
    void startWritePlan();

signals:
    void writePlanAnswer(QJsonDocument answer);
    void planWrited();

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void getPlan(const QJsonDocument& plan);
    void writePlan(const QJsonDocument& plan);
    void removePlanFile();
    void loadLastPlan();
    void setArmDisarm(bool value);
    void reset();

private slots:
    void transactionTimerEvent();
    void removePlan();

private:
    void sendSetCount();
    void sendWPRadius(int radius);
    void sendGetCount();
    void sendGetItem(int sequence);
    void sendSetItem(int sequence);
    void sendAnswer(int result,
                    const QString& text,
                    const QString& cookie = QString());
    bool checkMissionType(std::uint8_t missionType);
    void restartTransactionTimer();
    void handleMissionCount(const mavlink_message_t& message);
    void handleMissionItem(const mavlink_message_t& message);
    void handleMissionRequest(const mavlink_message_t& message);
    void handleMissionAck(const mavlink_message_t& message);
    void parsePlanItems(const QJsonDocument& document);
    QJsonDocument planAnswer(const QString& cookie) const;
    void saveLastPlan();
    void roadMapFromPlan();
    void searchNeighbours();

    bool transactionInProgress_{};
    QString cookie_;
    QTimer transactionTimer_;
    Plan plan_;
    RoadMap roadMap_;
    std::uint64_t reservedTransactionState_{};
    bool reservedRoadMapFlag_{};
    std::int32_t transactionMinimumMs_{2000};
    std::int32_t transactionMaximumMs_{20000};
    std::int32_t transactionPerItemMs_{200};
    PlanWind globalWind_{};
    QJsonDocument lastPlan_;
    bool saveLastPlan_{};
    std::int32_t planVersion_{1};
    bool fixedTwoItemUpload_{true};
    std::uint16_t missionCount_{};
    std::uint16_t receivedItemCount_{};
};
