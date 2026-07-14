#pragma once

#include "lidar/interceptor_lidar.hpp"

#include <QList>

#include <string>
#include <vector>

#include <livox_def.h>

class InterceptorLivox : public InterceptorLidar {
    Q_OBJECT

public:
    explicit InterceptorLivox(QObject* parent = nullptr);
    ~InterceptorLivox() override = default;

    int frameTime() const;

public slots:
    virtual void Init();
    void setFrameTime(int frameTime);

private:
    friend struct InterceptorLivoxTestProbe;

    void livoxDataCallback(const QList<LivoxPoint>& points);

    std::vector<std::string> broadcast_codes_;
    int frame_time_;
    float cluster_distance_squared_;
};

