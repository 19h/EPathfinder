#include "mavlink/log_handler.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QTimer>

#include <algorithm>

namespace {

constexpr std::uint32_t kLogChunkSize = 90U;

} // namespace

LogHandler::LogHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    QTimer::singleShot(100, this, &LogHandler::initIntervals);
}

bool LogHandler::isRunning() const
{
    return running_;
}

std::uint32_t LogHandler::downloaded() const
{
    return downloaded_;
}

std::uint32_t LogHandler::remaining() const
{
    return remaining_;
}

std::uint16_t LogHandler::loadingId() const
{
    return loadingId_;
}

void LogHandler::initIntervals()
{
}

QString LogHandler::fileName(const LogItem& item) const
{
    return QStringLiteral("%1_%2_%3")
        .arg(item.id)
        .arg(item.time)
        .arg(item.size);
}

QString LogHandler::filePath(const LogItem& item) const
{
    return logDirectory_ + QLatin1Char('/') + fileName(item);
}

void LogHandler::getLogPart()
{
    mavlink_message_t message{};
    const std::uint8_t systemId = communicator_->systemId();
    const std::uint8_t componentId = communicator_->componentId();
    const std::uint32_t count = std::min(remaining_, kLogChunkSize);
    mavlink_msg_log_request_data_pack(systemId,
                                      MAV_COMP_ID_ONBOARD_COMPUTER,
                                      &message,
                                      systemId,
                                      componentId,
                                      loadingId_,
                                      downloaded_,
                                      count);
    communicator_->sendMessageOnMainLink(message);
}

bool LogHandler::getLogList()
{
    if (running_) {
        return false;
    }

    running_ = true;
    items_.clear();
    reservedListState_ = 0U;

    mavlink_message_t message{};
    const std::uint8_t systemId = communicator_->systemId();
    const std::uint8_t componentId = communicator_->componentId();
    mavlink_msg_log_request_list_pack(systemId,
                                      MAV_COMP_ID_ONBOARD_COMPUTER,
                                      &message,
                                      systemId,
                                      componentId,
                                      0U,
                                      UINT16_MAX);
    communicator_->sendMessageOnMainLink(message);
    return true;
}

bool LogHandler::getLog(const int id)
{
    if (running_ || id > items_.size()) {
        return false;
    }

    QDir directory(logDirectory_);
    if (!directory.exists()) {
        QDir().mkdir(logDirectory_);
    }

    LogItem& item = items_[id - 1];
    if (item.loaded) {
        QFile file(filePath(item));
        (void)file.open(QIODevice::ReadOnly);
        emit logEvent(file.readAll());
        file.close();
        return true;
    }

    running_ = true;
    currentFileName_ = fileName(item);
    downloaded_ = 0U;
    loadingId_ = static_cast<std::uint16_t>(id);
    remaining_ = item.size;
    getLogPart();
    return true;
}

void LogHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_LOG_ENTRY) {
        mavlink_log_entry_t entry{};
        mavlink_msg_log_entry_decode(&message, &entry);
        qDebug() << "log entry time:" << entry.time_utc
                 << " size:" << entry.size << " id:" << entry.id
                 << " num_logs:" << entry.num_logs;

        if (static_cast<std::uint16_t>(entry.id - 1U) != items_.size()) {
            items_.clear();
            reservedListState_ = 0U;
            emit getLogListError(1);
            running_ = false;
            return;
        }

        items_.append(LogItem{entry.id, entry.time_utc, entry.size, false});
        if (entry.num_logs != items_.size()) {
            return;
        }

        for (LogItem& item : items_) {
            const QString path = filePath(item);
            if (QFile::exists(path)) {
                item.loaded = static_cast<quint64>(item.size)
                    == static_cast<quint64>(QFileInfo(path).size());
            }
        }
        emit logListEvent(items_);
        running_ = false;
        return;
    }

    if (message.msgid != MAVLINK_MSG_ID_LOG_DATA) {
        return;
    }

    mavlink_log_data_t data{};
    mavlink_msg_log_data_decode(&message, &data);
    if (data.id != loadingId_ || data.ofs != downloaded_) {
        emit getLogError(2);
        running_ = false;
        return;
    }

    const std::uint32_t count = std::min(remaining_, kLogChunkSize);
    QFile file(logDirectory_ + QLatin1Char('/') + currentFileName_);
    (void)file.open(QIODevice::WriteOnly | QIODevice::Append);
    file.write(reinterpret_cast<const char*>(data.data), count);
    file.close();

    downloaded_ += count;
    remaining_ -= count;
    if (remaining_ != 0U) {
        getLogPart();
        return;
    }

    // RVA 0x179c54 indexes by the one-based loading identifier without
    // subtracting one. Preserve that original source-level behavior.
    items_[loadingId_].loaded = true;
    (void)file.open(QIODevice::ReadOnly);
    emit logEvent(file.readAll());
    file.close();
    running_ = false;
    downloaded_ = 0U;
    remaining_ = 0U;
}
