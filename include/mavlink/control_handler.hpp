#pragma once

#include "mavlink/abstract_handler.hpp"

#include <QString>

#include <cstdint>

class QTimerEvent;

class ControlHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit ControlHandler(MavLinkCommunicator* communicator, int interval);

    unsigned char currentMountControlCenterOnValue() const;
    unsigned char currentMountControlCenterOffValue() const;

signals:
    void startMagCalAck(unsigned char result);
    void acceptMagCalAck(unsigned char result);
    void cancelMagCalAck(unsigned char result);
    void levelCalAck(unsigned char result);
    void magCalCompletionPercentage(unsigned char value);
    void magCalStatus(unsigned char value);
    void rpmValues(int rpm1, int rpm2);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;

    void takeoff();
    void arm(bool value = true);
    void setMode(int mode);
    void setControlParams(int roll, int yaw, int pitch, int throttle);
    void setFlapsEnabled(bool enabled);
    void setFlapsValue(int value);
    void controlOn(int mode = 5);
    void controlOff();
    void setCameraParams(int pitch);
    void setParachuteEnabled(bool enabled);
    void setPTZCameraEnabled(bool enabled);
    void setMinelayerEnabled(bool enabled);
    void parachuteOn();
    void cameraOn();
    void cameraOff();
    void ptzCameraOn();
    void ptzCameraOff();
    void burnRocket();
    void separateRocketStage();
    void setEPTZ(float roll,
                 float pitch,
                 float yaw,
                 float rollMaxSpeed,
                 float pitchMaxSpeed,
                 float yawMaxSpeed,
                 bool center = false,
                 bool yawByNorth = false,
                 unsigned char gimbalId = 154U);
    void writeParam(const QString& name, int value);
    void reboot();
    void startMagCal();
    void acceptMagCal();
    void cancelMagCal();
    void startLevelCalibration();
    void setThrottleMinValue(unsigned short value);
    void setMountControlCenterOnValue(unsigned char value);
    void setMountControlCenterOffValue(unsigned char value);
    void enableRpmControl(bool enable);
    void startRpmControl();
    void stopRpmControl();
    void enableSpyCamera(bool enabled);
    void openSpyCamera();
    void closeSpyCamera();
    void setBackupControllerEnabled(bool value);
    void setBackupSystemId(int value);
    void setBackupControllerInvertPitch(bool isInvert);
    void setBackupControllerDropEnabled(bool enabled);
    void startBackupControllerDrop();
    void dropMinelayer();
    void setReverseThrottleEnabled(bool enabled);
    void setNormalThrottle();
    void setReverseThrottle();

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void sendChanValues();
    void setRelay(int id, bool value);
    void mavCmdDoSet(uint16_t command,
                     float p1 = 0.0F,
                     float p2 = 0.0F,
                     float p3 = 0.0F,
                     float p4 = 0.0F,
                     float p5 = 0.0F,
                     float p6 = 0.0F,
                     float p7 = 0.0F);

private:
    void sendPrimaryOverride(std::uint16_t chan1,
                             std::uint16_t chan2,
                             std::uint16_t chan3,
                             std::uint16_t chan4,
                             std::uint16_t chan6,
                             std::uint16_t chan7,
                             std::uint16_t chan12);
    void sendBackupOverride(std::uint16_t chan1,
                            std::uint16_t chan2,
                            std::uint16_t chan3,
                            std::uint16_t chan4,
                            std::uint16_t chan12);

    bool controlEnabled_{};
    std::uint16_t roll_{1500U};
    std::uint16_t yaw_{1500U};
    std::uint16_t pitch_{1500U};
    std::uint16_t throttle_{1500U};
    std::uint16_t throttleMin_{988U};
    bool flapsEnabled_{};
    std::uint16_t flapsValue_{1000U};
    std::uint16_t parachuteValue_{1000U};
    std::uint16_t ptzValue_{2000U};
    float eptzRoll_{};
    float eptzPitch_{};
    float eptzYaw_{};
    bool eptzCenter_{};
    bool eptzCommandSuppressed_{};
    bool parachuteEnabled_{};
    bool ptzCameraEnabled_{};
    unsigned char mountControlCenterOnValue_{1U};
    unsigned char mountControlCenterOffValue_{2U};
    bool rpmControlEnabled_{};
    bool rpmControlActive_{};
    bool minelayerEnabled_{};
    std::uint16_t minelayerValue_{1000U};
    bool backupControllerEnabled_{};
    unsigned char backupSystemId_{1U};
    bool backupControllerInvertPitch_{};
    bool magCalCompletionPending_{};
    unsigned char magCalCompletion_{};
    qint64 lastMagCalCompletionTime_{};
    qint64 magCalCompletionInterval_{500};
    bool magCalStatusPending_{};
    unsigned char magCalStatusValue_{};
    qint64 lastMagCalStatusTime_{};
    qint64 magCalStatusInterval_{500};
    bool backupControllerDropEnabled_{};
    qint64 backupControllerDropStartTime_{};
    bool spyCameraEnabled_{};
    bool spyCameraOpen_{};
    bool reverseThrottleEnabled_{};
    bool reverseThrottle_{};
};
