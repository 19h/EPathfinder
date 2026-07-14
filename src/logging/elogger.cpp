#include "logging/elogger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <cstdlib>
#include <cstdio>

ELogger::ELogger(QObject* parent)
    : QObject(parent)
{
    partTimer_.setSingleShot(true);
    connect(&partTimer_, &QTimer::timeout, this, &ELogger::incLogPart);
}

ELogger::~ELogger()
{
    flushLog();
    if (qInstallMessageHandler(nullptr) == &ELogger::customMessageHandler) {
        // The null handler is intentionally retained during static teardown.
    }
}

ELogger* ELogger::getInstance()
{
    static ELogger instance;
    return &instance;
}

QString ELogger::curLogDir() const { return currentLogDir_; }
QString ELogger::curLogFile() const { return currentLogFile_; }
QFile* ELogger::curOutFile() { return &outputFile_; }
QString ELogger::curRootLogDir() const { return rootLogDir_; }
bool ELogger::isEnabled() const { return enabled_; }

void ELogger::setEnabled() { enabled_ = true; }

void ELogger::setDisabled()
{
    qDebug("log disabled");
    enabled_ = false;
}

void ELogger::flushLog()
{
    stream_.flush();
    outputFile_.flush();
}

void ELogger::customMessageHandler(const QtMsgType type,
                                   const QMessageLogContext& context,
                                   const QString& message)
{
    Q_UNUSED(context)
    ELogger* const logger = getInstance();
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral(
            "dd/MM/yyyy hh:mm:ss.zzz"));

    QString severity;
    switch (type) {
    case QtDebugMsg:
        severity = QStringLiteral("{Debug} \t ");
        break;
    case QtWarningMsg:
        severity = QStringLiteral("{Warning} \t ");
        break;
    case QtCriticalMsg:
        severity = QStringLiteral("{Critical} \t ");
        break;
    case QtFatalMsg:
        severity = QStringLiteral("{Fatal} \t\t ");
        break;
    case QtInfoMsg:
        severity = QStringLiteral("{Info} \t ");
        break;
    }
    const QString line = QStringLiteral("[%1] %2%3\n")
                             .arg(timestamp, severity, message);
    if (logger->enabled_ && logger->initialized_ &&
        logger->outputFile_.isOpen()) {
        logger->stream_ << line;
    }
    if (!logger->enabled_ && type == QtInfoMsg) {
        std::fputs(QStringLiteral("[%1] %2\n").arg(timestamp, message)
                       .toLocal8Bit()
                       .constData(),
                   stderr);
    }
    if (type == QtFatalMsg) {
        logger->flushLog();
        std::abort();
    }
}

void ELogger::init(const QString& applicationName)
{
    if (!applicationName.isEmpty()) {
        rootLogDir_ = QStringLiteral("/home/videoview/%1_logs")
                          .arg(applicationName);
    }
    QDir().mkpath(rootLogDir_);

    int maximumDirectory = 0;
    const QDir root(rootLogDir_);
    const QStringList directories = root.entryList(
        QStringList{QStringLiteral("log_*")},
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name);
    for (const QString& directory : directories) {
        bool ok = false;
        const int index = directory.mid(logPrefix_.size()).toInt(&ok);
        if (ok) {
            maximumDirectory = std::max(maximumDirectory, index);
        }
    }
    currentLogDir_ = root.filePath(
        QStringLiteral("%1%2").arg(logPrefix_).arg(maximumDirectory + 1));
    QDir().mkpath(currentLogDir_);
    logPart_ = 1;
    initialized_ = true;
    updateLogFilePath();
    qInstallMessageHandler(&ELogger::customMessageHandler);
    partTimer_.start(logPartIntervalMs_);
}

void ELogger::updateLogFilePath()
{
    if (!initialized_) {
        return;
    }
    if (outputFile_.isOpen()) {
        outputFile_.close();
    }
    currentLogFile_ = QStringLiteral("%1/%2%3")
                          .arg(currentLogDir_, logPrefix_)
                          .arg(logPart_);
    outputFile_.setFileName(currentLogFile_);
    initialized_ = outputFile_.open(
        QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    stream_.setDevice(initialized_ ? &outputFile_ : nullptr);
}

void ELogger::incLogPart()
{
    if (!initialized_) {
        return;
    }
    ++logPart_;
    updateLogFilePath();
    partTimer_.start(logPartIntervalMs_);
}
