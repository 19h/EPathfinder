#include "camera/camera_t205.hpp"

CameraT205::CameraT205(const QString& device, QObject* parent)
    : CameraV4L(device, parent)
{
    setGimbalStabRoll(true);
    setControlFeatures(true, true);
    // Exact 21-record constructor table recovered from RVA 0x25c90.
    // Record order is optical command, percent, horizontal FOV, vertical FOV,
    // two image-scale corrections, then two calibration-tail scales.
    const struct Calibration {
        float optical;
        float percent;
        float width;
        float height;
        float scaleX;
        float scaleY;
    } values[] = {
        {0.0F, 0.0F, 60.0F, 32.3F, 1.0167F, 1.065F},
        {37.0F, 5.0F, 58.0F, 31.4F, 1.0F, 1.0F},
        {74.0F, 10.0F, 56.0F, 30.6F, 1.0F, 1.0F},
        {110.0F, 15.0F, 54.0F, 29.7F, 1.0F, 1.0F},
        {147.0F, 20.0F, 52.0F, 28.9F, 1.0F, 1.0F},
        {184.0F, 25.0F, 50.0F, 28.0F, 1.0F, 1.0F},
        {221.0F, 30.0F, 46.8F, 26.2F, 1.0F, 1.0F},
        {258.0F, 35.0F, 43.6F, 24.4F, 1.0F, 1.0F},
        {294.0F, 40.0F, 40.4F, 22.6F, 1.0F, 1.0F},
        {331.0F, 45.0F, 37.2F, 20.8F, 1.0F, 1.0F},
        {368.0F, 50.0F, 34.0F, 19.0F, 1.0F, 1.0F},
        {405.0F, 55.0F, 30.4F, 17.0F, 1.0F, 1.0F},
        {442.0F, 60.0F, 26.8F, 15.0F, 1.0F, 1.0F},
        {478.0F, 65.0F, 23.2F, 13.0F, 1.0F, 1.0F},
        {515.0F, 70.0F, 19.6F, 11.0F, 1.0F, 1.0F},
        {552.0F, 75.0F, 16.0F, 9.0F, 1.0F, 1.0F},
        {589.0F, 80.0F, 14.6F, 8.2F, 1.0F, 1.0F},
        {626.0F, 85.0F, 13.2F, 7.4F, 1.0F, 1.0F},
        {662.0F, 90.0F, 11.8F, 6.6F, 1.0F, 1.0F},
        {699.0F, 95.0F, 10.4F, 5.8F, 1.0F, 1.0F},
        {736.0F, 100.0F, 9.0F, 5.0F, 1.0F, 1.0F},
    };
    for (const Calibration& item : values) {
        Zoom zoom{item.optical,
                                       item.percent,
                                       item.width,
                                       item.height,
                                       item.scaleX,
                                       item.scaleY,
                                       0};
        zoom.calibration.scaleX = 1.0F;
        zoom.calibration.scaleY = 1.0F;
        calibrationZooms().append(zoom);
    }
}
