# Recovered Qt meta-object interfaces

Recovered 57 meta-objects and 819 methods, including 284 signals.

## `AbstractHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void initIntervals()` | `0xa` |
| slot | protected | `void processMessage(mavlink_message_t message)` | `0x9` |
| slot | protected | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0x9` |

## `AbstractLink`

Properties:

| Name | Type | Flags |
|---|---|---:|
| `isUp` | `bool` | `0x495001` |

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void upChanged(bool isUp)` | `0x6` |
| signal | public | `void dataReceived(QByteArray data)` | `0x6` |
| slot | public | `void up()` | `0xa` |
| slot | public | `void down()` | `0xa` |
| slot | public | `void reconnect()` | `0xa` |
| slot | public | `void sendData(QByteArray data)` | `0xa` |
| slot | public | `void sendData(const char* data, qlonglong len)` | `0xa` |

## `AttitudeHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void attitudeEvent(uint32_t time_boot_ms, float pitch, float yaw, float roll, float pitchSpeed, float yawSpeed, float rollSpeed)` | `0x6` |
| signal | public | `void backupAttitudeEvent(uchar linkId, uint32_t time_boot_ms, float pitch, float yaw, float roll, float pitchSpeed, float yawSpeed, float rollSpeed)` | `0x6` |
| signal | public | `void ahrsError(float rp, float yaw)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0xa` |
| slot | public | `void setAHRS(qlonglong time, float roll, float pitch, float yaw)` | `0xa` |

## `Camera`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void connected()` | `0x6` |
| signal | public | `void disconnected()` | `0x6` |
| signal | public | `void deviceEnabled()` | `0x6` |
| signal | public | `void deviceDisabled()` | `0x6` |
| signal | public | `void paramsChanged(CameraParams params)` | `0x6` |
| signal | public | `void setPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool yawByNorth, uchar gimbalId)` | `0x6` |
| signal | public | `void ahrsByCamera(qlonglong time, float vehicleRoll, float vehiclePitch, float vehicleYaw)` | `0x6` |
| slot | public | `void gimbalValues(float roll, float pitch, float yaw, bool isCenter, qlonglong time)` | `0xa` |
| slot | public | `void gimbalRollMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magRoll)` | `0xa` |
| slot | public | `void gimbalPitchMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magPitch)` | `0xa` |
| slot | public | `void gimbalYawMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magYaw)` | `0xa` |
| slot | public | `void gimbalMagValues(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magRoll, float magPitch, float magYaw)` | `0xa` |
| slot | public | `void setGimbalAHRS(qlonglong time, float gmblRoll, float gmblPitch, float gmblYaw)` | `0xa` |

## `CameraT205`

No class-local signals, slots, or invokable methods.

## `CameraUSB`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void checkDevice()` | `0x8` |

## `CameraV4L`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void initStartZoom100()` | `0x8` |
| slot | private | `void initStartZoom0()` | `0x8` |
| slot | private | `void readCurrentZoom()` | `0x8` |

## `CameraZR10`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void ping()` | `0x8` |
| slot | private | `void pingFinished(int exitCode, QProcess::ExitStatus exitStatus)` | `0x8` |
| slot | private | `void readPendingDatagrams()` | `0x8` |
| slot | private | `void setAutoFocus()` | `0x8` |
| slot | private | `void serialLinkDataReceived(QByteArray data)` | `0x8` |

## `ClientController`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void setNodesMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getNodesMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getStatusMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getExtStatusMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getParamsMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void setParamsMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getVehicleTypesMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void launchMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void unlaunchMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void set5GMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void resetPlanHandlerMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void startExampleControl(QJsonDocument message)` | `0x6` |
| signal | public | `void homingMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void targetMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void testControlMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getCameraScreenshotMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void setCodeMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void uploadSoftwareMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void uploadVisionMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void applySoftwareMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void applyVisionMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void setVisionEnabledMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getLogMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getMavLogListMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getMavLogMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void setZoomCameraParams(QJsonDocument message)` | `0x6` |
| signal | public | `void rebootFC(QJsonDocument message)` | `0x6` |
| signal | public | `void startMagnitometerCalibration(QJsonDocument message)` | `0x6` |
| signal | public | `void acceptMagnitometerCalibration(QJsonDocument message)` | `0x6` |
| signal | public | `void cancelMagnitometerCalibration(QJsonDocument message)` | `0x6` |
| signal | public | `void startLevelCalibration(QJsonDocument message)` | `0x6` |
| signal | public | `void clientVersionMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void setTelemetryParamsMessage(QJsonDocument message)` | `0x6` |
| signal | public | `void getMapBounds(QJsonDocument message)` | `0x6` |
| signal | public | `void getMapCRC(QJsonDocument message)` | `0x6` |
| signal | public | `void poweroff(QJsonDocument message)` | `0x6` |
| signal | public | `void stopCurrentTarget()` | `0x6` |
| signal | public | `void startTestTargetExample()` | `0x6` |
| signal | public | `void setLedState(VehicleLed::Type type, VehicleLed::State state)` | `0x6` |
| signal | public | `void setControlParams(QJsonDocument message)` | `0x6` |
| signal | public | `void takeoff()` | `0x6` |
| signal | public | `void arm(bool value)` | `0x6` |
| signal | public | `void setMode(int mode)` | `0x6` |
| signal | public | `void setExampleMode(int exampleId)` | `0x6` |
| signal | public | `void stopProcMessage()` | `0x6` |
| slot | public | `void sendMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void updateVehicleStatus(VehicleStatus status)` | `0xa` |
| slot | private | `void initServer()` | `0x8` |
| slot | private | `void newConnection()` | `0x8` |
| slot | private | `void readClient()` | `0x8` |
| slot | private | `void clientDisconnected()` | `0x8` |
| slot | private | `void sendVehicleStatus()` | `0x8` |
| slot | private | `void tabletLinkDataReceived(QByteArray msg)` | `0x8` |
| slot | private | `void parseBuffer(QByteArray& data, QTcpSocket* sender)` | `0x8` |
| slot | private | `void parseBuffer(QByteArray& data)` | `0x28` |
| slot | private | `void ckeckSerialLink()` | `0x8` |

## `ControlHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void startMagCalAck(uchar result)` | `0x6` |
| signal | public | `void acceptMagCalAck(uchar result)` | `0x6` |
| signal | public | `void cancelMagCalAck(uchar result)` | `0x6` |
| signal | public | `void levelCalAck(uchar result)` | `0x6` |
| signal | public | `void magCalCompletionPercentage(uchar value)` | `0x6` |
| signal | public | `void magCalStatus(uchar value)` | `0x6` |
| signal | public | `void rpmValues(int rpm1, int rpm2)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void takeoff()` | `0xa` |
| slot | public | `void arm(bool value)` | `0xa` |
| slot | public | `void arm()` | `0x2a` |
| slot | public | `void setMode(int mode)` | `0xa` |
| slot | public | `void setControlParams(int roll, int yaw, int pitch, int throttle)` | `0xa` |
| slot | public | `void setFlapsEnabled(bool enabled)` | `0xa` |
| slot | public | `void setFlapsValue(int value)` | `0xa` |
| slot | public | `void controlOn(int mode)` | `0xa` |
| slot | public | `void controlOn()` | `0x2a` |
| slot | public | `void controlOff()` | `0xa` |
| slot | public | `void setCameraParams(int pitch)` | `0xa` |
| slot | public | `void setParachuteEnabled(bool enabled)` | `0xa` |
| slot | public | `void setPTZCameraEnabled(bool enabled)` | `0xa` |
| slot | public | `void setMinelayerEnabled(bool enabled)` | `0xa` |
| slot | public | `void parachuteOn()` | `0xa` |
| slot | public | `void cameraOn()` | `0xa` |
| slot | public | `void cameraOff()` | `0xa` |
| slot | public | `void ptzCameraOn()` | `0xa` |
| slot | public | `void ptzCameraOff()` | `0xa` |
| slot | public | `void burnRocket()` | `0xa` |
| slot | public | `void separateRocketStage()` | `0xa` |
| slot | public | `void setEPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool yawByNorth, uchar gimbalId)` | `0xa` |
| slot | public | `void setEPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool yawByNorth)` | `0x2a` |
| slot | public | `void setEPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center)` | `0x2a` |
| slot | public | `void setEPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed)` | `0x2a` |
| slot | public | `void writeParam(QString name, int value)` | `0xa` |
| slot | public | `void reboot()` | `0xa` |
| slot | public | `void startMagCal()` | `0xa` |
| slot | public | `void acceptMagCal()` | `0xa` |
| slot | public | `void cancelMagCal()` | `0xa` |
| slot | public | `void startLevelCalibration()` | `0xa` |
| slot | public | `void setThrottleMinValue(ushort value)` | `0xa` |
| slot | public | `void setMountControlCenterOnValue(uchar value)` | `0xa` |
| slot | public | `void setMountControlCenterOffValue(uchar value)` | `0xa` |
| slot | public | `void enableRpmControl(bool enable)` | `0xa` |
| slot | public | `void startRpmControl()` | `0xa` |
| slot | public | `void stopRpmControl()` | `0xa` |
| slot | public | `void enableSpyCamera(bool enabled)` | `0xa` |
| slot | public | `void openSpyCamera()` | `0xa` |
| slot | public | `void closeSpyCamera()` | `0xa` |
| slot | public | `void setBackupControllerEnabled(bool value)` | `0xa` |
| slot | public | `void setBackupSystemId(int value)` | `0xa` |
| slot | public | `void setBackupControllerInvertPitch(bool isInvert)` | `0xa` |
| slot | public | `void setBackupControllerDropEnabled(bool enabled)` | `0xa` |
| slot | public | `void startBackupControllerDrop()` | `0xa` |
| slot | public | `void dropMinelayer()` | `0xa` |
| slot | public | `void setReverseThrottleEnabled(bool enabled)` | `0xa` |
| slot | public | `void setNormalThrottle()` | `0xa` |
| slot | public | `void setReverseThrottle()` | `0xa` |
| slot | private | `void sendChanValues()` | `0x8` |
| slot | private | `void setRelay(int id, bool value)` | `0x8` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2, float p3, float p4, float p5, float p6, float p7)` | `0x8` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2, float p3, float p4, float p5, float p6)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2, float p3, float p4, float p5)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2, float p3, float p4)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2, float p3)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1, float p2)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command, float p1)` | `0x28` |
| slot | private | `void mavCmdDoSet(uint16_t command)` | `0x28` |

## `EInterceptor`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void planeTarget(PlaneTarget target)` | `0x6` |
| signal | public | `void lidarPlaneTarget(QList<PlaneCoord> targets)` | `0x6` |
| slot | private | `void connected()` | `0x8` |
| slot | private | `void disconnected()` | `0x8` |
| slot | private | `void readServer()` | `0x8` |

## `ELinkAbstractHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | protected | `void processMessage(QByteArray message)` | `0x9` |
| slot | protected | `void processMessage()` | `0x9` |

## `ELinkArmHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void armTestOnEvent()` | `0x6` |
| signal | public | `void armTestOffEvent()` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void armOn()` | `0xa` |
| slot | public | `void selfDestruction()` | `0xa` |
| slot | private | `void sendArmMsg()` | `0x8` |

## `ELinkAttitudeHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void attitudeEvent(qlonglong time, float pitch, float roll, float yaw)` | `0x6` |
| signal | public | `void mgCfgStatus(uchar status)` | `0x6` |
| signal | public | `void mgInited()` | `0x6` |
| signal | public | `void mgReseted(uchar rc)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void mgReset()` | `0xa` |

## `ELinkCommunicator`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void messageReceived(QByteArray message)` | `0x6` |
| slot | public | `void addLink(AbstractLink* link)` | `0xa` |
| slot | public | `void removeLink(AbstractLink* link)` | `0xa` |
| slot | public | `void sendMessageOnAllLinks(QByteArray message)` | `0xa` |
| slot | public | `void sendMessage(QByteArray message, AbstractLink* link)` | `0xa` |
| slot | public | `void addMessageTimeDelay(int value)` | `0xa` |
| slot | protected | `void onDataReceived(QByteArray data)` | `0x9` |

## `ELinkFlowerHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void currentAHRS(float roll, float pitch, float yaw, uchar gnss, int lat, int lon, int alt, float hdg, float gs, float as)` | `0x6` |
| signal | public | `void currentGPS(int lat, int lon)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void sendCurrentCoordinate(qlonglong time, int lat, int lon, float conf)` | `0xa` |

## `ELinkGimbalHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void setServoPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool bYawByNorth, int gimbalId)` | `0xa` |
| slot | public | `void setServoPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool bYawByNorth)` | `0x2a` |
| slot | public | `void setServoPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center)` | `0x2a` |
| slot | public | `void setServoPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed)` | `0x2a` |
| slot | private | `void fixRoll()` | `0x8` |
| slot | private | `void unfixRoll()` | `0x8` |
| slot | private | `void setRollFixed(bool fixed)` | `0x8` |
| slot | private | `void setPitch(float angle, bool fixed)` | `0x8` |

## `ELinkInterceptorHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void targetDetected()` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |

## `ELinkMovemenetHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void accelerationEvent(qlonglong time, uint xacc, uint yacc, uint zacc, int temp)` | `0x6` |
| signal | public | `void airspeedRawEvent(qlonglong time, int pressure, int temp)` | `0x6` |
| signal | public | `void airspeedEvent(qlonglong time, float speed)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |

## `ELinkPositionHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void positionEvent(qlonglong time, uchar status, uchar satellites, int lat, int lon, int alt, float vx, float vy, float vz, float groundSpeed, float groundCourse)` | `0x6` |
| signal | public | `void backupPositionEvent(qlonglong time, uchar status, uchar satellites, int lat, int lon, int alt, float vx, float vy, float vz, float groundSpeed, float groundCourse)` | `0x6` |
| signal | public | `void barometricRawEvent(qlonglong time, int pressure, int temp)` | `0x6` |
| signal | public | `void magneticEvent(qlonglong time, int xmag, int ymag, int zmag)` | `0x6` |
| signal | public | `void barometricAltEvent(qlonglong time, int alt)` | `0x6` |
| signal | public | `void distanceSensorEvent(uint minDist, uint maxDist, uint curDist)` | `0x6` |
| signal | public | `void launchAcceleration()` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void setManualAccelerationEvent()` | `0xa` |
| slot | public | `void sendAirspeed(float value)` | `0xa` |

## `ELinkShumodavHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void target(short powerAzim, short powerElev, short angleAzim, short angleElev)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |

## `ELinkStatusHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void batteryVoltageChanged(uint voltage)` | `0x6` |
| signal | public | `void buttonStateChanged(bool pressed)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void setLedState(LedState state)` | `0xa` |
| slot | public | `void getBoardVersion()` | `0xa` |

## `ELinkTelemetry`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void telemetryConnected()` | `0x6` |
| signal | public | `void telemetryDisconnected()` | `0x6` |
| signal | public | `void peekAtGPS()` | `0x6` |
| signal | public | `void gotoLastWP()` | `0x6` |
| signal | public | `void selfDestruct()` | `0x6` |
| signal | public | `void setCurrentPos(qlonglong time, int lat, int lon)` | `0x6` |
| signal | public | `void resetVNav()` | `0x6` |
| signal | public | `void setLastWPCoord(int lat, int lon)` | `0x6` |
| signal | public | `void startSearch(qlonglong time)` | `0x6` |
| signal | public | `void planeTarget(PlaneTarget target)` | `0x6` |
| signal | public | `void remoteControlEnable(uchar mode, int roll, int pitch, int throttle, uchar options)` | `0x6` |
| signal | public | `void remoteControlDisable()` | `0x6` |
| signal | public | `void startManuLand()` | `0x6` |
| signal | public | `void stopManuLand()` | `0x6` |
| signal | public | `void setInterceptorEnabled(bool enabled)` | `0x6` |
| slot | public | `void processMessage()` | `0xa` |
| slot | public | `void sendPosAtt(qlonglong time, int lat, int lon, int alt, float roll, float pitch, float yaw, float airSpeed)` | `0xa` |
| slot | public | `void sendGPSPos(qlonglong time, int lat, int lon, int alt)` | `0xa` |
| slot | public | `void sendTelemetryCameraParams(qlonglong time, float gimbalRoll, float gimbalPitch, float gimbalYaw)` | `0xa` |
| slot | public | `void sendLastWPPos(qlonglong time, int lat, int lon)` | `0xa` |
| slot | public | `void sendRadarEmulationInfo(qlonglong time, int lat, int lon, int alt, float azimuth, float groundSpeed)` | `0xa` |
| slot | public | `void sendPlan(qlonglong time, Plan plan)` | `0xa` |
| slot | public | `void sendGPSStatus(qlonglong time, uchar epStatus, uchar gpsStatus, uchar satellites)` | `0xa` |
| slot | public | `void sendCurrentFlightMode(qlonglong time, uchar mode)` | `0xa` |
| slot | public | `void sendCurrentMode(qlonglong time, uchar mode, ushort trgAlt)` | `0xa` |
| slot | public | `void sendPlanModeInfo(qlonglong time, uchar nextWP, ushort nextWPDist, float da)` | `0xa` |
| slot | public | `void sendFindModeInfo(qlonglong time, uchar dir, ushort centerDist)` | `0xa` |
| slot | public | `void sendTelemetryDistanceInfo(qlonglong time, int battery, int homeDistance, int distanceTraveled)` | `0xa` |
| slot | public | `void sendTargetCorrectionModeInfo(qlonglong time, int trgLat, int trgLon, ushort dtpCount, ushort dist, float da)` | `0xa` |
| slot | public | `void sendTargetModeInfo(qlonglong time, int trgLat, int trgLon, uchar state, ushort dtpCount, ushort dist, float da)` | `0xa` |
| slot | public | `void sendExplorationInfo(qlonglong time, ushort realWindDir, float realWindSpeed)` | `0xa` |
| slot | public | `void setLaunchReady(bool ready)` | `0xa` |
| slot | public | `void setLaunched()` | `0xa` |
| slot | public | `void startVideoStream()` | `0xa` |
| slot | public | `void enableFullPower()` | `0xa` |
| slot | public | `void getVideoStreamStatus()` | `0xa` |
| slot | public | `void enableVideoStream()` | `0xa` |
| slot | public | `void disableVideoStream()` | `0xa` |
| slot | private | `void connectToUDPTelemetry()` | `0x8` |
| slot | private | `void connectToUDPCommands()` | `0x8` |
| slot | private | `void connectToUDPEPLaunch()` | `0x8` |
| slot | private | `void readTelemetryPendingDatagrams()` | `0x8` |
| slot | private | `void readTelemetryPendingDatagrams2()` | `0x8` |
| slot | private | `void readModemCommandPendingDatagrams()` | `0x8` |
| slot | private | `void readEPLaunchDatagrams()` | `0x8` |
| slot | private | `void pingTelemetry()` | `0x8` |
| slot | private | `void pingTelemetryFinished(int exitCode, QProcess::ExitStatus exitStatus)` | `0x8` |

## `ELogger`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void setEnabled()` | `0xa` |
| slot | public | `void setDisabled()` | `0xa` |
| slot | public | `void flushLog()` | `0xa` |
| slot | private | `void incLogPart()` | `0x8` |

## `EPathfinder`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void controlOn(int mode)` | `0x6` |
| signal | public | `void controlOff()` | `0x6` |
| signal | public | `void copterControlOn(int mode)` | `0x6` |
| signal | public | `void copterControlOff()` | `0x6` |
| signal | public | `void setControlParams(int roll, int yaw, int pitch, int throttle)` | `0x6` |
| signal | public | `void setFlapsValue(int value)` | `0x6` |
| signal | public | `void setCopterControlParams(int roll, int yaw, int pitch, int throttle)` | `0x6` |
| signal | public | `void setCameraParams(int pitch)` | `0x6` |
| signal | public | `void resyncMavTimer()` | `0x6` |
| signal | public | `void resyncCopterMavTimer()` | `0x6` |
| signal | public | `void setMode(int mode)` | `0x6` |
| signal | public | `void testEnded()` | `0x6` |
| signal | public | `void showMessageOnOSD(QString message)` | `0x6` |
| signal | public | `void mavlinkConnected()` | `0x6` |
| signal | public | `void mavlinkDisconnected()` | `0x6` |
| signal | public | `void reconnectMavlink()` | `0x6` |
| signal | public | `void elinkConnected()` | `0x6` |
| signal | public | `void elinkDisconnected()` | `0x6` |
| signal | public | `void enableAccelLog()` | `0x6` |
| signal | public | `void disableAccelLog()` | `0x6` |
| signal | public | `void parachuteOn()` | `0x6` |
| signal | public | `void landed()` | `0x6` |
| signal | public | `void setGimbal(Gimbal gimbal)` | `0x6` |
| signal | public | `void setZoom(float percent)` | `0x6` |
| signal | public | `void setFocus(float distance, float zoomPercent, int timeoutMSec)` | `0x6` |
| signal | public | `void ptzCameraOn()` | `0x6` |
| signal | public | `void ptzCameraOff()` | `0x6` |
| signal | public | `void takeoffed()` | `0x6` |
| signal | public | `void armOn()` | `0x6` |
| signal | public | `void setPreprocessingScale(float)` | `0x6` |
| signal | public | `void setObjectDetectorVisionModuleEnabled(bool enabled)` | `0x6` |
| signal | public | `void setPathfinderVisionModuleEnabled(bool enabled)` | `0x6` |
| signal | public | `void setVideoSaverVisionModuleEnabled(bool enabled)` | `0x6` |
| signal | public | `void sendTelemetryPosAtt(qlonglong time, int lat, int lon, int alt, float roll, float pitch, float yaw, float airSpeed)` | `0x6` |
| signal | public | `void sendTelemetryCameraParams(qlonglong time, float gimbalRoll, float gimbalPitch, float gimbalYaw)` | `0x6` |
| signal | public | `void sendTelemetryGPSPos(qlonglong time, int lat, int lon, int alt)` | `0x6` |
| signal | public | `void sendTelemetryPlan(qlonglong time, Plan plan)` | `0x6` |
| signal | public | `void sendTelemetryLastWPPos(qlonglong time, int lat, int lon)` | `0x6` |
| signal | public | `void sendTelemetryGPSStatus(qlonglong time, uchar epStatus, uchar gpsStatus, uchar satellites)` | `0x6` |
| signal | public | `void sendTelemetryCurrentFlightMode(qlonglong time, uchar mode)` | `0x6` |
| signal | public | `void sendTelemetryCurrentMode(qlonglong time, uchar mode, ushort trgAlt)` | `0x6` |
| signal | public | `void sendTelemetryPlanModeInfo(qlonglong time, uchar nextWP, ushort nextWPDist, float da)` | `0x6` |
| signal | public | `void sendTelemetryFindModeInfo(qlonglong time, uchar dir, ushort centerDist)` | `0x6` |
| signal | public | `void sendTelemetryTargetCorrectionModeInfo(qlonglong time, int trgLat, int trgLon, ushort dtpCount, ushort dist, float da)` | `0x6` |
| signal | public | `void sendTelemetryTargetModeInfo(qlonglong time, int trgLat, int trgLon, uchar state, ushort dtpCount, ushort dist, float da)` | `0x6` |
| signal | public | `void sendTelemetryDistanceInfo(qlonglong time, int battery, int homeDistance, int distanceTraveled)` | `0x6` |
| signal | public | `void sendTelemetryExplorationInfo(qlonglong time, ushort realWindDir, float realWindSpeed)` | `0x6` |
| signal | public | `void sendTelemetryRadarEmulationInfo(qlonglong time, int lat, int lon, int alt, float azimuth, float groundSpeed)` | `0x6` |
| signal | public | `void sendTelemetryToVNav(int lat, int lon, int alt, float yaw, short vx, short vy, float gmblRoll, float gmblPitch, float gmblYaw, float zoom, int gpsLat, int gpsLon)` | `0x6` |
| signal | public | `void sendTelemetryLaunchReady(bool ready)` | `0x6` |
| signal | public | `void sendTelemetryLaunched()` | `0x6` |
| signal | public | `void sendAirspeedToELink(float value)` | `0x6` |
| signal | public | `void sendVNavStart()` | `0x6` |
| signal | public | `void sendVNavStop()` | `0x6` |
| signal | public | `void sendVNavReset()` | `0x6` |
| signal | public | `void sendVNavMode(uchar mode, int trgLat, int trgLon)` | `0x6` |
| signal | public | `void sendVNavRoadPoints(qlonglong time, QList<QPoint> points)` | `0x6` |
| signal | public | `void sendTVisStart()` | `0x6` |
| signal | public | `void sendTVisStop()` | `0x6` |
| signal | public | `void setPTZDistSensorPitch(float pitch)` | `0x6` |
| signal | public | `void burnRocket()` | `0x6` |
| signal | public | `void separateRocketStage()` | `0x6` |
| signal | public | `void roadData(QVector<QGeoCoordinate> visibleCoords, bool isSingleRoadLine, QGeoCoordinate planeCoord, QGeoCoordinate planeRawCoord, QGeoCoordinate viewCenter, int prevWPPlanIdx, int nextWPPlanIdx, QGeoCoordinate prevWPCoord, QGeoCoordinate nextWPCoord, int planeAltitude, float planeYaw, float viewAngle, float viewDist, float currentYawOffset, float imuMaxOffset, bool isExactMagValue, bool isObjAsRP)` | `0x6` |
| signal | public | `void setRoadSimpleMode(bool enabled)` | `0x6` |
| signal | public | `void setFrameText(QStringList lines)` | `0x6` |
| signal | public | `void setTelemetryFullPower()` | `0x6` |
| signal | public | `void startTelemetryVideoStream()` | `0x6` |
| signal | public | `void egimbalValues(float roll, float pitch, float yaw, bool isCenter, qlonglong time)` | `0x6` |
| signal | public | `void egimbalRollMagValue(qlonglong time, uchar compId, float roll, float pitch, float yaw, float magRoll)` | `0x6` |
| signal | public | `void egimbalPitchMagValue(qlonglong time, uchar compId, float roll, float pitch, float yaw, float magPitch)` | `0x6` |
| signal | public | `void egimbalYawMagValue(qlonglong time, uchar compId, float roll, float pitch, float yaw, float magYaw)` | `0x6` |
| signal | public | `void egimbalMagValues(qlonglong time, uchar compId, float roll, float pitch, float yaw, float magRoll, float magPitch, float magYaw)` | `0x6` |
| signal | public | `void setCurrentPos(qlonglong srcTime, int lat, int lon)` | `0x6` |
| signal | public | `void writeParam(QString name, float value)` | `0x6` |
| signal | public | `void startRpmControl()` | `0x6` |
| signal | public | `void stopRpmControl()` | `0x6` |
| signal | public | `void startBackupControllerDrop()` | `0x6` |
| signal | public | `void openSpyCamera()` | `0x6` |
| signal | public | `void closeSpyCamera()` | `0x6` |
| signal | public | `void dropMinelayer()` | `0x6` |
| signal | public | `void setNormalThrottle()` | `0x6` |
| signal | public | `void setReverseThrottle()` | `0x6` |
| signal | public | `void flushLog()` | `0x6` |
| signal | public | `void sendAHRSToMavlink(qlonglong time, float roll, float pitch, float yaw)` | `0x6` |
| slot | public | `void currentModeChanged(int newPlaneMode)` | `0xa` |
| slot | public | `void copterCurrentModeChanged(int newCopterMode)` | `0xa` |
| slot | public | `void currentMissionItemChanged(int idx, bool bUpdatePrev)` | `0xa` |
| slot | public | `void currentMissionItemChanged(int idx)` | `0x2a` |
| slot | public | `void homingOn()` | `0xa` |
| slot | public | `void homingOff()` | `0xa` |
| slot | public | `void copterHomingOn()` | `0xa` |
| slot | public | `void copterHomingOff()` | `0xa` |
| slot | public | `void backupHomingOn()` | `0xa` |
| slot | public | `void backupHomingOff()` | `0xa` |
| slot | public | `void airspeedEvent(float airSpeed, float groundSpeed)` | `0xa` |
| slot | public | `void windEvent(float direction, float speed, float speed_z)` | `0xa` |
| slot | public | `void setTestControlParams(int testRoll, int testPitch, int throttle, int throttle2, int cameraRoll, int cameraPitch, int cameraYaw, int cameraZoom, bool cameraCenter)` | `0xa` |
| slot | public | `void emptyDTP()` | `0xa` |
| slot | public | `void targetDetected(CameraParams cameraParams, QList<Target> targets)` | `0xa` |
| slot | public | `void roadDetected(qlonglong time, CameraParams cameraParams, QVector<RoadPointAngle> points, bool isSingleLine)` | `0xa` |
| slot | public | `void imageTargetDetected(CameraParams cameraParams, Target target)` | `0xa` |
| slot | public | `void roadScreenParts(qlonglong time, int left, int middle, int right)` | `0xa` |
| slot | public | `void enableVisionDetect()` | `0xa` |
| slot | public | `void launchAcceleration()` | `0xa` |
| slot | public | `void setCurrentPlan(Plan plan)` | `0xa` |
| slot | public | `void setWind(Wind globalWind)` | `0xa` |
| slot | public | `void setLastWPCoordFromTelemetry(int lat, int lon)` | `0xa` |
| slot | public | `void startSearchFromTelemetry(qlonglong time)` | `0xa` |
| slot | public | `void setParams(EPathfinderParams params)` | `0xa` |
| slot | public | `void currentControls(uint16_t roll, uint16_t pitch, uint16_t throttle)` | `0xa` |
| slot | public | `void copterCurrentControls(uint16_t roll, uint16_t pitch, uint16_t throttle)` | `0xa` |
| slot | public | `uint16_t rcValue(int id)` | `0xa` |
| slot | public | `void currentRcValues(uint16_t chan05_raw, uint16_t chan06_raw, uint16_t chan07_raw, uint16_t chan08_raw, uint16_t chan09_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint16_t chan13_raw, uint16_t chan14_raw, uint16_t chan15_raw, uint16_t chan16_raw, uint16_t chan17_raw, uint16_t chan18_raw)` | `0xa` |
| slot | public | `void copterCurrentRcValues(uint16_t chan05_raw, uint16_t chan06_raw, uint16_t chan07_raw, uint16_t chan08_raw, uint16_t chan09_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint16_t chan13_raw, uint16_t chan14_raw, uint16_t chan15_raw, uint16_t chan16_raw, uint16_t chan17_raw, uint16_t chan18_raw)` | `0xa` |
| slot | public | `float getManuMaxAccum()` | `0xa` |
| slot | public | `qlonglong getManuOldTimeOffset()` | `0xa` |
| slot | public | `void activateWeapon()` | `0xa` |
| slot | public | `void cameraParamsChanged(CameraParams params)` | `0xa` |
| slot | public | `void setPTZCamera(bool isPTZ)` | `0xa` |
| slot | public | `void scoutValid()` | `0xa` |
| slot | public | `void scoutInvalid()` | `0xa` |
| slot | public | `void setTestPTZCameraIsCenter(bool isCenter)` | `0xa` |
| slot | public | `void setPTZCalibration(bool enable)` | `0xa` |
| slot | public | `void setVisionTest(bool enabled)` | `0xa` |
| slot | public | `void preprocessingScaleChanged(float value)` | `0xa` |
| slot | public | `void objectDetectorVisionModuleEnabled(bool enabled)` | `0xa` |
| slot | public | `void pathfinderVisionModuleEnabled(bool enabled)` | `0xa` |
| slot | public | `void videoSaverVisionModuleEnabled(bool enabled)` | `0xa` |
| slot | public | `void updateVisionModuleActivities()` | `0xa` |
| slot | public | `void setRoadsEnabled(bool enabled)` | `0xa` |
| slot | public | `void roadOffsetItemAdded(RoadValidateResult item)` | `0xa` |
| slot | public | `void roadOffsetAccepted(RoadValidateResult res)` | `0xa` |
| slot | public | `void imuCorrected(float confidence, float offset, int objectIdx, IMUCorrectSource source)` | `0xa` |
| slot | public | `void checkNextWP()` | `0xa` |
| slot | public | `void checkARMWP()` | `0xa` |
| slot | public | `void imuCorrectedByPeekAtGPS()` | `0xa` |
| slot | public | `void spinDetected()` | `0xa` |
| slot | public | `void setRelease(bool release)` | `0xa` |
| slot | public | `void setBatteryVoltage(int voltage)` | `0xa` |
| slot | public | `void setRadarPlaneTarget(PlaneTarget target)` | `0xa` |
| slot | public | `void setLidarPlaneTarget(QList<PlaneCoord> targets)` | `0xa` |
| slot | public | `void setShumodavTarget(short powerAzim, short powerElev, short angleAzim, short angleElev)` | `0xa` |
| slot | public | `void gimbalDeviceAttitudeStatus(qlonglong time, uchar compId, uchar gimbalDeviceId, float qw, float qx, float qy, float qz, float angular_velocity_x, float angular_velocity_y, float angular_velocity_z, float deltaYaw, float deltaYawVelocity)` | `0xa` |
| slot | public | `void roadsLookAt(QGeoCoordinate coord)` | `0xa` |
| slot | public | `void roadsStopLookAt()` | `0xa` |
| slot | public | `void setVNavEnabled(bool enabled)` | `0xa` |
| slot | public | `void setCurrentPosFromFlower(int lat, int lon)` | `0xa` |
| slot | public | `void setAHRSFromFlower(float roll, float pitch, float yaw, uchar gnss, int lat, int lon, int alt, float hdg, float gs, float as)` | `0xa` |
| slot | public | `void setAHRSFromVNav(qlonglong time, int lat, int lon, uint alt, float gmblRoll, float gmblPitch, float gmblYaw, uchar status)` | `0xa` |
| slot | public | `void setAHRSByCamera(qlonglong time, float vehicleRoll, float vehiclePitch, float vehicleYaw)` | `0xa` |
| slot | public | `void rpmValues(int _rpm1, int _rpm2)` | `0xa` |
| slot | public | `void vehicleDetected(qlonglong time, QList<VehicleTarget> targets)` | `0xa` |
| slot | public | `void setVehicleVersion(int value)` | `0xa` |
| slot | public | `void setAutoLandEnabled(bool enabled)` | `0xa` |
| slot | public | `void setInterceptorEnabled(bool enabled)` | `0xa` |
| slot | public | `void setExtremeGimbalTest(bool enabled)` | `0xa` |
| slot | public | `void setLoadImageTargetsFromClient(bool value)` | `0xa` |
| slot | public | `void setLoadImageMinelayerFromClient(bool value)` | `0xa` |
| slot | public | `void setLaunchReady(bool ready)` | `0xa` |
| slot | public | `void setBackupBattery(uchar id, short value)` | `0xa` |
| slot | private | `void disableControlHandler()` | `0x8` |
| slot | private | `void setDisableVehicleMode()` | `0x8` |
| slot | private | `void setCircleMode()` | `0x8` |
| slot | private | `void setStabilizeMode()` | `0x8` |
| slot | private | `void rebootPTZCameraIfCalm()` | `0x8` |
| slot | private | `void rebootPTZCamera()` | `0x8` |
| slot | private | `void enablePTZCamera()` | `0x8` |
| slot | private | `void disablePTZCamera()` | `0x8` |
| slot | private | `void updatePreprocessingScale()` | `0x8` |
| slot | private | `void switchRoadDir()` | `0x8` |

## `EScout`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void timeSynced()` | `0x6` |
| signal | public | `void takeoffed()` | `0x6` |
| signal | public | `void valid()` | `0x6` |
| signal | public | `void invalid()` | `0x6` |
| signal | public | `void addElinkTimeOffset(int value)` | `0x6` |
| signal | public | `void resetRoadMaxDistanceOffsetTime()` | `0x6` |
| signal | public | `void roadOffsetAccepted(RoadValidateResult res)` | `0x6` |
| signal | public | `void imuCorrectedByPeekAtGPS()` | `0x6` |
| signal | public | `void imuCorrected(float confidence, float offset, int objectIdx, IMUCorrectSource source)` | `0x6` |
| slot | public | `void setTimeOffset(qlonglong offset, int roundDt)` | `0xa` |
| slot | public | `void setCopterThrottle(int throttle)` | `0xa` |
| slot | public | `void setPosition(uint32_t time_boot_ms, int32_t lat, int32_t lon, int32_t alt, int32_t mslAlt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)` | `0xa` |
| slot | public | `void setRawPositionEvent(uint32_t time_boot_ms, int32_t lat, int32_t lon, int32_t mslAlt)` | `0xa` |
| slot | public | `void setAttitude(uint32_t time_boot_ms, float pitch, float yaw, float roll, float pitchSpeed, float yawSpeed, float rollSpeed)` | `0xa` |
| slot | public | `void setBackupAttitude(uchar linkId, uint32_t time_boot_ms, float pitch, float yaw, float roll, float pitchSpeed, float yawSpeed, float rollSpeed)` | `0xa` |
| slot | public | `void setAhrsError(float rp, float yaw)` | `0xa` |
| slot | public | `void setDistanceSensor(uint minDist, uint maxDist, uint curDist)` | `0xa` |
| slot | public | `void launchAcceleration()` | `0xa` |
| slot | public | `void setManuTakeoffed()` | `0xa` |
| slot | public | `void setSmoothAttSpeed(bool value)` | `0xa` |
| slot | public | `void landed()` | `0xa` |
| slot | public | `void setEPosition(qlonglong time, uchar status, uchar satellites, int lat, int lon, int alt, float vx, float vy, float vz, float groundSpeed, float groundCourse)` | `0xa` |
| slot | public | `void setEBackupPosition(qlonglong time, uchar status, uchar satellites, int lat, int lon, int alt, float vx, float vy, float vz, float groundSpeed, float groundCourse)` | `0xa` |
| slot | public | `void setEAttitude(qlonglong time, float pitch, float roll, float yaw)` | `0xa` |
| slot | public | `void setExactMagOffsetEnabled(bool enabled)` | `0xa` |
| slot | public | `void setExactMagTargetEnabled(bool enabled)` | `0xa` |
| slot | public | `void setCurrentExactMagValue(int yaw)` | `0xa` |
| slot | public | `void setExactMagDisableTime(qlonglong time)` | `0xa` |
| slot | public | `void setExactMagIsFreezed(bool value)` | `0xa` |
| slot | public | `void exactMagReseted()` | `0xa` |
| slot | public | `void setCopterAttitude(uint32_t time_boot_ms, float pitch, float yaw, float roll, float pitchSpeed, float yawSpeed, float rollSpeed)` | `0xa` |
| slot | public | `void setCopterPosition(uint32_t time_boot_ms, int32_t lat, int32_t lon, int32_t alt, int32_t mslAlt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)` | `0xa` |
| slot | public | `void calibrateRollPitch()` | `0xa` |
| slot | public | `void calibrateYaw()` | `0xa` |
| slot | public | `void resetYawOffset()` | `0xa` |
| slot | public | `float currentYawOffset()` | `0xa` |
| slot | public | `void setELinkRCControlState(bool enabled)` | `0xa` |
| slot | public | `void setPTZDistSensorPitch(float pitch)` | `0xa` |
| slot | public | `void setDebugCorrectAltByPitch(bool enabled)` | `0xa` |
| slot | public | `void setRoadOffset(RoadValidateResult result)` | `0xa` |
| slot | public | `void setLerpDistSensorOffset(bool enabled)` | `0xa` |
| slot | public | `void selfDiagnostic()` | `0xa` |
| slot | public | `void setCurrentPosFromTelemetry(qlonglong time, int lat, int lon)` | `0xa` |
| slot | public | `void setCurrentPosFromVNav(qlonglong time, int lat, int lon, float conf)` | `0xa` |
| slot | public | `void setCurrentPosFromVNav(qlonglong time, int lat, int lon)` | `0x2a` |
| slot | public | `void setCurrentPosFromImageTarget(qlonglong time, int lat, int lon)` | `0xa` |
| slot | public | `void setCurrentPosFromWayTargets(qlonglong time, int lat, int lon)` | `0xa` |
| slot | public | `void setCurrentPosFromFlower(qlonglong time, int lat, int lon)` | `0xa` |
| slot | public | `void setCurrentAHRSFromVNav(qlonglong time, int lat, int lon, uint alt, float roll, float pitch, float yaw, uchar status)` | `0xa` |
| slot | public | `void setYawOffsetFromVNav(qlonglong time, float offset)` | `0xa` |
| slot | public | `void setEnabledYawOffset(bool enabled)` | `0xa` |
| slot | public | `void undoLastRoadOffset()` | `0xa` |
| slot | public | `void setVNavIgnored(bool ignore)` | `0xa` |
| slot | public | `QGeoCoordinate lastRawIMU()` | `0xa` |
| slot | public | `void saveCoordinateByIMU()` | `0xa` |
| slot | public | `void restoreCoordinateByIMU()` | `0xa` |
| slot | public | `void setReplaceYawFromExactMag(bool enabled)` | `0xa` |
| slot | public | `void exactMagInited()` | `0xa` |
| slot | public | `void setInertCoeff(float value)` | `0xa` |
| slot | public | `bool vnavAzimuth(float& value)` | `0xa` |
| slot | private | `void enableOdometry()` | `0x8` |
| slot | private | `void upConnectOdometry()` | `0x8` |
| slot | private | `void readOdoPendingDatagrams()` | `0x8` |
| slot | private | `void updateCurMSL()` | `0x8` |
| slot | private | `void autoUpdateExactMagOffset()` | `0x8` |
| slot | private | `void calcExactMagOffsetByMainMag()` | `0x8` |
| slot | private | `void readExactMagTable()` | `0x8` |

## `EVision`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void targetDetected(CameraParams params, QList<Target> targets)` | `0x6` |
| signal | public | `void imageTargetDetected(CameraParams params, Target target)` | `0x6` |
| signal | public | `void roadDetected(qlonglong time, CameraParams cameraParams, QVector<RoadPointAngle> roadPoints, bool isSingleLine)` | `0x6` |
| signal | public | `void roadScreenParts(qlonglong time, int left, int middle, int right)` | `0x6` |
| signal | public | `void emptyDTP()` | `0x6` |
| signal | public | `void cameraParamsChanged(CameraParams params)` | `0x6` |
| signal | public | `void setPTZ(float roll, float pitch, float yaw, float rollMaxSpeed, float pitchMaxSpeed, float yawMaxSpeed, bool center, bool bYawByNorth, uchar gimbalId)` | `0x6` |
| signal | public | `void preprocessingScaleChanged(float)` | `0x6` |
| signal | public | `void rtspEnabled()` | `0x6` |
| signal | public | `void rtspDisabled()` | `0x6` |
| signal | public | `void dtpDebugInfo(QString text)` | `0x6` |
| signal | public | `void objectDetectorEnabled(bool enabled)` | `0x6` |
| signal | public | `void pathfinderEnabled(bool enabled)` | `0x6` |
| signal | public | `void videoSaverEnabled(bool enabled)` | `0x6` |
| signal | public | `void ahrsByCamera(qlonglong time, float vehicleRoll, float vehiclePitch, float vehicleYaw)` | `0x6` |
| slot | public | `void connectToCamera()` | `0xa` |
| slot | public | `void setGimbal(Gimbal gimbal)` | `0xa` |
| slot | public | `void gimbalValues(float roll, float pitch, float yaw, bool isCenter, qlonglong time)` | `0xa` |
| slot | public | `void gimbalRollMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magRoll)` | `0xa` |
| slot | public | `void gimbalPitchMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magPitch)` | `0xa` |
| slot | public | `void gimbalYawMagValue(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magYaw)` | `0xa` |
| slot | public | `void gimbalMagValues(qlonglong time, uchar componentId, float roll, float pitch, float yaw, float magRoll, float magPitch, float magYaw)` | `0xa` |
| slot | public | `void setGimbalAHRS(qlonglong time, int lat, int lon, uint alt, float gmblRoll, float gmblPitch, float gmblYaw, uchar status)` | `0xa` |
| slot | public | `void setZoom(float percent)` | `0xa` |
| slot | public | `void setFocus(float distance, float zoomPercent, int timeoutMSec)` | `0xa` |
| slot | public | `void setPreprocessingScale(float value)` | `0xa` |
| slot | public | `void setRTSPEnabled(bool enabled)` | `0xa` |
| slot | public | `void enableRTSP()` | `0xa` |
| slot | public | `void disableRTSP()` | `0xa` |
| slot | public | `void setRoadSimpleMode(bool enabled)` | `0xa` |
| slot | public | `void setFrameText(QStringList lines)` | `0xa` |
| slot | public | `void setObjectDetectorEnabled(bool enabled)` | `0xa` |
| slot | public | `void setPathfinderEnabled(bool enabled)` | `0xa` |
| slot | public | `void setVideoSaverEnabled(bool enabled)` | `0xa` |
| slot | public | `void sendImageTarget(qlonglong time, short x, short, uchar type)` | `0xa` |
| slot | private | `void readPendingDatagrams()` | `0x8` |
| slot | private | `void upConnect()` | `0x8` |
| slot | private | `void setTargets(QList<Target>& targets)` | `0x8` |
| slot | private | `void frameBlur(float value)` | `0x8` |

## `GPIOController`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void setLedState(VehicleLed::Type type, VehicleLed::State state)` | `0xa` |
| slot | public | `void armTestOn()` | `0xa` |
| slot | public | `void armTestOff()` | `0xa` |
| slot | public | `void armOn()` | `0xa` |
| slot | public | `void selfDestruction()` | `0xa` |

## `GpsHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void satelliteCountChanged()` | `0x6` |
| signal | public | `void positionEvent(uint32_t time_boot_ms, int32_t lat, int32_t lon, int32_t alt, int32_t mslAlt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)` | `0x6` |
| signal | public | `void rawPositionEvent(uint32_t time_boot_ms, int32_t lat, int32_t lon, int32_t mslAlt)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0xa` |

## `HeartbeatHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void currentModeChanged(int mode)` | `0x6` |
| signal | public | `void currentMissionItemChanged(int idx)` | `0x6` |
| signal | public | `void homingOn()` | `0x6` |
| signal | public | `void homingOff()` | `0x6` |
| signal | public | `void launchAcceleration()` | `0x6` |
| signal | public | `void armed()` | `0x6` |
| signal | public | `void disarmed()` | `0x6` |
| signal | public | `void compassStatusChanged(bool enabled)` | `0x6` |
| signal | public | `void mavlinkParamValue(QString name, float value)` | `0x6` |
| signal | public | `void airspeedEvent(float airSpeed, float groundSpeed)` | `0x6` |
| signal | public | `void distanceSensorEvent(uint minDist, uint maxDist, uint curDist)` | `0x6` |
| signal | public | `void gimbalDeviceAttitudeStatus(qlonglong time, uchar compId, uchar gimbalDeviceId, float qw, float qx, float qy, float qz, float angular_velocity_x, float angular_velocity_y, float angular_velocity_z, float deltaYaw, float deltaYawVelocity)` | `0x6` |
| signal | public | `void isValidArdupilotVersion()` | `0x6` |
| signal | public | `void backupBattery(uchar id, short value)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0xa` |
| slot | public | `void setManualAccelerationEvent()` | `0xa` |
| slot | public | `void showMessageOnOSD(QString message)` | `0xa` |
| slot | public | `void readParam(QString name)` | `0xa` |
| slot | public | `void writeParam(QString name, float value)` | `0xa` |
| slot | public | `void enableAccelLog()` | `0xa` |
| slot | public | `void disableAccelLog()` | `0xa` |
| slot | public | `void disableOtherMessages()` | `0xa` |
| slot | private | `void sendOsdMessage()` | `0x8` |
| slot | private | `void writeParams()` | `0x8` |
| slot | private | `void readParams()` | `0x8` |
| slot | private | `void readArdupilotVersion()` | `0x8` |

## `InterceptorLidar`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void lidarData(QList<PlaneCoord> objects)` | `0x6` |

## `InterceptorLivox`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void Init()` | `0xa` |
| slot | public | `void setFrameTime(int milliseconds)` | `0xa` |

## `Kalman`

No class-local signals, slots, or invokable methods.

## `LogHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void logListEvent(QList<LogItem> items)` | `0x6` |
| signal | public | `void getLogListError(int code)` | `0x6` |
| signal | public | `void logEvent(QByteArray data)` | `0x6` |
| signal | public | `void getLogError(int code)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |

## `MavLinkCommunicator`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void messageReceived(mavlink_message_t message)` | `0x6` |
| signal | public | `void backupMessageReceived(QString sender, mavlink_message_t message, uchar linkId)` | `0x6` |
| slot | public | `void addLink(AbstractLink* link, uint8_t channel, int id)` | `0xa` |
| slot | public | `void removeLink(AbstractLink* link)` | `0xa` |
| slot | public | `void setSystemId(uint8_t systemId)` | `0xa` |
| slot | public | `void setComponentId(uint8_t componentId)` | `0xa` |
| slot | public | `void sendMessage(mavlink_message_t message, AbstractLink* link)` | `0xa` |
| slot | public | `void sendMessageOnLastReceivedLink(mavlink_message_t message)` | `0xa` |
| slot | public | `void sendMessageOnAllLinks(mavlink_message_t message)` | `0xa` |
| slot | public | `void sendMessageOnMainLink(mavlink_message_t message)` | `0xa` |
| slot | public | `void sendMessageOnBackupLink(mavlink_message_t message)` | `0xa` |
| slot | protected | `void onDataReceived(QByteArray data)` | `0x9` |

## `PlanHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void writePlanAnswer(QJsonDocument answer)` | `0x6` |
| signal | public | `void planWrited()` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void getPlan(QJsonDocument plan)` | `0xa` |
| slot | public | `void writePlan(QJsonDocument plan)` | `0xa` |
| slot | public | `void removePlanFile()` | `0xa` |
| slot | public | `void loadLastPlan()` | `0xa` |
| slot | public | `void setArmDisarm(bool value)` | `0xa` |
| slot | public | `void reset()` | `0xa` |
| slot | private | `void transactionTimerEvent()` | `0x8` |
| slot | private | `void removePlan()` | `0x8` |

## `RcChannelsHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void homingOn()` | `0x6` |
| signal | public | `void homingOff()` | `0x6` |
| signal | public | `void backupHomingOn()` | `0x6` |
| signal | public | `void backupHomingOff()` | `0x6` |
| signal | public | `void currentControls(uint16_t roll, uint16_t pitch, uint16_t throttle)` | `0x6` |
| signal | public | `void currentChannels(uint16_t chan05_raw, uint16_t chan06_raw, uint16_t chan07_raw, uint16_t chan08_raw, uint16_t chan09_raw, uint16_t chan10_raw, uint16_t chan11_raw, uint16_t chan12_raw, uint16_t chan13_raw, uint16_t chan14_raw, uint16_t chan15_raw, uint16_t chan16_raw, uint16_t chan17_raw, uint16_t chan18_raw)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0xa` |

## `RemoteController`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void enableControl()` | `0x6` |
| signal | public | `void disableControl()` | `0x6` |
| signal | public | `void controlValues(int roll, int pitch, int throttle)` | `0x6` |
| slot | private | `void sendMsgIAmPlane()` | `0x8` |
| slot | private | `void connected()` | `0x8` |
| slot | private | `void disconnected()` | `0x8` |
| slot | private | `void readServer()` | `0x8` |

## `RoadAnalyzer`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void elementsDetected(QVector<RoadElement> elements)` | `0x6` |

## `RoadAnalyzerV1`

No class-local signals, slots, or invokable methods.

## `RoadAnalyzerV2`

No class-local signals, slots, or invokable methods.

## `RoadNetworkAnalyzer`

No class-local signals, slots, or invokable methods.

## `RoadRunner`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void offsetItemAdded(RoadValidateResult item)` | `0x6` |
| signal | public | `void offsetChanged(RoadValidateResult result)` | `0x6` |
| signal | public | `void elementsDetected(qlonglong time, QList<RoadElement*> elements)` | `0x6` |
| signal | public | `void lookAt(QGeoCoordinate coord)` | `0x6` |
| signal | public | `void stopLookAt()` | `0x6` |
| slot | public | `void addData(QVector<QGeoCoordinate> visibleCoords, bool isSingleRoadLine, QGeoCoordinate planeCoord, QGeoCoordinate planeRawCoord, QGeoCoordinate viewCenter, int prevWPPlanIdx, int nextWPPlanIdx, QGeoCoordinate prevWPCoord, QGeoCoordinate nextWPCoord, int planeAltitude, float planeYaw, float viewAngle, float viewDist, float currentYawOffset, float maxOffset, bool isExactMagValue)` | `0xa` |
| slot | public | `void addData2(QVector<QGeoCoordinate> visibleCoords, bool isSingleRoadLine, QGeoCoordinate planeCoord, QGeoCoordinate planeRawCoord, QGeoCoordinate viewCenter, int prevWPPlanIdx, int nextWPPlanIdx, QGeoCoordinate prevWPCoord, QGeoCoordinate nextWPCoord, int planeAltitude, float planeYaw, float viewAngle, float viewDist, float currentYawOffset, float maxOffset, bool isExactMagValue, bool isObjAsRP)` | `0xa` |
| slot | private | `void detectedElementsHandler(QVector<RoadElement> elements)` | `0x8` |

## `SerialLink`

Properties:

| Name | Type | Flags |
|---|---|---:|
| `portName` | `QString` | `0x495103` |
| `baudRate` | `int` | `0x495103` |

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void portNameChanged(QString portName)` | `0x6` |
| signal | public | `void baudRateChanged(int baudRate)` | `0x6` |
| slot | public | `void up()` | `0xa` |
| slot | public | `void down()` | `0xa` |
| slot | public | `void reconnect()` | `0xa` |
| slot | public | `void sendData(QByteArray data)` | `0xa` |
| slot | public | `void sendData(const char* data, qlonglong len)` | `0xa` |
| slot | public | `void setPortName(QString portName)` | `0xa` |
| slot | public | `void setBaudRate(int baudRate)` | `0xa` |
| slot | private | `void readSerialData()` | `0x8` |
| slot | private | `void portError(QSerialPort::SerialPortError error)` | `0x8` |

## `TcpLink`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | public | `void up()` | `0xa` |
| slot | public | `void down()` | `0xa` |
| slot | public | `void sendData(QByteArray data)` | `0xa` |
| slot | public | `void sendData(const char* data, qlonglong len)` | `0xa` |
| slot | private | `void connected()` | `0x8` |
| slot | private | `void disconnected()` | `0x8` |
| slot | private | `void readData()` | `0x8` |

## `ThunderGaze`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void emptyDTP()` | `0x6` |
| signal | public | `void setTargets(QList<Target>& targets)` | `0x6` |
| signal | public | `void preprocessingScaleChanged(float)` | `0x6` |
| signal | public | `void dtpDebugInfo(QString text)` | `0x6` |
| signal | public | `void roadScreenParts(qlonglong time, int left, int middle, int right)` | `0x6` |
| signal | public | `void roadDetected(qlonglong time, CameraParams cameraParams, QVector<RoadPointAngle> roadPoints, bool isSingleLine)` | `0x6` |
| signal | public | `void rtspEnabled()` | `0x6` |
| signal | public | `void rtspDisabled()` | `0x6` |
| signal | public | `void objectDetectorEnabled(bool enabled)` | `0x6` |
| signal | public | `void pathfinderEnabled(bool enabled)` | `0x6` |
| signal | public | `void videoSaverEnabled(bool enabled)` | `0x6` |
| signal | public | `void frameBlur(float value)` | `0x6` |

## `ThunderGazeV1`

No class-local signals, slots, or invokable methods.

## `ThunderGazeV2`

No class-local signals, slots, or invokable methods.

## `ThunderGazeV3`

No class-local signals, slots, or invokable methods.

## `ThunderGazeV4`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void applyTelemetryParams()` | `0x8` |
| slot | private | `void computeBlur()` | `0x8` |

## `ThunderGazeV5`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void applyTelemetryParams()` | `0x8` |
| slot | private | `void computeBlur()` | `0x8` |

## `TimeSyncHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void updateTimeOffset(qlonglong offset, int roundDt)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void reset()` | `0xa` |
| slot | public | `void startSync()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | private | `void sendSyncMessage()` | `0x8` |
| slot | private | `void updateDeltaOffset()` | `0x8` |

## `UdpLink`

Properties:

| Name | Type | Flags |
|---|---|---:|
| `rxPort` | `int` | `0x495103` |
| `address` | `QString` | `0x495103` |
| `txPort` | `int` | `0x495103` |

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void rxPortChanged(int port)` | `0x6` |
| signal | public | `void addressChanged(QString address)` | `0x6` |
| signal | public | `void txPortChanged(int port)` | `0x6` |
| slot | public | `void up()` | `0xa` |
| slot | public | `void down()` | `0xa` |
| slot | public | `void sendData(QByteArray data)` | `0xa` |
| slot | public | `void sendData(const char* data, qlonglong len)` | `0xa` |
| slot | public | `void setRxPort(int port)` | `0xa` |
| slot | public | `void setAddress(QString address)` | `0xa` |
| slot | public | `void setTxPort(int port)` | `0xa` |
| slot | private | `void readPendingDatagrams()` | `0x8` |

## `VNav`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void currentPos(qlonglong time, int lat, int lon, float conf)` | `0x6` |
| signal | public | `void statusChanged(bool avtive)` | `0x6` |
| signal | public | `void vehicleDetected(qlonglong time, QList<VehicleTarget> targets)` | `0x6` |
| signal | public | `void checkPlanResult(bool validVNav, bool validVNav2)` | `0x6` |
| signal | public | `void imageTarget(qlonglong time, short x, short y, uchar type)` | `0x6` |
| signal | public | `void ahrs(qlonglong time, int lat, int lon, uint alt, float roll, float pitch, float yaw, uchar status)` | `0x6` |
| signal | public | `void yawOffset(qlonglong time, float offset)` | `0x6` |
| slot | public | `void start()` | `0xa` |
| slot | public | `void stop()` | `0xa` |
| slot | public | `void reset()` | `0xa` |
| slot | public | `void setOdoEnabled(bool value)` | `0xa` |
| slot | public | `void sendCurrentIMU(int lat, int lon, int alt, float yaw, short vx, short vy, float gmblRoll, float gmblPitch, float gmblYaw, float zoom, int gpsLat, int gpsLon)` | `0xa` |
| slot | public | `void setMode(uchar mode, int trgLat, int trgLon)` | `0xa` |
| slot | public | `void checkPlan(QList<QGeoCoordinate> coords)` | `0xa` |
| slot | public | `void setRoadPoints(qlonglong time, QList<QPoint> coords)` | `0xa` |
| slot | private | `void readPendingDatagrams()` | `0x8` |

## `VehicleController`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void sendAnswer(QJsonDocument answer)` | `0x6` |
| signal | public | `void updateVehicleStatus(VehicleStatus status)` | `0x6` |
| signal | public | `void armed()` | `0x6` |
| signal | public | `void disarmed()` | `0x6` |
| signal | public | `void flushLog()` | `0x6` |
| slot | public | `void getStatusMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void getExtStatusMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void getParamsMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setParamsMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setNodesMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void getNodesMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void getVehicleTypesMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void launchMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void unlaunchMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void set5GMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void resetPlanHandlerMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void testControlMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void testEnded()` | `0xa` |
| slot | public | `void landed()` | `0xa` |
| slot | public | `void setControlParams(QJsonDocument message)` | `0xa` |
| slot | public | `void getCameraScreenshotMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setCodeMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void uploadSoftwareMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void uploadVisionMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void applySoftwareMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void applyVisionMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setVisionEnabledMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void rebootFC(QJsonDocument message)` | `0xa` |
| slot | public | `void startMagnitometerCalibration(QJsonDocument message)` | `0xa` |
| slot | public | `void acceptMagnitometerCalibration(QJsonDocument message)` | `0xa` |
| slot | public | `void cancelMagnitometerCalibration(QJsonDocument message)` | `0xa` |
| slot | public | `void startLevelCalibration(QJsonDocument message)` | `0xa` |
| slot | public | `void clientVersionMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setTelemetryParamsMessage(QJsonDocument message)` | `0xa` |
| slot | public | `void setZoomCameraParams(QJsonDocument message)` | `0xa` |
| slot | public | `void getMapBounds(QJsonDocument message)` | `0xa` |
| slot | public | `void getMapCRC(QJsonDocument message)` | `0xa` |
| slot | public | `void poweroff(QJsonDocument message)` | `0xa` |
| slot | public | `void takeoff()` | `0xa` |
| slot | public | `void arm(bool value)` | `0xa` |
| slot | public | `void setMode(int mode)` | `0xa` |
| slot | public | `void setLedState(VehicleLed::Type type, VehicleLed::State state)` | `0xa` |
| slot | private | `void setIsValidArdupilotVersion()` | `0x8` |
| slot | private | `void readPlaneId()` | `0x8` |
| slot | private | `void planWrited()` | `0x8` |
| slot | private | `void vnavCheckPlanResult(bool validVNav, bool validVNav2)` | `0x8` |
| slot | private | `void satelliteCountChanged()` | `0x8` |
| slot | private | `void mavlinkConnected()` | `0x8` |
| slot | private | `void mavlinkDisconnected()` | `0x8` |
| slot | private | `void reconnectMavlink()` | `0x8` |
| slot | private | `void elinkConnected()` | `0x8` |
| slot | private | `void elinkDisconnected()` | `0x8` |
| slot | private | `void updateGreenLed()` | `0x8` |
| slot | private | `void takeoffed()` | `0x8` |
| slot | private | `void startThunderGaze()` | `0x8` |
| slot | private | `void elinkButtonStateChanged(bool pressed)` | `0x8` |
| slot | private | `void disconnectArmTest()` | `0x8` |
| slot | private | `void checkArmReady()` | `0x8` |
| slot | private | `void checkMavlink()` | `0x8` |
| slot | private | `void checkElink()` | `0x8` |
| slot | private | `void checkCameraDev()` | `0x8` |
| slot | private | `void checkThunderGaze()` | `0x8` |
| slot | private | `void readPTZServoValues()` | `0x8` |
| slot | private | `void armedSysStatus()` | `0x8` |
| slot | private | `void disarmedSysStatus()` | `0x8` |
| slot | private | `void compassStatusChanged(bool enabled)` | `0x8` |
| slot | private | `void vnavStatusChanged(bool active)` | `0x8` |
| slot | private | `void mavlinkParamValue(QString name, float value)` | `0x8` |
| slot | private | `void updatePathfinderParams()` | `0x8` |
| slot | private | `void updateParamsFile()` | `0x8` |
| slot | private | `void screenshotStart()` | `0x8` |
| slot | private | `void screenshotStartThunderGaze()` | `0x8` |
| slot | private | `void screenshotFinished(int exitCode, QProcess::ExitStatus exitStatus)` | `0x8` |
| slot | private | `void logListEvent(QList<LogItem> items)` | `0x8` |
| slot | private | `void getLogListError(int code)` | `0x8` |
| slot | private | `void logEvent(QByteArray data)` | `0x8` |
| slot | private | `void getLogError(int code)` | `0x8` |
| slot | private | `void cameraParamsChanged(CameraParams params)` | `0x8` |
| slot | private | `void batteryVoltageChanged(uint voltage)` | `0x8` |
| slot | private | `void testLed()` | `0x8` |
| slot | private | `void dtpDebugInfo(QString text)` | `0x8` |
| slot | private | `void removeOtherVersions()` | `0x8` |
| slot | private | `void rebootSystem()` | `0x8` |
| slot | private | `void disableNTPService()` | `0x8` |
| slot | private | `void startMagCalAck(uchar result)` | `0x8` |
| slot | private | `void acceptMagCalAck(uchar result)` | `0x8` |
| slot | private | `void cancelMagCalAck(uchar result)` | `0x8` |
| slot | private | `void magCalCompletionPercentage(uchar value)` | `0x8` |
| slot | private | `void magCalStatus(uchar value)` | `0x8` |
| slot | private | `void levelCalAck(uchar result)` | `0x8` |
| slot | private | `void mgCfgStatus(uchar status)` | `0x8` |
| slot | private | `void mgReseted(uchar rc)` | `0x8` |
| slot | private | `void initWithoutLaunch()` | `0x8` |
| slot | private | `void updateVehicleTypeConnects()` | `0x8` |
| slot | private | `void generateUniqueId()` | `0x8` |
| slot | private | `void vnavMapCRCProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)` | `0x8` |
| slot | private | `void vnavMapCRCProcessReadOutput()` | `0x8` |

## `VehicleLed`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| slot | private | `void changeBlink()` | `0x8` |

## `WindHandler`

| Kind | Access | Signature | Flags |
|---|---|---|---:|
| signal | public | `void windEvent(float direction, float speed, float speed_z)` | `0x6` |
| slot | public | `void initIntervals()` | `0xa` |
| slot | public | `void processMessage(mavlink_message_t message)` | `0xa` |
| slot | public | `void backupProcessMessage(QString sender, mavlink_message_t message, uchar linkId)` | `0xa` |

