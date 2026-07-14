#pragma once

#include "mavlink/abstract_handler.hpp"

#include <QByteArray>
#include <QList>
#include <QString>

#include <cstddef>
#include <cstdint>

struct LogItem {
    std::uint16_t id{};
    std::uint32_t time{};
    std::uint32_t size{};
    bool loaded{};
};

static_assert(offsetof(LogItem, id) == 0U);
static_assert(offsetof(LogItem, time) == 4U);
static_assert(offsetof(LogItem, size) == 8U);
static_assert(offsetof(LogItem, loaded) == 12U);
static_assert(sizeof(LogItem) == 16U);

class LogHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit LogHandler(MavLinkCommunicator* communicator);

    bool isRunning() const;
    std::uint32_t downloaded() const;
    std::uint32_t remaining() const;
    std::uint16_t loadingId() const;

    bool getLogList();
    bool getLog(int id);

signals:
    void logListEvent(const QList<LogItem>& items);
    void getLogListError(int code);
    void logEvent(const QByteArray& data);
    void getLogError(int code);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;

private:
    void getLogPart();
    QString fileName(const LogItem& item) const;
    QString filePath(const LogItem& item) const;

    QList<LogItem> items_;
    std::uint16_t reservedListState_{};
    bool running_{};
    QString currentFileName_;
    std::uint16_t loadingId_{};
    QString logDirectory_{QStringLiteral("mavlink_logs")};
    std::uint32_t downloaded_{};
    std::uint32_t remaining_{};
};
