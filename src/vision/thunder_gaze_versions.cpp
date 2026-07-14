#include "vision/thunder_gaze.hpp"

namespace {

bool validateOnly(const ThunderGaze& gaze, const QByteArray& message)
{
    return message.size() == gaze.messageSize();
}

} // namespace

ThunderGazeV1::ThunderGazeV1(QObject* parent, QUdpSocket* socket)
    : ThunderGaze(parent, socket)
{
}

qsizetype ThunderGazeV1::messageSize() const { return 0x330; }

bool ThunderGazeV1::parseMessage(const QByteArray& message,
                                 qlonglong,
                                 const QHostAddress&,
                                 quint16)
{
    return validateOnly(*this, message);
}

ThunderGazeV2::ThunderGazeV2(QObject* parent, QUdpSocket* socket)
    : ThunderGaze(parent, socket)
{
}

qsizetype ThunderGazeV2::messageSize() const { return 0xfe30; }

void ThunderGazeV2::setPreprocessingScale(const float value)
{
    preprocessingScale_ = value;
    emit preprocessingScaleChanged(value);
}

bool ThunderGazeV2::parseMessage(const QByteArray& message,
                                 qlonglong,
                                 const QHostAddress&,
                                 quint16)
{
    return validateOnly(*this, message);
}

ThunderGazeV3::ThunderGazeV3(QObject* parent, QUdpSocket* socket)
    : ThunderGaze(parent, socket)
{
}

qsizetype ThunderGazeV3::messageSize() const { return 0xfdf0; }

void ThunderGazeV3::setPreprocessingScale(const float value)
{
    preprocessingScale_ = value;
    emit preprocessingScaleChanged(value);
}

void ThunderGazeV3::setFrameText(const QStringList& lines)
{
    frameText_ = lines;
}

bool ThunderGazeV3::parseMessage(const QByteArray& message,
                                 qlonglong,
                                 const QHostAddress&,
                                 quint16)
{
    return validateOnly(*this, message);
}

ThunderGazeV4::ThunderGazeV4(QObject* parent, QUdpSocket* socket)
    : ThunderGaze(parent, socket)
{
    reductionRows_ = 40;
    reductionColumns_ = 60;
}

qsizetype ThunderGazeV4::messageSize() const { return 0xfdf0; }

void ThunderGazeV4::setPreprocessingScale(const float value)
{
    preprocessingScale_ = value;
    emit preprocessingScaleChanged(value);
}

void ThunderGazeV4::setFrameText(const QStringList& lines)
{
    frameText_ = lines;
}

void ThunderGazeV4::setRTSPEnabled(const bool enabled)
{
    if (enabled) {
        emit rtspEnabled();
    } else {
        emit rtspDisabled();
    }
}

void ThunderGazeV4::setTelemetryParams(const QJsonDocument& params)
{
    telemetryParams_ = params;
    applyTelemetryParams();
}

void ThunderGazeV4::setObjectDetectorEnabled(const bool enabled)
{
    emit objectDetectorEnabled(enabled);
}

void ThunderGazeV4::setPathfinderEnabled(const bool enabled)
{
    emit pathfinderEnabled(enabled);
}

bool ThunderGazeV4::parseMessage(const QByteArray& message,
                                 qlonglong,
                                 const QHostAddress&,
                                 quint16)
{
    return validateOnly(*this, message);
}

void ThunderGazeV4::applyTelemetryParams() {}
void ThunderGazeV4::computeBlur() {}

ThunderGazeV5::ThunderGazeV5(QObject* parent, QUdpSocket* socket)
    : ThunderGazeV4(parent, socket)
{
}

qsizetype ThunderGazeV5::messageSize() const { return 0xfdec; }

void ThunderGazeV5::setVideoSaverEnabled(const bool enabled)
{
    emit videoSaverEnabled(enabled);
}

bool ThunderGazeV5::parseMessage(const QByteArray& message,
                                 qlonglong,
                                 const QHostAddress&,
                                 quint16)
{
    return validateOnly(*this, message);
}

void ThunderGazeV5::applyTelemetryParams() {}
void ThunderGazeV5::computeBlur() {}

