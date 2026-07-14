// Adapted from the Livox SDK 2.3.0 sample_cc/lidar implementation.
// The upstream sample is MIT licensed; see vendor/Livox-SDK/LICENSE.txt.

#pragma once

#include <QList>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <livox_def.h>
#include <livox_sdk.h>

enum LidarConnectState {
    kConnectStateOff = 0,
    kConnectStateOn = 1,
    kConnectStateConfig = 2,
    kConnectStateSampling = 3,
};

enum LidarConfigCodeBit {
    kConfigFan = 1,
    kConfigReturnMode = 2,
    kConfigCoordinate = 4,
    kConfigImuRate = 8,
};

enum CoordinateType {
    kCoordinateCartesian = 0,
    kCoordinateSpherical,
};

struct UserConfig {
    bool enable_fan;
    std::uint32_t return_mode;
    std::uint32_t coordinate;
    std::uint32_t imu_rate;
    volatile std::uint32_t set_bits;
    volatile std::uint32_t get_bits;
};

struct LidarDevice {
    std::uint8_t handle;
    LidarConnectState connect_state;
    DeviceInfo info;
    UserConfig config;
};

class LdsLidar {
public:
    using QDataCallback = std::function<void(const QList<LivoxPoint>&)>;

    static LdsLidar& GetInstance()
    {
        static LdsLidar ldsLidar;
        return ldsLidar;
    }

    int InitLdsLidar(std::vector<std::string>& broadcastCodes);
    int DeInitLdsLidar();
    void SetQDataCallback(QDataCallback callback);

private:
    friend struct LdsLidarTestProbe;

    LdsLidar();
    LdsLidar(const LdsLidar&) = delete;
    ~LdsLidar();
    LdsLidar& operator=(const LdsLidar&) = delete;

    static void GetLidarDataCb(std::uint8_t handle,
                               LivoxEthPacket* data,
                               std::uint32_t dataNum,
                               void* clientData);
    static void OnDeviceBroadcast(const BroadcastDeviceInfo* info);
    static void OnDeviceChange(const DeviceInfo* info, DeviceEvent type);
    static void StartSampleCb(livox_status status,
                              std::uint8_t handle,
                              std::uint8_t response,
                              void* clientData);
    static void StopSampleCb(livox_status status,
                             std::uint8_t handle,
                             std::uint8_t response,
                             void* clientData);
    static void DeviceInformationCb(livox_status status,
                                    std::uint8_t handle,
                                    DeviceInformationResponse* ack,
                                    void* clientData);
    static void LidarErrorStatusCb(livox_status status,
                                   std::uint8_t handle,
                                   ErrorMessage* message);
    static void ControlFanCb(livox_status status,
                             std::uint8_t handle,
                             std::uint8_t response,
                             void* clientData);
    static void SetPointCloudReturnModeCb(livox_status status,
                                          std::uint8_t handle,
                                          std::uint8_t response,
                                          void* clientData);
    static void SetCoordinateCb(livox_status status,
                                std::uint8_t handle,
                                std::uint8_t response,
                                void* clientData);
    static void SetImuRatePushFrequencyCb(livox_status status,
                                          std::uint8_t handle,
                                          std::uint8_t response,
                                          void* clientData);

    int AddBroadcastCodeToWhitelist(const char* broadcastCode);
    void AddLocalBroadcastCode();
    bool FindInWhitelist(const char* broadcastCode);

    void EnableAutoConnectMode() { auto_connect_mode_ = true; }
    void DisableAutoConnectMode() { auto_connect_mode_ = false; }
    bool IsAutoConnectMode() const { return auto_connect_mode_; }

    bool auto_connect_mode_;
    std::uint32_t whitelist_count_;
    volatile bool is_initialized_;
    char broadcast_code_whitelist_[kMaxLidarCount][kBroadcastCodeSize];

    std::uint32_t lidar_count_;
    LidarDevice lidars_[kMaxLidarCount];
    std::uint32_t data_recveive_count_[kMaxLidarCount];
    QDataCallback q_data_callback_;
};

