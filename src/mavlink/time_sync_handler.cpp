#include "mavlink/time_sync_handler.hpp"

#include <QDateTime>
#include <QDebug>
#include <QTimer>

TimeSyncHandler::TimeSyncHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    QTimer::singleShot(5000, this, &TimeSyncHandler::updateDeltaOffset);
}

void TimeSyncHandler::initIntervals()
{
}

void TimeSyncHandler::startSync()
{
    if (syncing_) {
        qDebug() << "sync time in progress";
        return;
    }

    offsetSum_ = 0;
    roundTripSum_ = 0;
    sampleCount_ = 0;
    syncing_ = true;
    qDebug() << "start sync timers";
    QTimer::singleShot(1, this, &TimeSyncHandler::sendSyncMessage);
}

void TimeSyncHandler::reset()
{
    if (syncing_) {
        resetPending_ = true;
        QTimer::singleShot(100, this, &TimeSyncHandler::reset);
        return;
    }

    synchronizationCount_ = 0;
    startSync();
}

void TimeSyncHandler::sendSyncMessage()
{
    mavlink_message_t message{};
    const uint8_t systemId = communicator_->systemId();
    const qint64 localTimeNanoseconds =
        1000000LL * QDateTime::currentMSecsSinceEpoch();
    mavlink_msg_timesync_pack(systemId,
                              MAV_COMP_ID_ONBOARD_COMPUTER,
                              &message,
                              0,
                              localTimeNanoseconds,
                              0,
                              0);
    communicator_->sendMessageOnMainLink(message);
}

void TimeSyncHandler::sendTimeOffset(const qint64 offset, const int roundDt)
{
    if (emittedOffset_ == offset && emittedRoundTrip_ == roundDt) {
        return;
    }
    emittedOffset_ = offset;
    emittedRoundTrip_ = roundDt;
    emit updateTimeOffset(offset, roundDt);
}

void TimeSyncHandler::updateDeltaOffset()
{
    if (!syncing_ && deltaTime_ != 0 && offsetSum_ != 0) {
        const qint64 elapsed =
            QDateTime::currentMSecsSinceEpoch() - lastSyncWallTime_;
        if (elapsed > 0) {
            const float extrapolated =
                (static_cast<float>(elapsed) / static_cast<float>(deltaTime_))
                * static_cast<float>(deltaOffset_);
            sendTimeOffset(offsetSum_ + static_cast<int>(extrapolated),
                           roundTripSum_);
        }
    }
    QTimer::singleShot(5000, this, &TimeSyncHandler::updateDeltaOffset);
}

void TimeSyncHandler::processMessage(const mavlink_message_t& message)
{
    if (resetPending_) {
        offsetSum_ = 0;
        roundTripSum_ = 0;
        sampleCount_ = 0;
        syncing_ = false;
        resetPending_ = false;
        return;
    }
    if (sampleCount_ == sampleTarget_ || !syncing_
        || message.msgid != MAVLINK_MSG_ID_TIMESYNC) {
        return;
    }

    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    mavlink_timesync_t timesync{};
    mavlink_msg_timesync_decode(&message, &timesync);
    if (timesync.tc1 > 0) {
        const qint64 roundTrip = currentTime - timesync.ts1 / 1000000LL;
        if (roundTrip > 0 && roundTrip <= 399) {
            const qint64 offset =
                (timesync.ts1 - timesync.tc1) / 1000000LL
                - roundTrip / 2;
            offsetSum_ += offset;
            roundTripSum_ += static_cast<int>(roundTrip);
            qDebug() << "time offset current" << offset << "round dt:" << roundTrip;
            ++sampleCount_;
        }
    }

    if (sampleCount_ < sampleTarget_) {
        QTimer::singleShot(1, this, &TimeSyncHandler::sendSyncMessage);
        return;
    }

    offsetSum_ /= sampleCount_;
    roundTripSum_ /= sampleCount_;
    sendTimeOffset(offsetSum_, roundTripSum_);
    syncing_ = false;

    if (lastSyncWallTime_ > 0) {
        deltaOffset_ = static_cast<int>(offsetSum_ - lastOffset_);
        deltaTime_ = static_cast<int>(currentTime - lastSyncWallTime_);
        qDebug() << "delta offset:" << deltaOffset_ << " in " << deltaTime_;
    }
    lastSyncWallTime_ = currentTime;
    lastOffset_ = offsetSum_;
    ++synchronizationCount_;

    int nextSynchronizationDelay = 60000;
    if (synchronizationCount_ <= 6) {
        switch (synchronizationCount_) {
        case 3:
            nextSynchronizationDelay = 10000;
            break;
        case 4:
            nextSynchronizationDelay = 30000;
            break;
        case 5:
            nextSynchronizationDelay = 40000;
            break;
        case 6:
            nextSynchronizationDelay = 50000;
            break;
        default:
            nextSynchronizationDelay = 20000;
            break;
        }
    }
    QTimer::singleShot(nextSynchronizationDelay,
                       this,
                       &TimeSyncHandler::startSync);
}
