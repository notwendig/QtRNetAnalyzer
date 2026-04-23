// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#include "canframe.h"

class RNetFrame : public CanFrame
{
  public:
    RNetFrame(const CanFrame &canframe, const char *name, quint32 idmask);
    virtual ~RNetFrame() = default;

    virtual const QString &toString() const;
    QString name() const;

    typedef enum
    {
        msk_ext = 0x40000000,
        msk_rpt = 0x20000000,
        msk_id  = 0x1FFFFFFF
    } mask_t;

    quint32 getKey() const
    {
        return (id & idMask_) | (extended ? msk_ext : 0) | (remote ? msk_rpt : 0);
    }
    static std::unique_ptr<RNetFrame> decodeRNetMessage(const CanFrame &frame);

  protected:
    virtual QString detailsString() const;
    static quint8 byteAt(const QByteArray &data, int index);
    static quint16 le16At(const QByteArray &data, int index);
    static quint32 le32At(const QByteArray &data, int index);
    static QString hexByte(quint8 value);
    static QString hexBytes(const QByteArray &data);

    const QString name_;
    mutable QString displayCache_;

    static quint8 u8(const QByteArray &data, int i)
    {
        if (i < 0 || i >= data.size())
            return 0;
        return static_cast<quint8>(data.at(i));
    }
    quint32 idMask_;
};

class RNetSleepAllDevices : public RNetFrame
{
  public:
    RNetSleepAllDevices(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetSleepAllDevices", idMask) {}
};

class RNetJsmSleeping : public RNetFrame
{
  public:
    RNetJsmSleeping(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetJsmSleeping", idMask) {}
};

class RNetJsmSleepCommencing : public RNetFrame
{
  public:
    RNetJsmSleepCommencing(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetJsmSleepCommencing", idMask) {}
};

class RNetJsmCanBusTest : public RNetFrame
{
  public:
    RNetJsmCanBusTest(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetJsmCanBusTest", idMask) {}
};

class RNetJsmEndParameterExchange : public RNetFrame
{
  public:
    explicit RNetJsmEndParameterExchange(const CanFrame &frame, quint32 idMask);

    int modeMap() const;
    void setModeMap(int value);

  protected:
    QString detailsString() const override;

  private:
    int modeMap_ = 0;
};

class RNetJsmSelectModeMap : public RNetFrame
{
  public:
    explicit RNetJsmSelectModeMap(const CanFrame &frame, quint32 idMask);

    int modeMap() const;
    void setModeMap(int value);

  protected:
    QString detailsString() const override;

  private:
    int modeMap_ = 0;
};

class RNetSerialExchangeRequest : public RNetFrame
{
  public:
    RNetSerialExchangeRequest(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetSerialExchangeRequest", idMask) {}
};

class RNetSerialExchangeReplyRtr : public RNetFrame
{
  public:
    RNetSerialExchangeReplyRtr(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetSerialExchangeReplyRtr", idMask) {}
};

class RNetDropToConfigMode1 : public RNetFrame
{
  public:
    RNetDropToConfigMode1(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetDropToConfigMode1", idMask) {}
};

class RNetDropToConfigMode0 : public RNetFrame
{
  public:
    RNetDropToConfigMode0(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "RNetDropToConfigMode0", idMask) {}
};

class RNetParameterRequest : public RNetFrame
{
  public:
    explicit RNetParameterRequest(const CanFrame &frame, quint32 idMask);

    int module() const;
    void setModule(int value);

    quint8 opcodeByte() const;
    void setOpcodeByte(quint8 value);

    quint8 parameterByte() const;
    void setParameterByte(quint8 value);

    quint8 commandByte() const;
    void setCommandByte(quint8 value);

    quint8 pointer() const;
    void setPointer(quint8 value);

    quint8 subIndex() const;
    void setSubIndex(quint8 value);

    quint16 value16() const;
    void setValue16(quint16 value);

  protected:
    QString detailsString() const override;

  private:
    int module_ = 0;
    quint8 opcodeByte_ = 0;
    quint8 parameterByte_ = 0;
    quint8 commandByte_ = 0;
    quint8 pointer_ = 0;
    quint8 subIndex_ = 0;
    quint16 value16_ = 0;
};

class RNetParameterReply : public RNetFrame
{
  public:
    explicit RNetParameterReply(const CanFrame &frame, quint32 idMask);

    int module() const;
    void setModule(int value);

    quint8 opcodeByte() const;
    void setOpcodeByte(quint8 value);

    quint8 parameterByte() const;
    void setParameterByte(quint8 value);

    quint8 commandByte() const;
    void setCommandByte(quint8 value);

    quint8 pointer() const;
    void setPointer(quint8 value);

    quint8 subIndex() const;
    void setSubIndex(quint8 value);

    quint16 value16() const;
    void setValue16(quint16 value);

  protected:
    QString detailsString() const override;

  private:
    int module_ = 0;
    quint8 opcodeByte_ = 0;
    quint8 parameterByte_ = 0;
    quint8 commandByte_ = 0;
    quint8 pointer_ = 0;
    quint8 subIndex_ = 0;
    quint16 value16_ = 0;
};

class RNetJsmUniqueId : public RNetFrame
{
  public:
    explicit RNetJsmUniqueId(const CanFrame &frame, quint32 idMask);

    QByteArray uniqueId() const;
    void setUniqueId(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    QByteArray uniqueId_;
};

class RNetModeSelectProfile : public RNetFrame
{
  public:
    explicit RNetModeSelectProfile(const CanFrame &frame, quint32 idMask);

    int profile() const;
    void setProfile(int value);

  protected:
    QString detailsString() const override;

  private:
    int profile_ = 0;
};

class RNetModeResponse50 : public RNetFrame
{
  public:
    explicit RNetModeResponse50(const CanFrame &frame, quint32 idMask);

    quint8 status() const;
    void setStatus(quint8 value);

    int mode() const;
    void setMode(int value);

    quint8 valueByte() const;
    void setValueByte(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    quint8 status_ = 0;
    int mode_ = 0;
    quint8 valueByte_ = 0;
};

class RNetModeSelectOrSuspend : public RNetFrame
{
  public:
    explicit RNetModeSelectOrSuspend(const CanFrame &frame, quint32 idMask);

    bool suspend() const;
    void setSuspend(bool value);

    int mode() const;
    void setMode(int value);

  protected:
    QString detailsString() const override;

  private:
    bool suspend_ = false;
    int mode_ = 0;
};

class RNetModeResponse60 : public RNetFrame
{
  public:
    explicit RNetModeResponse60(const CanFrame &frame, quint32 idMask);

    quint8 status() const;
    void setStatus(quint8 value);

    int mode() const;
    void setMode(int value);

    quint8 valueByte() const;
    void setValueByte(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    quint8 status_ = 0;
    int mode_ = 0;
    quint8 valueByte_ = 0;
};

class Unknown : public RNetFrame
{
  public:
    Unknown(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "Unknown", idMask) {}
};

class RNetJoystickPosition : public RNetFrame
{
  public:
    explicit RNetJoystickPosition(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    qint8 x() const;
    void setX(qint8 value);

    qint8 y() const;
    void setY(qint8 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    qint8 x_ = 0;
    qint8 y_ = 0;
};

class RNetDeviceHeartbeat : public RNetFrame
{
  public:
    explicit RNetDeviceHeartbeat(const CanFrame &frame, quint32 idMask);

    QByteArray payload() const;
    void setPayload(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    QByteArray payload_;
};

class RNetMotorMaxSpeed : public RNetFrame
{
  public:
    explicit RNetMotorMaxSpeed(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint8 percent() const;
    void setPercent(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint8 percent_ = 0;
};

class RNetUiInteraction : public RNetFrame
{
  public:
    explicit RNetUiInteraction(const CanFrame &frame, quint32 idMask);

    int module() const;
    void setModule(int value);

  protected:
    QString detailsString() const override;

  private:
    int module_ = 0;
};

class RNetLampCommand : public RNetFrame
{
  public:
    explicit RNetLampCommand(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    QString command() const;
    void setCommand(const QString &value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    QString command_;
};

class RNetLampStatus : public RNetFrame
{
  public:
    explicit RNetLampStatus(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint8 mask() const;
    void setMask(quint8 value);

    quint8 bitmap() const;
    void setBitmap(quint8 value);

    bool left() const;
    void setLeft(bool value);

    bool right() const;
    void setRight(bool value);

    bool flood() const;
    void setFlood(bool value);

    bool hazard() const;
    void setHazard(bool value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint8 mask_ = 0;
    quint8 bitmap_ = 0;
    bool left_ = false;
    bool right_ = false;
    bool flood_ = false;
    bool hazard_ = false;
};

class RNetHorn : public RNetFrame
{
  public:
    explicit RNetHorn(const CanFrame &frame, quint32 idMask);

    int origin() const;
    void setOrigin(int value);

    bool active() const;
    void setActive(bool value);

  protected:
    QString detailsString() const override;

  private:
    int origin_ = 0;
    bool active_ = false;
};

class RNetPmHeartbeat : public RNetFrame
{
  public:
    explicit RNetPmHeartbeat(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint8 valueByte() const;
    void setValueByte(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint8 valueByte_ = 0;
};

class RNetPmConnected : public RNetFrame
{
  public:
    explicit RNetPmConnected(const CanFrame &frame, quint32 idMask);

    quint8 valueByte() const;
    void setValueByte(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    quint8 valueByte_ = 0;
};

class RNetPmMotorState : public RNetFrame
{
  public:
    explicit RNetPmMotorState(const CanFrame &frame, quint32 idMask);

    QString state() const;
    void setState(const QString &value);

    QByteArray payload() const;
    void setPayload(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    QString state_;
    QByteArray payload_;
};

class RNetPlayTone : public RNetFrame
{
  public:
    explicit RNetPlayTone(const CanFrame &frame, quint32 idMask);

    quint8 duration1() const;
    void setDuration1(quint8 value);
    quint8 note1() const;
    void setNote1(quint8 value);

    quint8 duration2() const;
    void setDuration2(quint8 value);
    quint8 note2() const;
    void setNote2(quint8 value);

    quint8 duration3() const;
    void setDuration3(quint8 value);
    quint8 note3() const;
    void setNote3(quint8 value);

    quint8 duration4() const;
    void setDuration4(quint8 value);
    quint8 note4() const;
    void setNote4(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    quint8 duration1_ = 0;
    quint8 note1_ = 0;
    quint8 duration2_ = 0;
    quint8 note2_ = 0;
    quint8 duration3_ = 0;
    quint8 note3_ = 0;
    quint8 duration4_ = 0;
    quint8 note4_ = 0;
};

class RNetDriveMotorCurrent : public RNetFrame
{
  public:
    explicit RNetDriveMotorCurrent(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint16 currentRaw() const;
    void setCurrentRaw(quint16 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint16 currentRaw_ = 0;
};

class RNetBatteryLevel : public RNetFrame
{
  public:
    explicit RNetBatteryLevel(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint8 percent() const;
    void setPercent(quint8 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint8 percent_ = 0;
};

class RNetDistanceCounter : public RNetFrame
{
  public:
    explicit RNetDistanceCounter(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    quint32 leftCounter() const;
    void setLeftCounter(quint32 value);

    quint32 rightCounter() const;
    void setRightCounter(quint32 value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    quint32 leftCounter_ = 0;
    quint32 rightCounter_ = 0;
};

class RNetTimeOfDay : public RNetFrame
{
  public:
    explicit RNetTimeOfDay(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

    QByteArray rawTime() const;
    void setRawTime(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
    QByteArray rawTime_;
};

class RNetReady : public RNetFrame
{
  public:
    explicit RNetReady(const CanFrame &frame, quint32 idMask);

    int device() const;
    void setDevice(int value);

  protected:
    QString detailsString() const override;

  private:
    int device_ = 0;
};

class RNetEnableMotorOutputFamily : public RNetFrame
{
  public:
    explicit RNetEnableMotorOutputFamily(const CanFrame &frame, quint32 idMask);

    quint8 familyCode() const;
    void setFamilyCode(quint8 value);

    QByteArray payload() const;
    void setPayload(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    quint8 familyCode_ = 0;
    QByteArray payload_;
};

class RNetBlockTransferData : public RNetFrame
{
  public:
    explicit RNetBlockTransferData(const CanFrame &frame, quint32 idMask);

    quint16 sequence() const;
    void setSequence(quint16 value);

    QByteArray payload() const;
    void setPayload(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    quint16 sequence_ = 0;
    QByteArray payload_;
};

class RNetBlockTransferTail : public RNetFrame
{
  public:
    explicit RNetBlockTransferTail(const CanFrame &frame, quint32 idMask);

    quint16 sequence() const;
    void setSequence(quint16 value);

    QByteArray payload() const;
    void setPayload(const QByteArray &value);

  protected:
    QString detailsString() const override;

  private:
    quint16 sequence_ = 0;
    QByteArray payload_;
};

class RNetBlockTransferAck : public RNetFrame
{
  public:
    explicit RNetBlockTransferAck(const CanFrame &frame, quint32 idMask);

    quint16 sequence() const;
    void setSequence(quint16 value);

  protected:
    QString detailsString() const override;

  private:
    quint16 sequence_ = 0;
};

class RNetUnknownFrame : public RNetFrame
{
  public:
    RNetUnknownFrame(const CanFrame &frame, quint32 idMask) : RNetFrame(frame, "Unknown", idMask) {}
};
