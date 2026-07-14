#include "vision/thunder_gaze.hpp"

ThunderGaze* ThunderGazeFactory::create(const float version,
                                        QObject* const parent,
                                        QUdpSocket* const socket)
{
    if (version >= 5.0F) {
        return new ThunderGazeV5(parent, socket);
    }
    if (version >= 4.0F) {
        return new ThunderGazeV4(parent, socket);
    }
    if (version >= 3.0F) {
        return new ThunderGazeV3(parent, socket);
    }
    if (version < 2.0F) {
        return new ThunderGazeV1(parent, socket);
    }
    return new ThunderGazeV2(parent, socket);
}

