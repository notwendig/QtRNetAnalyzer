#include "rnetframe.h"

#include <QStringList>

RNetFrame::RNetFrame(const CanFrame &canframe, const char *name, quint32 idMask)
    : CanFrame(canframe)
    , name_(QString::fromLatin1(name))
    , idMask_(idMask)
{
}

const QString &RNetFrame::toString() const
{
    const QString details = detailsString();
    displayCache_ = details.isEmpty() ? name_ : (name_ + QLatin1Char(':') + details);
    return displayCache_;
}

QString RNetFrame::name() const
{
    return name_;
}

QString RNetFrame::detailsString() const
{
    return QString();
}

quint8 RNetFrame::byteAt(const QByteArray &data, int index)
{
    if (index < 0 || index >= data.size())
        return 0;
    return static_cast<quint8>(data.at(index));
}

quint16 RNetFrame::le16At(const QByteArray &data, int index)
{
    return quint16(byteAt(data, index)) | (quint16(byteAt(data, index + 1)) << 8);
}

quint32 RNetFrame::le32At(const QByteArray &data, int index)
{
    return quint32(byteAt(data, index))
    | (quint32(byteAt(data, index + 1)) << 8)
        | (quint32(byteAt(data, index + 2)) << 16)
        | (quint32(byteAt(data, index + 3)) << 24);
}

QString RNetFrame::hexByte(quint8 value)
{
    return QStringLiteral("0x%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
}

QString RNetFrame::hexBytes(const QByteArray &data)
{
    return data.isEmpty() ? QStringLiteral("-") : data.toHex(' ').toUpper();
}


std::unique_ptr<RNetFrame> RNetFrame::decodeRNetMessage(const CanFrame &frame)
{
    const quint32 id = frame.id;
    const QByteArray &d = frame.data;

    if (!frame.extended)
    {
        if (frame.remote && (id == 0x000u || id == 0x002u))
            return std::make_unique<RNetSleepAllDevices>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x000u)
            return std::make_unique<RNetJsmSleeping>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x004u)
            return std::make_unique<RNetJsmSleepCommencing>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x00Cu)
            return std::make_unique<RNetJsmCanBusTest>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && ((id & 0x7F0u) == 0x040u))
        {
            if (!d.isEmpty() && u8(d, 0) == 0x80u)
                return std::make_unique<RNetJsmEndParameterExchange>(frame, static_cast<quint32>(0xFFFFFFFF));
            return std::make_unique<RNetJsmSelectModeMap>(frame, static_cast<quint32>(0xFFFFFFFF));
        }

        if (!frame.remote && id == 0x7B3u)
            return std::make_unique<RNetSerialExchangeRequest>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (frame.remote && id == 0x7B3u)
            return std::make_unique<RNetSerialExchangeReplyRtr>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x7B1u)
            return std::make_unique<RNetDropToConfigMode1>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x7B0u)
            return std::make_unique<RNetDropToConfigMode0>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && ((id & 0x7F0u) == 0x780u))
            return std::make_unique<RNetParameterRequest>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && ((id & 0x7F0u) == 0x790u))
            return std::make_unique<RNetParameterReply>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x00Eu)
            return std::make_unique<RNetJsmUniqueId>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x051u)
            return std::make_unique<RNetModeSelectProfile>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x050u)
            return std::make_unique<RNetModeResponse50>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x061u)
            return std::make_unique<RNetModeSelectOrSuspend>(frame, static_cast<quint32>(0xFFFFFFFF));

        if (!frame.remote && id == 0x060u)
            return std::make_unique<RNetModeResponse60>(frame, static_cast<quint32>(0xFFFFFFFF));

        return std::make_unique<RNetUnknownFrame>(frame, static_cast<quint32>(0xFFFFFFFF));
    }

    if ((id & 0x2FFF0FFFu) == 0x02000000u || (id & 0x2FFF0FFFu) == 0x02000300u)
        return std::make_unique<RNetJoystickPosition>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x03C30F0Fu)
        return std::make_unique<RNetDeviceHeartbeat>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x0A040000u)
        return std::make_unique<RNetMotorMaxSpeed>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x0C000205u || id == 0x0C000301u || id == 0x0C000303u)
        return std::make_unique<RNetUiInteraction>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x0C000401u || id == 0x0C000402u || id == 0x0C000403u || id == 0x0C000404u)
        return std::make_unique<RNetLampCommand>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x0C000E00u)
        return std::make_unique<RNetLampStatus>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FEu) == 0x0C040000u)
        return std::make_unique<RNetHorn>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x0C140000u)
        return std::make_unique<RNetPmHeartbeat>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x0C280000u)
        return std::make_unique<RNetPmConnected>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x0C000005u || id == 0x0C000006u || (id & 0xFFFFF0FFu) == 0x06000000u)
        return std::make_unique<RNetPmMotorState>(frame, static_cast<quint32>(0xFFFFFFFF));

    if (id == 0x181C0D00u || id == 0x181C0100u)
        return std::make_unique<RNetPlayTone>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x14300000u)
        return std::make_unique<RNetDriveMotorCurrent>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x1C0C0000u)
        return std::make_unique<RNetBatteryLevel>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x1C300004u)
        return std::make_unique<RNetDistanceCounter>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x1C2C0000u)
        return std::make_unique<RNetTimeOfDay>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF0FFu) == 0x1C240001u)
        return std::make_unique<RNetReady>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFFF000u) == 0x0C180000u)
        return std::make_unique<RNetEnableMotorOutputFamily>(frame, static_cast<quint32>(0xFFFFFFFF));

    if ((id & 0xFFFF0000u) == 0x1E420000u)
        return std::make_unique<RNetBlockTransferData>(frame, static_cast<quint32>(0xFFFF0000));

    if ((id & 0xFFFF0000u) == 0x1E430000u)
        return std::make_unique<RNetBlockTransferTail>(frame, static_cast<quint32>(0xFFFF0000));

    if ((id & 0xFFFF0000u) == 0x1E3F0000u)
        return std::make_unique<RNetBlockTransferAck>(frame, static_cast<quint32>(0xFFFF0000));

    return std::make_unique<RNetUnknownFrame>(frame, static_cast<quint32>(0xFFFFFFFF));
}

// ---- simple standard frames with one known parameter ----

RNetJsmEndParameterExchange::RNetJsmEndParameterExchange(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJsmEndParameterExchange", idMask)
    , modeMap_(int(frame.id & 0x0F))
{
}

int RNetJsmEndParameterExchange::modeMap() const { return modeMap_; }
void RNetJsmEndParameterExchange::setModeMap(int value) { modeMap_ = value; }
QString RNetJsmEndParameterExchange::detailsString() const
{
    return QStringLiteral("modeMap=%1").arg(modeMap_);
}

RNetJsmSelectModeMap::RNetJsmSelectModeMap(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJsmSelectModeMap", idMask)
    , modeMap_(int(frame.id & 0x0F))
{
}

int RNetJsmSelectModeMap::modeMap() const { return modeMap_; }
void RNetJsmSelectModeMap::setModeMap(int value) { modeMap_ = value; }
QString RNetJsmSelectModeMap::detailsString() const
{
    return QStringLiteral("modeMap=%1").arg(modeMap_);
}

RNetParameterRequest::RNetParameterRequest(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetParameterRequest", idMask)
    , module_(int(frame.id & 0x0F))
    , opcodeByte_(byteAt(frame.data, 0))
    , parameterByte_(byteAt(frame.data, 1))
    , commandByte_(byteAt(frame.data, 2))
    , pointer_(byteAt(frame.data, 4))
    , subIndex_(byteAt(frame.data, 6))
    , value16_(quint16(byteAt(frame.data, 4)) | (quint16(byteAt(frame.data, 5)) << 8))
{
}

int RNetParameterRequest::module() const { return module_; }
void RNetParameterRequest::setModule(int value) { module_ = value; }
quint8 RNetParameterRequest::opcodeByte() const { return opcodeByte_; }
void RNetParameterRequest::setOpcodeByte(quint8 value) { opcodeByte_ = value; }
quint8 RNetParameterRequest::parameterByte() const { return parameterByte_; }
void RNetParameterRequest::setParameterByte(quint8 value) { parameterByte_ = value; }
quint8 RNetParameterRequest::commandByte() const { return commandByte_; }
void RNetParameterRequest::setCommandByte(quint8 value) { commandByte_ = value; }
quint8 RNetParameterRequest::pointer() const { return pointer_; }
void RNetParameterRequest::setPointer(quint8 value) { pointer_ = value; }
quint8 RNetParameterRequest::subIndex() const { return subIndex_; }
void RNetParameterRequest::setSubIndex(quint8 value) { subIndex_ = value; }
quint16 RNetParameterRequest::value16() const { return value16_; }
void RNetParameterRequest::setValue16(quint16 value) { value16_ = value; }
QString RNetParameterRequest::detailsString() const
{
    return QStringLiteral("module=%1,op=%2,param=%3,cmd=%4,pointer=%5,sub=%6,value16=%7")
    .arg(module_)
        .arg(hexByte(opcodeByte_))
        .arg(hexByte(parameterByte_))
        .arg(hexByte(commandByte_))
        .arg(hexByte(pointer_))
        .arg(hexByte(subIndex_))
        .arg(value16_);
}

RNetParameterReply::RNetParameterReply(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetParameterReply", idMask)
    , module_(int(frame.id & 0x0F))
    , opcodeByte_(byteAt(frame.data, 0))
    , parameterByte_(byteAt(frame.data, 1))
    , commandByte_(byteAt(frame.data, 2))
    , pointer_(byteAt(frame.data, 4))
    , subIndex_(byteAt(frame.data, 6))
    , value16_(quint16(byteAt(frame.data, 4)) | (quint16(byteAt(frame.data, 5)) << 8))
{
}

int RNetParameterReply::module() const { return module_; }
void RNetParameterReply::setModule(int value) { module_ = value; }
quint8 RNetParameterReply::opcodeByte() const { return opcodeByte_; }
void RNetParameterReply::setOpcodeByte(quint8 value) { opcodeByte_ = value; }
quint8 RNetParameterReply::parameterByte() const { return parameterByte_; }
void RNetParameterReply::setParameterByte(quint8 value) { parameterByte_ = value; }
quint8 RNetParameterReply::commandByte() const { return commandByte_; }
void RNetParameterReply::setCommandByte(quint8 value) { commandByte_ = value; }
quint8 RNetParameterReply::pointer() const { return pointer_; }
void RNetParameterReply::setPointer(quint8 value) { pointer_ = value; }
quint8 RNetParameterReply::subIndex() const { return subIndex_; }
void RNetParameterReply::setSubIndex(quint8 value) { subIndex_ = value; }
quint16 RNetParameterReply::value16() const { return value16_; }
void RNetParameterReply::setValue16(quint16 value) { value16_ = value; }
QString RNetParameterReply::detailsString() const
{
    return QStringLiteral("module=%1,op=%2,param=%3,cmd=%4,pointer=%5,sub=%6,value16=%7")
    .arg(module_)
        .arg(hexByte(opcodeByte_))
        .arg(hexByte(parameterByte_))
        .arg(hexByte(commandByte_))
        .arg(hexByte(pointer_))
        .arg(hexByte(subIndex_))
        .arg(value16_);
}

RNetJsmUniqueId::RNetJsmUniqueId(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJsmUniqueId", idMask)
    , uniqueId_(frame.data)
{
}

QByteArray RNetJsmUniqueId::uniqueId() const { return uniqueId_; }
void RNetJsmUniqueId::setUniqueId(const QByteArray &value) { uniqueId_ = value; }
QString RNetJsmUniqueId::detailsString() const
{
    return QStringLiteral("uid=%1").arg(hexBytes(uniqueId_));
}

RNetModeSelectProfile::RNetModeSelectProfile(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeSelectProfile", idMask)
    , profile_(int(byteAt(frame.data, 1) & 0x0F))
{
}

int RNetModeSelectProfile::profile() const { return profile_; }
void RNetModeSelectProfile::setProfile(int value) { profile_ = value; }
QString RNetModeSelectProfile::detailsString() const
{
    return QStringLiteral("profile=%1").arg(profile_);
}

RNetModeResponse50::RNetModeResponse50(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeResponse50", idMask)
    , status_(byteAt(frame.data, 0))
    , mode_(int(byteAt(frame.data, 1) & 0x0F))
    , valueByte_(byteAt(frame.data, 3))
{
}

quint8 RNetModeResponse50::status() const { return status_; }
void RNetModeResponse50::setStatus(quint8 value) { status_ = value; }
int RNetModeResponse50::mode() const { return mode_; }
void RNetModeResponse50::setMode(int value) { mode_ = value; }
quint8 RNetModeResponse50::valueByte() const { return valueByte_; }
void RNetModeResponse50::setValueByte(quint8 value) { valueByte_ = value; }
QString RNetModeResponse50::detailsString() const
{
    return QStringLiteral("status=%1,mode=%2,value=%3")
    .arg(hexByte(status_))
        .arg(mode_)
        .arg(hexByte(valueByte_));
}

RNetModeSelectOrSuspend::RNetModeSelectOrSuspend(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeSelectOrSuspend", idMask)
    , suspend_(byteAt(frame.data, 0) == 0x40)
    , mode_(int(byteAt(frame.data, 1) & 0x0F))
{
}

bool RNetModeSelectOrSuspend::suspend() const { return suspend_; }
void RNetModeSelectOrSuspend::setSuspend(bool value) { suspend_ = value; }
int RNetModeSelectOrSuspend::mode() const { return mode_; }
void RNetModeSelectOrSuspend::setMode(int value) { mode_ = value; }
QString RNetModeSelectOrSuspend::detailsString() const
{
    return QStringLiteral("mode=%1,action=%2").arg(mode_).arg(suspend_ ? "suspend" : "select");
}

RNetModeResponse60::RNetModeResponse60(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeResponse60", idMask)
    , status_(byteAt(frame.data, 0))
    , mode_(int(byteAt(frame.data, 1) & 0x0F))
    , valueByte_(byteAt(frame.data, 3))
{
}

quint8 RNetModeResponse60::status() const { return status_; }
void RNetModeResponse60::setStatus(quint8 value) { status_ = value; }
int RNetModeResponse60::mode() const { return mode_; }
void RNetModeResponse60::setMode(int value) { mode_ = value; }
quint8 RNetModeResponse60::valueByte() const { return valueByte_; }
void RNetModeResponse60::setValueByte(quint8 value) { valueByte_ = value; }
QString RNetModeResponse60::detailsString() const
{
    return QStringLiteral("status=%1,mode=%2,value=%3")
    .arg(hexByte(status_))
        .arg(mode_)
        .arg(hexByte(valueByte_));
}

// ---- extended frames ----

RNetJoystickPosition::RNetJoystickPosition(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJoystickPosition", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , x_(static_cast<qint8>(byteAt(frame.data, 0)))
    , y_(static_cast<qint8>(byteAt(frame.data, 1)))
{
}

int RNetJoystickPosition::device() const { return device_; }
void RNetJoystickPosition::setDevice(int value) { device_ = value; }
qint8 RNetJoystickPosition::x() const { return x_; }
void RNetJoystickPosition::setX(qint8 value) { x_ = value; }
qint8 RNetJoystickPosition::y() const { return y_; }
void RNetJoystickPosition::setY(qint8 value) { y_ = value; }
QString RNetJoystickPosition::detailsString() const
{
    return QStringLiteral("device=%1,x=%2,y=%3").arg(device_).arg(x_).arg(y_);
}

RNetDeviceHeartbeat::RNetDeviceHeartbeat(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetDeviceHeartbeat", idMask)
    , payload_(frame.data)
{
}

QByteArray RNetDeviceHeartbeat::payload() const { return payload_; }
void RNetDeviceHeartbeat::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetDeviceHeartbeat::detailsString() const
{
    return QStringLiteral("payload=%1").arg(hexBytes(payload_));
}

RNetMotorMaxSpeed::RNetMotorMaxSpeed(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetMotorMaxSpeed", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , percent_(byteAt(frame.data, 0))
{
}

int RNetMotorMaxSpeed::device() const { return device_; }
void RNetMotorMaxSpeed::setDevice(int value) { device_ = value; }
quint8 RNetMotorMaxSpeed::percent() const { return percent_; }
void RNetMotorMaxSpeed::setPercent(quint8 value) { percent_ = value; }
QString RNetMotorMaxSpeed::detailsString() const
{
    return QStringLiteral("device=%1,percent=%2").arg(device_).arg(percent_);
}

RNetUiInteraction::RNetUiInteraction(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetUiInteraction", idMask)
    , module_(int(frame.id & 0xFF))
{
}

int RNetUiInteraction::module() const { return module_; }
void RNetUiInteraction::setModule(int value) { module_ = value; }
QString RNetUiInteraction::detailsString() const
{
    return QStringLiteral("module=%1").arg(module_);
}

RNetLampCommand::RNetLampCommand(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetLampCommand", idMask)
    , device_(int(frame.id & 0xFF))
{
    switch (frame.id & 0xFF)
    {
    case 0x01: command_ = QStringLiteral("left"); break;
    case 0x02: command_ = QStringLiteral("right"); break;
    case 0x03: command_ = QStringLiteral("hazard"); break;
    case 0x04: command_ = QStringLiteral("flood"); break;
    default:   command_ = QStringLiteral("unknown"); break;
    }
}

int RNetLampCommand::device() const { return device_; }
void RNetLampCommand::setDevice(int value) { device_ = value; }
QString RNetLampCommand::command() const { return command_; }
void RNetLampCommand::setCommand(const QString &value) { command_ = value; }
QString RNetLampCommand::detailsString() const
{
    return QStringLiteral("device=%1,command=%2").arg(device_).arg(command_);
}

RNetLampStatus::RNetLampStatus(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetLampStatus", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , mask_(byteAt(frame.data, 0))
    , bitmap_(byteAt(frame.data, 1))
    , left_((bitmap_ & 0x01u) != 0u)
    , right_((bitmap_ & 0x04u) != 0u)
    , flood_((bitmap_ & 0x80u) != 0u)
    , hazard_((bitmap_ & 0x10u) != 0u)
{
}

int RNetLampStatus::device() const { return device_; }
void RNetLampStatus::setDevice(int value) { device_ = value; }
quint8 RNetLampStatus::mask() const { return mask_; }
void RNetLampStatus::setMask(quint8 value) { mask_ = value; }
quint8 RNetLampStatus::bitmap() const { return bitmap_; }
void RNetLampStatus::setBitmap(quint8 value) { bitmap_ = value; }
bool RNetLampStatus::left() const { return left_; }
void RNetLampStatus::setLeft(bool value) { left_ = value; }
bool RNetLampStatus::right() const { return right_; }
void RNetLampStatus::setRight(bool value) { right_ = value; }
bool RNetLampStatus::flood() const { return flood_; }
void RNetLampStatus::setFlood(bool value) { flood_ = value; }
bool RNetLampStatus::hazard() const { return hazard_; }
void RNetLampStatus::setHazard(bool value) { hazard_ = value; }
QString RNetLampStatus::detailsString() const
{
    return QStringLiteral("device=%1,mask=%2,bitmap=%3,left=%4,right=%5,flood=%6,hazard=%7")
    .arg(device_)
        .arg(hexByte(mask_))
        .arg(hexByte(bitmap_))
        .arg(left_ ? 1 : 0)
        .arg(right_ ? 1 : 0)
        .arg(flood_ ? 1 : 0)
        .arg(hazard_ ? 1 : 0);
}

RNetHorn::RNetHorn(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetHorn", idMask)
    , origin_(int((frame.id >> 8) & 0x0F))
    , active_(((frame.id & 0x01u) == 0u))
{
}

int RNetHorn::origin() const { return origin_; }
void RNetHorn::setOrigin(int value) { origin_ = value; }
bool RNetHorn::active() const { return active_; }
void RNetHorn::setActive(bool value) { active_ = value; }
QString RNetHorn::detailsString() const
{
    return QStringLiteral("origin=%1,active=%2").arg(origin_).arg(active_ ? 1 : 0);
}

RNetPmHeartbeat::RNetPmHeartbeat(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPmHeartbeat", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , valueByte_(byteAt(frame.data, 0))
{
}

int RNetPmHeartbeat::device() const { return device_; }
void RNetPmHeartbeat::setDevice(int value) { device_ = value; }
quint8 RNetPmHeartbeat::valueByte() const { return valueByte_; }
void RNetPmHeartbeat::setValueByte(quint8 value) { valueByte_ = value; }
QString RNetPmHeartbeat::detailsString() const
{
    return QStringLiteral("device=%1,value=%2").arg(device_).arg(hexByte(valueByte_));
}

RNetPmConnected::RNetPmConnected(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPmConnected", idMask)
    , valueByte_(byteAt(frame.data, 0))
{
}

quint8 RNetPmConnected::valueByte() const { return valueByte_; }
void RNetPmConnected::setValueByte(quint8 value) { valueByte_ = value; }
QString RNetPmConnected::detailsString() const
{
    return QStringLiteral("value=%1").arg(hexByte(valueByte_));
}

RNetPmMotorState::RNetPmMotorState(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPmMotorState", idMask)
    , payload_(frame.data)
{
    if (frame.id == 0x0C000005u)
        state_ = QStringLiteral("stopped");
    else if (frame.id == 0x0C000006u)
        state_ = QStringLiteral("decelerating");
    else if (payload_ == QByteArray::fromHex("90010000"))
        state_ = QStringLiteral("chairStopped");
    else if (payload_ == QByteArray::fromHex("90010040"))
        state_ = QStringLiteral("chairRunning");
    else
        state_ = QStringLiteral("unknown");
}

QString RNetPmMotorState::state() const { return state_; }
void RNetPmMotorState::setState(const QString &value) { state_ = value; }
QByteArray RNetPmMotorState::payload() const { return payload_; }
void RNetPmMotorState::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetPmMotorState::detailsString() const
{
    return QStringLiteral("state=%1,payload=%2").arg(state_).arg(hexBytes(payload_));
}

RNetPlayTone::RNetPlayTone(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPlayTone", idMask)
    , duration1_(byteAt(frame.data, 0))
    , note1_(byteAt(frame.data, 1))
    , duration2_(byteAt(frame.data, 2))
    , note2_(byteAt(frame.data, 3))
    , duration3_(byteAt(frame.data, 4))
    , note3_(byteAt(frame.data, 5))
    , duration4_(byteAt(frame.data, 6))
    , note4_(byteAt(frame.data, 7))
{
}

quint8 RNetPlayTone::duration1() const { return duration1_; }
void RNetPlayTone::setDuration1(quint8 value) { duration1_ = value; }
quint8 RNetPlayTone::note1() const { return note1_; }
void RNetPlayTone::setNote1(quint8 value) { note1_ = value; }
quint8 RNetPlayTone::duration2() const { return duration2_; }
void RNetPlayTone::setDuration2(quint8 value) { duration2_ = value; }
quint8 RNetPlayTone::note2() const { return note2_; }
void RNetPlayTone::setNote2(quint8 value) { note2_ = value; }
quint8 RNetPlayTone::duration3() const { return duration3_; }
void RNetPlayTone::setDuration3(quint8 value) { duration3_ = value; }
quint8 RNetPlayTone::note3() const { return note3_; }
void RNetPlayTone::setNote3(quint8 value) { note3_ = value; }
quint8 RNetPlayTone::duration4() const { return duration4_; }
void RNetPlayTone::setDuration4(quint8 value) { duration4_ = value; }
quint8 RNetPlayTone::note4() const { return note4_; }
void RNetPlayTone::setNote4(quint8 value) { note4_ = value; }
QString RNetPlayTone::detailsString() const
{
    return QStringLiteral("d1=%1,n1=%2,d2=%3,n2=%4,d3=%5,n3=%6,d4=%7,n4=%8")
    .arg(duration1_).arg(note1_)
        .arg(duration2_).arg(note2_)
        .arg(duration3_).arg(note3_)
        .arg(duration4_).arg(note4_);
}

RNetDriveMotorCurrent::RNetDriveMotorCurrent(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetDriveMotorCurrent", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , currentRaw_(le16At(frame.data, 0))
{
}

int RNetDriveMotorCurrent::device() const { return device_; }
void RNetDriveMotorCurrent::setDevice(int value) { device_ = value; }
quint16 RNetDriveMotorCurrent::currentRaw() const { return currentRaw_; }
void RNetDriveMotorCurrent::setCurrentRaw(quint16 value) { currentRaw_ = value; }
QString RNetDriveMotorCurrent::detailsString() const
{
    return QStringLiteral("device=%1,currentRaw=%2").arg(device_).arg(currentRaw_);
}

RNetBatteryLevel::RNetBatteryLevel(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetBatteryLevel", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , percent_(byteAt(frame.data, 0))
{
}

int RNetBatteryLevel::device() const { return device_; }
void RNetBatteryLevel::setDevice(int value) { device_ = value; }
quint8 RNetBatteryLevel::percent() const { return percent_; }
void RNetBatteryLevel::setPercent(quint8 value) { percent_ = value; }
QString RNetBatteryLevel::detailsString() const
{
    return QStringLiteral("device=%1,percent=%2").arg(device_).arg(percent_);
}

RNetDistanceCounter::RNetDistanceCounter(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetDistanceCounter", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , leftCounter_(le32At(frame.data, 0))
    , rightCounter_(le32At(frame.data, 4))
{
}

int RNetDistanceCounter::device() const { return device_; }
void RNetDistanceCounter::setDevice(int value) { device_ = value; }
quint32 RNetDistanceCounter::leftCounter() const { return leftCounter_; }
void RNetDistanceCounter::setLeftCounter(quint32 value) { leftCounter_ = value; }
quint32 RNetDistanceCounter::rightCounter() const { return rightCounter_; }
void RNetDistanceCounter::setRightCounter(quint32 value) { rightCounter_ = value; }
QString RNetDistanceCounter::detailsString() const
{
    return QStringLiteral("device=%1,left=%2,right=%3").arg(device_).arg(leftCounter_).arg(rightCounter_);
}

RNetTimeOfDay::RNetTimeOfDay(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetTimeOfDay", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , rawTime_(frame.data)
{
}

int RNetTimeOfDay::device() const { return device_; }
void RNetTimeOfDay::setDevice(int value) { device_ = value; }
QByteArray RNetTimeOfDay::rawTime() const { return rawTime_; }
void RNetTimeOfDay::setRawTime(const QByteArray &value) { rawTime_ = value; }
QString RNetTimeOfDay::detailsString() const
{
    return QStringLiteral("device=%1,raw=%2").arg(device_).arg(hexBytes(rawTime_));
}

RNetReady::RNetReady(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetReady", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
{
}

int RNetReady::device() const { return device_; }
void RNetReady::setDevice(int value) { device_ = value; }
QString RNetReady::detailsString() const
{
    return QStringLiteral("device=%1").arg(device_);
}

RNetEnableMotorOutputFamily::RNetEnableMotorOutputFamily(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetEnableMotorOutputFamily", idMask)
    , familyCode_(byteAt(frame.data, 0))
    , payload_(frame.data)
{
}

quint8 RNetEnableMotorOutputFamily::familyCode() const { return familyCode_; }
void RNetEnableMotorOutputFamily::setFamilyCode(quint8 value) { familyCode_ = value; }
QByteArray RNetEnableMotorOutputFamily::payload() const { return payload_; }
void RNetEnableMotorOutputFamily::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetEnableMotorOutputFamily::detailsString() const
{
    return QStringLiteral("familyCode=%1,payload=%2").arg(hexByte(familyCode_)).arg(hexBytes(payload_));
}

RNetBlockTransferData::RNetBlockTransferData(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetBlockTransferData", idMask)
    , sequence_(quint16(frame.id & 0xFFFFu))
    , payload_(frame.data)
{
}

quint16 RNetBlockTransferData::sequence() const { return sequence_; }
void RNetBlockTransferData::setSequence(quint16 value) { sequence_ = value; }
QByteArray RNetBlockTransferData::payload() const { return payload_; }
void RNetBlockTransferData::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetBlockTransferData::detailsString() const
{
    return QStringLiteral("sequence=%1,payload=%2").arg(sequence_).arg(hexBytes(payload_));
}

RNetBlockTransferTail::RNetBlockTransferTail(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetBlockTransferTail", idMask)
    , sequence_(quint16(frame.id & 0xFFFFu))
    , payload_(frame.data)
{
}

quint16 RNetBlockTransferTail::sequence() const { return sequence_; }
void RNetBlockTransferTail::setSequence(quint16 value) { sequence_ = value; }
QByteArray RNetBlockTransferTail::payload() const { return payload_; }
void RNetBlockTransferTail::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetBlockTransferTail::detailsString() const
{
    return QStringLiteral("sequence=%1,payload=%2").arg(sequence_).arg(hexBytes(payload_));
}

RNetBlockTransferAck::RNetBlockTransferAck(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetBlockTransferAck", idMask)
    , sequence_(quint16(frame.id & 0xFFFFu))
{
}

quint16 RNetBlockTransferAck::sequence() const { return sequence_; }
void RNetBlockTransferAck::setSequence(quint16 value) { sequence_ = value; }
QString RNetBlockTransferAck::detailsString() const
{
    return QStringLiteral("sequence=%1").arg(sequence_);
}
