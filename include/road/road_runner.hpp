#pragma once

#include "road/roads.hpp"
#include "scout/scout_types.hpp"

#include <QGeoCoordinate>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QPointF>
#include <QVector>

class QTimerEvent;

class RoadRunner : public QObject {
    Q_OBJECT

public:
    enum Mode : int {
        DisabledMode = 0,
        RoadMode = 1,
        RoadElementsMode = 2,
    };

    explicit RoadRunner(QObject* parent = nullptr);
    ~RoadRunner() override = default;

    void setMaxYawOffset(float degrees);
    void setMinConfidence(float value);
    bool isEnabled() const;
    bool isCalculatePlaneYawEnabled() const;
    void setCalculatePlaneYawEnabled(bool enabled);
    bool isSearchRoadElementsEnabled() const;
    void setSearchRoadElementsEnabled(bool enabled);
    bool isAlongOffsetEnabled() const;
    void setAlongOffsetEnabled(bool enabled);
    bool calculateTemporaryOffset(const QGeoCoordinate& coordinate,
                                  QGeoCoordinate& shifted) const;
    qlonglong lastDetectedTimeRoadItem(int planItemIndex) const;
    bool hasTurns() const;
    void setMode(Mode mode);
    Mode currentMode() const;
    void movePlaneAlongRoad(const QGeoCoordinate& coordinate,
                            int roadItemIndex,
                            float distanceMetres,
                            float& eastMetres,
                            float& northMetres) const;
    bool isEqualOffsets(double firstEast,
                        double firstNorth,
                        double secondEast,
                        double secondNorth) const;
    void accept();
    void match();
    QList<RoadElement*> planPointRoadElements() const;
    void clear();
    void setEnabled(bool enabled);
    qlonglong lastDetectedTimeElement(int index) const;
    QGeoCoordinate lastTurn() const;
    bool roadElementIsFork(int index) const;
    void offsetByAlongs();
    void calcNearbyMapItems(const QGeoCoordinate& coordinate,
                            float minimumDistanceMetres,
                            float maximumDistanceMetres,
                            float minimumHeadingDegrees,
                            float maximumHeadingDegrees,
                            QList<int>& output) const;
    float mapNearestDistance(const QPointF& local,
                             QPointF& projected,
                             int& itemIndex) const;
    float mapNearestDistance(const QGeoCoordinate& coordinate,
                             QPointF& projected,
                             int& itemIndex) const;
    float roadProjCoord(const QGeoCoordinate& coordinate,
                        QGeoCoordinate& projected,
                        RoadValidateOffset& offset,
                        int& itemIndex) const;
    bool pathsIsEqual(
        const QVector<QPair<QGeoCoordinate, QGeoCoordinate>>& paths) const;
    float roadDistance(const QGeoCoordinate& coordinate) const;
    void calculateLocalCoords();
    void setMap(RoadMap* map);
    void setCenterPos(const QGeoCoordinate& coordinate);

signals:
    void offsetItemAdded(const RoadValidateResult& item);
    void offsetChanged(const RoadValidateResult& result);
    void elementsDetected(qlonglong time, QList<RoadElement*> elements);
    void lookAt(const QGeoCoordinate& coordinate);
    void stopLookAt();

public slots:
    void addData(const QVector<QGeoCoordinate>& visibleCoords,
                 bool isSingleRoadLine,
                 const QGeoCoordinate& planeCoord,
                 const QGeoCoordinate& planeRawCoord,
                 const QGeoCoordinate& viewCenter,
                 int prevWPPlanIdx,
                 int nextWPPlanIdx,
                 const QGeoCoordinate& prevWPCoord,
                 const QGeoCoordinate& nextWPCoord,
                 int planeAltitude,
                 float planeYaw,
                 float viewAngle,
                 float viewDist,
                 float currentYawOffset,
                 float maxOffset,
                 bool isExactMagValue);
    void addData2(const QVector<QGeoCoordinate>& visibleCoords,
                  bool isSingleRoadLine,
                  const QGeoCoordinate& planeCoord,
                  const QGeoCoordinate& planeRawCoord,
                  const QGeoCoordinate& viewCenter,
                  int prevWPPlanIdx,
                  int nextWPPlanIdx,
                  const QGeoCoordinate& prevWPCoord,
                  const QGeoCoordinate& nextWPCoord,
                  int planeAltitude,
                  float planeYaw,
                  float viewAngle,
                  float viewDist,
                  float currentYawOffset,
                  float maxOffset,
                  bool isExactMagValue,
                  bool isObjectAsRoadPoint);

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void detectedElementsHandler(const QVector<RoadElement>& elements);

private:
    static bool headingInRange(float heading,
                               float minimum,
                               float maximum);
    void calculateAverageOffset();

    Mode mode_{DisabledMode};
    RoadAnalyzerV2* analyzer_{};
    RoadMap* map_{};
    int matchTimerId_{};
    float maximumYawOffsetDegrees_{20.0F};
    float minimumConfidence_{0.5F};
    QVector<RoadValidateResult> pendingOffsets_;
    QVector<RoadElement> detectedElements_;
    QVector<RoadValidateResult> acceptedOffsets_;
    RoadValidateResult currentOffset_;
    QMap<int, qlonglong> roadItemDetectionTimes_;
    bool enabled_{};
    bool alongOffsetEnabled_{true};
    bool calculatePlaneYawEnabled_{true};
    bool searchRoadElementsEnabled_{true};
    QGeoCoordinate centerPosition_;
};

Q_DECLARE_METATYPE(QList<RoadElement*>)

