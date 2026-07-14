#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class AbstractLink : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isUp READ isUp NOTIFY upChanged)

public:
    explicit AbstractLink(QObject* parent = nullptr);

    virtual bool isUp() const = 0;

signals:
    void upChanged(bool isUp);
    void dataReceived(const QByteArray& data);

public slots:
    virtual void up() = 0;
    virtual void down() = 0;
    virtual void reconnect();
    virtual void sendData(const QByteArray& data) = 0;
    virtual void sendData(const char* data, qlonglong length) = 0;

protected:
    virtual void switchLink();

    // The original base object contains one QString at offset 0x10. No
    // out-of-line symbol exposes its source identifier.
    QString linkName_;

    friend class MavLinkCommunicator;
};
