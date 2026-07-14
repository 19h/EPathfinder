#pragma once

#include "camera/camera.hpp"
#include "vision/vision_types.hpp"

#include <QByteArray>
#include <QHostAddress>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QVector>

class QUdpSocket;

class ThunderGaze : public QObject {
    Q_OBJECT

public:
    explicit ThunderGaze(QObject* parent, QUdpSocket* socket);
    ~ThunderGaze() override = default;

    void setFrameDelay(int milliseconds);
    void setPTZOffset(float horizontalDegrees, float verticalDegrees);
    void setDebugDTPEnabled(bool enabled);
    void setDetTypeEnabled(int index, bool enabled);
    void setRoadsEnabled(bool enabled);
    void setRoadSimpleMode(bool enabled);
    void setCamera(Camera* camera);
    void setReduceBorderEnabled(bool enabled);
    void setIsRailway(unsigned char value);

    virtual void setPreprocessingScale(float value);
    virtual void setFrameText(const QStringList& lines);
    virtual void setRTSPEnabled(bool enabled);
    virtual void setTelemetryParams(const QJsonDocument& params);
    virtual void setObjectDetectorEnabled(bool enabled);
    virtual void setPathfinderEnabled(bool enabled);
    virtual void setVideoSaverEnabled(bool enabled);

    void splitRoadScreenParts(qlonglong time,
                              const QVector<QPoint>& points);
    void clearIgnores();
    void addIgnoreBound(const IgnoreBound& bound);
    void reduceRoadPoints(QVector<QPoint>& points) const;
    void reduceRoadPointsSimpleMode(QVector<QPoint>& points) const;
    void calculateRoadPointAngles(const CameraParams& params,
                                  const QVector<QPoint>& points,
                                  QVector<RoadPointAngle>& output) const;

    virtual qsizetype messageSize() const = 0;
    virtual bool parseMessage(const QByteArray& message,
                              qlonglong receiveTime,
                              const QHostAddress& sender,
                              quint16 senderPort) = 0;

signals:
    void emptyDTP();
    void setTargets(QList<Target>& targets);
    void preprocessingScaleChanged(float value);
    void dtpDebugInfo(const QString& text);
    void roadScreenParts(qlonglong time, int left, int middle, int right);
    void roadDetected(qlonglong time,
                      const CameraParams& cameraParams,
                      const QVector<RoadPointAngle>& roadPoints,
                      bool isSingleLine);
    void rtspEnabled();
    void rtspDisabled();
    void objectDetectorEnabled(bool enabled);
    void pathfinderEnabled(bool enabled);
    void videoSaverEnabled(bool enabled);
    void frameBlur(float value);

protected:
    bool acceptsMessage(const QByteArray& message) const;

    QUdpSocket* socket_{};
    Camera* camera_{};
    int frameDelayMs_{156};
    float ptzOffsetX_{};
    float ptzOffsetY_{};
    bool debugDtpEnabled_{};
    bool detectionTypes_[4]{};
    bool roadsEnabled_{};
    bool roadSimpleMode_{};
    QList<IgnoreBound> ignoreBounds_;
    bool reduceBorderEnabled_{};
    int reductionRows_{54};
    int reductionColumns_{96};
    unsigned char railway_{};
    float preprocessingScale_{1.0F};
    QStringList frameText_;
    QJsonDocument telemetryParams_;
};

class ThunderGazeV1 final : public ThunderGaze {
    Q_OBJECT

public:
    explicit ThunderGazeV1(QObject* parent, QUdpSocket* socket);
    qsizetype messageSize() const override;
    bool parseMessage(const QByteArray& message,
                      qlonglong receiveTime,
                      const QHostAddress& sender,
                      quint16 senderPort) override;
};

class ThunderGazeV2 final : public ThunderGaze {
    Q_OBJECT

public:
    explicit ThunderGazeV2(QObject* parent, QUdpSocket* socket);
    qsizetype messageSize() const override;
    void setPreprocessingScale(float value) override;
    bool parseMessage(const QByteArray& message,
                      qlonglong receiveTime,
                      const QHostAddress& sender,
                      quint16 senderPort) override;
};

class ThunderGazeV3 final : public ThunderGaze {
    Q_OBJECT

public:
    explicit ThunderGazeV3(QObject* parent, QUdpSocket* socket);
    qsizetype messageSize() const override;
    void setPreprocessingScale(float value) override;
    void setFrameText(const QStringList& lines) override;
    bool parseMessage(const QByteArray& message,
                      qlonglong receiveTime,
                      const QHostAddress& sender,
                      quint16 senderPort) override;
};

class ThunderGazeV4 : public ThunderGaze {
    Q_OBJECT

public:
    explicit ThunderGazeV4(QObject* parent, QUdpSocket* socket);
    qsizetype messageSize() const override;
    void setPreprocessingScale(float value) override;
    void setFrameText(const QStringList& lines) override;
    void setRTSPEnabled(bool enabled) override;
    void setTelemetryParams(const QJsonDocument& params) override;
    void setObjectDetectorEnabled(bool enabled) override;
    void setPathfinderEnabled(bool enabled) override;
    bool parseMessage(const QByteArray& message,
                      qlonglong receiveTime,
                      const QHostAddress& sender,
                      quint16 senderPort) override;

private slots:
    void applyTelemetryParams();
    void computeBlur();
};

class ThunderGazeV5 final : public ThunderGazeV4 {
    Q_OBJECT

public:
    explicit ThunderGazeV5(QObject* parent, QUdpSocket* socket);
    qsizetype messageSize() const override;
    void setVideoSaverEnabled(bool enabled) override;
    bool parseMessage(const QByteArray& message,
                      qlonglong receiveTime,
                      const QHostAddress& sender,
                      quint16 senderPort) override;

private slots:
    void applyTelemetryParams();
    void computeBlur();
};

class ThunderGazeFactory {
public:
    static ThunderGaze* create(float version,
                               QObject* parent,
                               QUdpSocket* socket);
};

