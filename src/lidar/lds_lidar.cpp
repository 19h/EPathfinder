// Adapted from Livox SDK 2.3.0 sample_cc/lidar/lds_lidar.cpp.

#include "lidar/lds_lidar.hpp"

#include <cstdio>
#include <cstring>
#include <utility>

namespace {

constexpr const char* localBroadcastCodeList[] = {
    "000000000000001",
};

} // namespace

LdsLidar* g_lidars = nullptr;

LdsLidar::LdsLidar()
    : auto_connect_mode_(true)
    , whitelist_count_(0)
    , is_initialized_(false)
    , lidar_count_(0)
{
    std::memset(broadcast_code_whitelist_,
                0,
                sizeof(broadcast_code_whitelist_));
    std::memset(lidars_, 0, sizeof(lidars_));
    std::memset(data_recveive_count_, 0, sizeof(data_recveive_count_));
    for (std::uint32_t index = 0; index < kMaxLidarCount; ++index) {
        lidars_[index].handle = kMaxLidarCount;
        lidars_[index].connect_state = kConnectStateOff;
    }
}

LdsLidar::~LdsLidar() = default;

int LdsLidar::InitLdsLidar(std::vector<std::string>& broadcastCodes)
{
    if (is_initialized_) {
        std::printf("LiDAR data source is already inited!\n");
        return -1;
    }

    if (!Init()) {
        Uninit();
        std::printf("Livox-SDK init fail!\n");
        return -1;
    }

    LivoxSdkVersion sdkVersion{};
    GetLivoxSdkVersion(&sdkVersion);
    std::printf("Livox SDK version %d.%d.%d\n",
                sdkVersion.major,
                sdkVersion.minor,
                sdkVersion.patch);

    SetBroadcastCallback(LdsLidar::OnDeviceBroadcast);
    SetDeviceStateUpdateCallback(LdsLidar::OnDeviceChange);

    for (const auto& input : broadcastCodes) {
        AddBroadcastCodeToWhitelist(input.c_str());
    }
    AddLocalBroadcastCode();

    if (whitelist_count_ != 0U) {
        DisableAutoConnectMode();
        std::printf("Disable auto connect mode!\n");
        std::printf("List all broadcast code in whiltelist:\n");
        for (std::uint32_t index = 0; index < whitelist_count_; ++index) {
            std::printf("%s\n", broadcast_code_whitelist_[index]);
        }
    } else {
        EnableAutoConnectMode();
        std::printf("No broadcast code was added to whitelist, swith to automatic connection mode!\n");
    }

    if (!Start()) {
        Uninit();
        std::printf("Livox-SDK init fail!\n");
        return -1;
    }

    if (g_lidars == nullptr) {
        g_lidars = this;
    }
    is_initialized_ = true;
    std::printf("Livox-SDK init success!\n");
    return 0;
}

int LdsLidar::DeInitLdsLidar()
{
    if (!is_initialized_) {
        std::printf("LiDAR data source is not exit");
        return -1;
    }

    Uninit();
    std::printf("Livox SDK Deinit completely!\n");
    return 0;
}

void LdsLidar::SetQDataCallback(QDataCallback callback)
{
    q_data_callback_ = std::move(callback);
}

void LdsLidar::GetLidarDataCb(const std::uint8_t handle,
                              LivoxEthPacket* const data,
                              const std::uint32_t dataNum,
                              void* const clientData)
{
    auto* const lidar = static_cast<LdsLidar*>(clientData);
    if (data == nullptr || dataNum == 0U || handle >= kMaxLidarCount) {
        return;
    }

    ++lidar->data_recveive_count_[handle];
    if (lidar->data_recveive_count_[handle] % 100U != 0U) {
        return;
    }

    std::printf("receive packet count %d %d\n",
                handle,
                lidar->data_recveive_count_[handle]);

    QList<LivoxPoint> points;
    if (data->data_type == kExtendCartesian) {
        const auto* const rawPoints =
            reinterpret_cast<const LivoxExtendRawPoint*>(data->data);
        for (std::uint32_t index = 0; index < dataNum; ++index) {
            const auto& raw = rawPoints[index];
            if ((raw.x | raw.y | raw.z) == 0) {
                continue;
            }
            points.append(LivoxPoint{
                static_cast<float>(raw.x) / 1000.0F,
                static_cast<float>(raw.y) / 1000.0F,
                static_cast<float>(raw.z) / 1000.0F,
                raw.reflectivity,
            });
        }
    }

    if (!points.isEmpty()) {
        // The binary obtains the singleton here rather than invoking the
        // callback through clientData.
        GetInstance().q_data_callback_(points);
    }
}

void LdsLidar::OnDeviceBroadcast(const BroadcastDeviceInfo* const info)
{
    if (info == nullptr) {
        return;
    }
    if (info->dev_type == kDeviceTypeHub) {
        std::printf("In lidar mode, couldn't connect a hub : %s\n",
                    info->broadcast_code);
        return;
    }

    if (g_lidars->IsAutoConnectMode()) {
        std::printf("In automatic connection mode, will connect %s\n",
                    info->broadcast_code);
    } else if (!g_lidars->FindInWhitelist(info->broadcast_code)) {
        std::printf("Not in the whitelist, please add %s to if want to connect!\n",
                    info->broadcast_code);
        return;
    }

    std::uint8_t handle = 0;
    const livox_status result =
        AddLidarToConnect(info->broadcast_code, &handle);
    if (result == kStatusSuccess && handle < kMaxLidarCount) {
        SetDataCallback(handle, LdsLidar::GetLidarDataCb, g_lidars);
        auto* const lidar = &g_lidars->lidars_[handle];
        lidar->handle = handle;
        lidar->connect_state = kConnectStateOff;
        lidar->config.enable_fan = true;
        lidar->config.return_mode = kStrongestReturn;
        lidar->config.coordinate = kCoordinateCartesian;
        lidar->config.imu_rate = kImuFreq200Hz;
    } else {
        std::printf("Add lidar to connect is failed : %d %d \n",
                    result,
                    handle);
    }
}

void LdsLidar::OnDeviceChange(const DeviceInfo* const info,
                              const DeviceEvent type)
{
    if (info == nullptr || info->handle >= kMaxLidarCount) {
        return;
    }

    const std::uint8_t handle = info->handle;
    auto* const lidar = &g_lidars->lidars_[handle];
    if (type == kEventConnect) {
        QueryDeviceInformation(handle, DeviceInformationCb, g_lidars);
        if (lidar->connect_state == kConnectStateOff) {
            lidar->connect_state = kConnectStateOn;
            lidar->info = *info;
        }
        std::printf("[WARNING] Lidar sn: [%s] Connect!!!\n",
                    info->broadcast_code);
    } else if (type == kEventDisconnect) {
        lidar->connect_state = kConnectStateOff;
        std::printf("[WARNING] Lidar sn: [%s] Disconnect!!!\n",
                    info->broadcast_code);
    } else if (type == kEventStateChange) {
        lidar->info = *info;
        std::printf("[WARNING] Lidar sn: [%s] StateChange!!!\n",
                    info->broadcast_code);
    }

    if (lidar->connect_state != kConnectStateOn) {
        return;
    }

    std::printf("Device Working State %d\n", lidar->info.state);
    if (lidar->info.state == kLidarStateInit) {
        std::printf("Device State Change Progress %u\n",
                    lidar->info.status.progress);
    } else {
        std::printf("Device State Error Code 0X%08x\n",
                    lidar->info.status.status_code.error_code);
    }
    std::printf("Device feature %d\n", lidar->info.feature);
    SetErrorMessageCallback(handle, LdsLidar::LidarErrorStatusCb);

    if (lidar->info.state != kLidarStateNormal) {
        return;
    }

    if (lidar->config.coordinate != 0U) {
        SetSphericalCoordinate(handle, LdsLidar::SetCoordinateCb, g_lidars);
    } else {
        SetCartesianCoordinate(handle, LdsLidar::SetCoordinateCb, g_lidars);
    }
    lidar->config.set_bits |= kConfigCoordinate;

    if (info->type != kDeviceTypeLidarMid40) {
        LidarSetPointCloudReturnMode(
            handle,
            static_cast<PointCloudReturnMode>(lidar->config.return_mode),
            LdsLidar::SetPointCloudReturnModeCb,
            g_lidars);
        lidar->config.set_bits |= kConfigReturnMode;
    }

    if (info->type != kDeviceTypeLidarMid40
        && info->type != kDeviceTypeLidarMid70) {
        LidarSetImuPushFrequency(
            handle,
            static_cast<ImuFreq>(lidar->config.imu_rate),
            LdsLidar::SetImuRatePushFrequencyCb,
            g_lidars);
        lidar->config.set_bits |= kConfigImuRate;
    }
    lidar->connect_state = kConnectStateConfig;
}

void LdsLidar::DeviceInformationCb(const livox_status status,
                                   std::uint8_t,
                                   DeviceInformationResponse* const ack,
                                   void*)
{
    if (status != kStatusSuccess) {
        std::printf("Device Query Informations Failed : %d\n", status);
    }
    if (ack != nullptr) {
        std::printf("firm ver: %d.%d.%d.%d\n",
                    ack->firmware_version[0],
                    ack->firmware_version[1],
                    ack->firmware_version[2],
                    ack->firmware_version[3]);
    }
}

void LdsLidar::LidarErrorStatusCb(livox_status,
                                  const std::uint8_t handle,
                                  ErrorMessage* const message)
{
    static std::uint32_t errorMessageCount = 0;
    if (message == nullptr || ++errorMessageCount % 100U != 0U) {
        return;
    }
    std::printf("handle: %u\n", handle);
    std::printf("temp_status : %u\n", message->lidar_error_code.temp_status);
    std::printf("volt_status : %u\n", message->lidar_error_code.volt_status);
    std::printf("motor_status : %u\n", message->lidar_error_code.motor_status);
    std::printf("dirty_warn : %u\n", message->lidar_error_code.dirty_warn);
    std::printf("firmware_err : %u\n", message->lidar_error_code.firmware_err);
    std::printf("pps_status : %u\n", message->lidar_error_code.device_status);
    std::printf("fan_status : %u\n", message->lidar_error_code.fan_status);
    std::printf("self_heating : %u\n", message->lidar_error_code.self_heating);
    std::printf("ptp_status : %u\n", message->lidar_error_code.ptp_status);
    std::printf("time_sync_status : %u\n",
                message->lidar_error_code.time_sync_status);
    std::printf("system_status : %u\n",
                message->lidar_error_code.system_status);
}

void LdsLidar::ControlFanCb(livox_status, std::uint8_t, std::uint8_t, void*)
{
}

void LdsLidar::SetPointCloudReturnModeCb(const livox_status status,
                                         const std::uint8_t handle,
                                         std::uint8_t,
                                         void* const clientData)
{
    auto* const source = static_cast<LdsLidar*>(clientData);
    if (handle >= kMaxLidarCount) {
        return;
    }
    auto* const lidar = &source->lidars_[handle];
    if (status == kStatusSuccess) {
        lidar->config.set_bits &= ~std::uint32_t(kConfigReturnMode);
        std::printf("Set return mode success!\n");
        if (lidar->config.set_bits == 0U) {
            LidarStartSampling(handle, LdsLidar::StartSampleCb, source);
            lidar->connect_state = kConnectStateSampling;
        }
    } else {
        LidarSetPointCloudReturnMode(
            handle,
            static_cast<PointCloudReturnMode>(lidar->config.return_mode),
            LdsLidar::SetPointCloudReturnModeCb,
            source);
        std::printf("Set return mode fail, try again!\n");
    }
}

void LdsLidar::SetCoordinateCb(const livox_status status,
                               const std::uint8_t handle,
                               std::uint8_t,
                               void* const clientData)
{
    auto* const source = static_cast<LdsLidar*>(clientData);
    if (handle >= kMaxLidarCount) {
        return;
    }
    auto* const lidar = &source->lidars_[handle];
    if (status == kStatusSuccess) {
        lidar->config.set_bits &= ~std::uint32_t(kConfigCoordinate);
        std::printf("Set coordinate success!\n");
        if (lidar->config.set_bits == 0U) {
            LidarStartSampling(handle, LdsLidar::StartSampleCb, source);
            lidar->connect_state = kConnectStateSampling;
        }
    } else {
        if (lidar->config.coordinate != 0U) {
            SetSphericalCoordinate(handle, LdsLidar::SetCoordinateCb, source);
        } else {
            SetCartesianCoordinate(handle, LdsLidar::SetCoordinateCb, source);
        }
        std::printf("Set coordinate fail, try again!\n");
    }
}

void LdsLidar::SetImuRatePushFrequencyCb(const livox_status status,
                                         const std::uint8_t handle,
                                         std::uint8_t,
                                         void* const clientData)
{
    auto* const source = static_cast<LdsLidar*>(clientData);
    if (handle >= kMaxLidarCount) {
        return;
    }
    auto* const lidar = &source->lidars_[handle];
    if (status == kStatusSuccess) {
        lidar->config.set_bits &= ~std::uint32_t(kConfigImuRate);
        std::printf("Set imu rate success!\n");
        if (lidar->config.set_bits == 0U) {
            LidarStartSampling(handle, LdsLidar::StartSampleCb, source);
            lidar->connect_state = kConnectStateSampling;
        }
    } else {
        LidarSetImuPushFrequency(
            handle,
            static_cast<ImuFreq>(lidar->config.imu_rate),
            LdsLidar::SetImuRatePushFrequencyCb,
            source);
        std::printf("Set imu rate fail, try again!\n");
    }
}

void LdsLidar::StartSampleCb(const livox_status status,
                             const std::uint8_t handle,
                             const std::uint8_t response,
                             void* const clientData)
{
    auto* const source = static_cast<LdsLidar*>(clientData);
    if (handle >= kMaxLidarCount) {
        return;
    }
    auto* const lidar = &source->lidars_[handle];
    if (status == kStatusSuccess) {
        if (response != 0U) {
            lidar->connect_state = kConnectStateOn;
            std::printf("Lidar start sample fail : state[%d] handle[%d] res[%d]\n",
                        status,
                        handle,
                        response);
        } else {
            std::printf("Lidar start sample success\n");
        }
    } else if (status == kStatusTimeout) {
        lidar->connect_state = kConnectStateOn;
        std::printf("Lidar start sample timeout : state[%d] handle[%d] res[%d]\n",
                    status,
                    handle,
                    response);
    }
}

void LdsLidar::StopSampleCb(livox_status, std::uint8_t, std::uint8_t, void*)
{
}

int LdsLidar::AddBroadcastCodeToWhitelist(const char* const broadcastCode)
{
    if (broadcastCode == nullptr
        || std::strlen(broadcastCode) > kBroadcastCodeSize
        || whitelist_count_ >= kMaxLidarCount) {
        return -1;
    }
    if (FindInWhitelist(broadcastCode)) {
        std::printf("%s is alrealy exist!\n", broadcastCode);
        return -1;
    }

    std::strcpy(broadcast_code_whitelist_[whitelist_count_], broadcastCode);
    ++whitelist_count_;
    return 0;
}

void LdsLidar::AddLocalBroadcastCode()
{
    for (const char* const broadcastCode : localBroadcastCodeList) {
        const std::string invalidBroadcastCode = "000000000";
        std::printf("Local broadcast code : %s\n", broadcastCode);
        if (kBroadcastCodeSize == std::strlen(broadcastCode) + 1U
            && std::strstr(broadcastCode, invalidBroadcastCode.c_str())
                == nullptr) {
            AddBroadcastCodeToWhitelist(broadcastCode);
        } else {
            std::printf("Invalid local broadcast code : %s\n", broadcastCode);
        }
    }
}

bool LdsLidar::FindInWhitelist(const char* const broadcastCode)
{
    if (broadcastCode == nullptr) {
        return false;
    }
    for (std::uint32_t index = 0; index < whitelist_count_; ++index) {
        if (std::strncmp(broadcastCode,
                         broadcast_code_whitelist_[index],
                         kBroadcastCodeSize)
            == 0) {
            return true;
        }
    }
    return false;
}
