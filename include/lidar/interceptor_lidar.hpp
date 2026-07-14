#pragma once

#include <QList>
#include <QMetaType>
#include <QObject>

struct PlaneCoord {
    float x{};
    float y{};
    float z{};
};

Q_DECLARE_METATYPE(PlaneCoord)
Q_DECLARE_METATYPE(QList<PlaneCoord>)

class InterceptorLidar : public QObject {
    Q_OBJECT

public:
    explicit InterceptorLidar(QObject* parent = nullptr);
    ~InterceptorLidar() override = default;

signals:
    void lidarData(const QList<PlaneCoord>& objects);
};

