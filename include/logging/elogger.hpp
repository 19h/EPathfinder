#pragma once

#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QTimer>

class ELogger final : public QObject {
    Q_OBJECT

public:
    explicit ELogger(QObject* parent = nullptr);
    ~ELogger() override;

    static ELogger* getInstance();
    static void customMessageHandler(QtMsgType type,
                                     const QMessageLogContext& context,
                                     const QString& message);

    void init(const QString& applicationName = {});
    QString curLogDir() const;
    QString curLogFile() const;
    QFile* curOutFile();
    QString curRootLogDir() const;
    bool isEnabled() const;

public slots:
    void setEnabled();
    void setDisabled();
    void flushLog();

private slots:
    void incLogPart();

private:
    void updateLogFilePath();

    int logPartIntervalMs_{60000};
    QString rootLogDir_{QStringLiteral("/home/videoview/EPathfinder_logs")};
    QString logPrefix_{QStringLiteral("log_")};
    int logPart_{1};
    QString currentLogDir_;
    QString currentLogFile_;
    QFile outputFile_;
    QFile rootStateFile_;
    QTextStream stream_;
    bool enabled_{true};
    bool initialized_{true};
    QTimer partTimer_;
};

