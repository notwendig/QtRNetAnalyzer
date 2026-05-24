#include "rnetframe.h"

#include <QStringList>

#include <array>
#include <memory>

namespace {

constexpr int RemoteAny = -1;
constexpr int RemoteData = 0;
constexpr int RemoteRtr = 1;

struct OpenRNetRule
{
    bool extended;
    int remote;
    quint32 mask;
    quint32 value;
    quint32 keyMask;
    const char *typeName;
    const char *category;
    const char *summary;
};

class RNetOpenRNetKnownFrame final : public RNetFrame
{
public:
    RNetOpenRNetKnownFrame(const CanFrame &frame, const OpenRNetRule &rule)
        : RNetFrame(frame, rule.typeName, rule.keyMask)
        , category_(QString::fromLatin1(rule.category))
        , summary_(QString::fromLatin1(rule.summary))
        , mask_(rule.mask)
        , value_(rule.value)
    {
    }

protected:
    QString detailsString() const override
    {
        QStringList parts;
        if (!category_.isEmpty())
            parts << QStringLiteral("category=%1").arg(category_);
        if (!summary_.isEmpty())
            parts << summary_;
        parts << QStringLiteral("pattern=0x%1/0x%2")
                     .arg(value_, 0, 16)
                     .arg(mask_, 0, 16)
                     .toUpper();
        if (!data.isEmpty())
            parts << QStringLiteral("data=%1").arg(hexBytes(data));
        if (remote)
            parts << QStringLiteral("RTR");
        return parts.join(QStringLiteral(", "));
    }

private:
    QString category_;
    QString summary_;
    quint32 mask_ = 0;
    quint32 value_ = 0;
};

constexpr std::array<OpenRNetRule, 49> kOpenRNetRules {{
    // Standard 11-bit frames from redragonx/open-rnet reference/RNET_FRAME_DICTIONARY.md.
    {false, RemoteRtr,  0x7FFu,      0x004u,      0xFFFFFFFFu, "RNetSleepWakeRtr",             "power",        "sleep/wake sequence RTR"},
    {false, RemoteData, 0x7FFu,      0x002u,      0xFFFFFFFFu, "RNetJsmInitSeen",              "power",        "seen during JSM init"},
    {false, RemoteRtr,  0x7FFu,      0x7B1u,      0xFFFFFFFFu, "RNetConfigMode1Request",       "config",       "PM requests drop to config mode 1"},
    {false, RemoteRtr,  0x7FFu,      0x7B0u,      0xFFFFFFFFu, "RNetConfigMode0Request",       "config",       "PM requests drop to config mode 0"},
    {false, RemoteAny,  0x7FFu,      0x7E0u,      0xFFFFFFFFu, "RNetDiagnosticsReserved7E0",   "config",       "unknown diagnostics/reserved frame reported from DongleInterface.dll data"},
    {false, RemoteData, 0x7F0u,      0x060u,      0xFFFFFFF0u, "RNetStdMotorOrModeStatus",     "motor",        "standard 06X status/mode response family"},
    {false, RemoteData, 0x7FFu,      0x78Fu,      0xFFFFFFFFu, "RNetProgrammerQuickRequest",   "pop-quick",    "R-Net Programmer POP Quick request"},
    {false, RemoteData, 0x7FFu,      0x793u,      0xFFFFFFFFu, "RNetProgrammerQuickResponse",  "pop-quick",    "R-Net Programmer POP Quick response"},

    // Extended serial/authentication frames.
    {true,  RemoteAny,  0x1F00F000u, 0x1F000000u, 0x1F00F000u, "RNetSerialExchangeSeq0",      "serial",       "extended serial challenge/response sequence 0"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F001000u, 0x1F00F000u, "RNetSerialExchangeSeq1",      "serial",       "extended serial challenge/response sequence 1"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F002000u, 0x1F00F000u, "RNetSerialExchangeSeq2",      "serial",       "extended serial challenge/response sequence 2"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F003000u, 0x1F00F000u, "RNetSerialExchangeSeq3",      "serial",       "extended serial challenge/response sequence 3"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F004000u, 0x1F00F000u, "RNetSerialExchangeSeq4",      "serial",       "extended serial challenge/response sequence 4"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F005000u, 0x1F00F000u, "RNetSerialExchangeSeq5",      "serial",       "extended serial challenge/response sequence 5"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F006000u, 0x1F00F000u, "RNetSerialExchangeSeq6",      "serial",       "extended serial challenge/response sequence 6"},
    {true,  RemoteAny,  0x1F00F000u, 0x1F007000u, 0x1F00F000u, "RNetSerialExchangeSeq7",      "serial",       "extended serial challenge/response sequence 7"},
    {true,  RemoteData, 0x1FFFFF00u, 0x1F800000u, 0x1FFFFF00u, "RNetColorJsmSlot8CryForHelp", "serial",       "cJSM slot-8 authentication retry/cry-for-help family"},
    {true,  RemoteData, 0x1FFFFF00u, 0x1F900000u, 0x1FFFFF00u, "RNetColorJsmAuthResponse90", "serial",       "cJSM/PM authentication response family 0x1F90"},
    {true,  RemoteData, 0x1FFFFF00u, 0x1F910000u, 0x1FFFFF00u, "RNetColorJsmAuthResponse91", "serial",       "cJSM/PM authentication response family 0x1F91"},
    {true,  RemoteData, 0x1FFFFFF0u, 0x1FB00000u, 0x1FFFFFF0u, "RNetDeviceSerialSlot",       "serial",       "device slot serial-number broadcast"},

    // POP segmented/config transfer families.
    {true,  RemoteAny,  0x1FC00000u, 0x1E000000u, 0x1FC00000u, "RNetPopSegmentedResponse",   "pop-segmented","POP segmented response base family"},
    {true,  RemoteAny,  0x1FC00000u, 0x1E400000u, 0x1FC00000u, "RNetPopSegmentedRequest",    "pop-segmented","POP segmented request base family"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E3C0001u, 0xFFFFFFFFu, "RNetConfigTransferHeader1",  "config-xfer",  "configuration transfer header segment 1"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E3C0002u, 0xFFFFFFFFu, "RNetConfigTransferHeader2",  "config-xfer",  "configuration transfer header segment 2"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E3D0003u, 0xFFFFFFFFu, "RNetConfigTransferHeaderCrc","config-xfer",  "configuration transfer header CRC segment"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E4D0003u, 0xFFFFFFFFu, "RNetConfigTransferBlockEnd", "config-xfer",  "configuration transfer data block end"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E4E0001u, 0xFFFFFFFFu, "RNetConfigTransferData1",    "config-xfer",  "configuration transfer data segment 1"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E4F0002u, 0xFFFFFFFFu, "RNetConfigTransferDataCrc",  "config-xfer",  "configuration transfer data segment with CRC"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1E80000Fu, 0xFFFFFFFFu, "RNetConfigTransferComplete", "config-xfer",  "configuration transfer complete/status"},

    // Speed, BTM and unknown 0A40 families.
    {true,  RemoteData, 0xFFFFFF0Fu, 0x0A400D00u, 0xFFFFFF0Fu, "RNetSpeedOrBtUnknown0A400D", "speed-btm",    "unknown 0A400D0M status/control family"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0A400002u, 0xFFFFFFFFu, "RNetBtmStatus0002",         "btm",          "Bluetooth module status"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0A400102u, 0xFFFFFFFFu, "RNetBtmStatus0102",         "btm",          "Bluetooth module status"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0A400300u, 0xFFFFFFFFu, "RNetBtmControl0300",        "btm",          "Bluetooth module control"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0A400301u, 0xFFFFFFFFu, "RNetBtmControl0301",        "btm",          "Bluetooth module control"},

    // Lamp/UI/error/diagnostic families not covered by the old typed decoder.
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C000400u, 0xFFFFFFFFu, "RNetLampControlStatus",     "lighting",     "lamp control status bitmap"},
    {true,  RemoteData, 0xFFFFF0FFu, 0x0C000000u, 0xFFFFF0FFu, "RNetJsmNetworkErrorTrigger","error",        "frame family known to trigger JSM network error"},
    {true,  RemoteData, 0xFFFFFFF0u, 0x0C040D00u, 0xFFFFFFF0u, "RNetPeriodic0C040D0X",      "unknown",      "periodic 0C040D0X family"},
    {true,  RemoteData, 0xFFFFFF00u, 0x0C180100u, 0xFFFFFF00u, "RNetModuleStatus0C1801",    "module",       "module status/control family 0C1801xx"},
    {true,  RemoteData, 0xFFFFFF00u, 0x0C180200u, 0xFFFFFF00u, "RNetModuleStatus0C1802",    "module",       "module status/control family 0C1802xx"},
    {true,  RemoteData, 0xFFFFFF00u, 0x0C180400u, 0xFFFFFF00u, "RNetModuleStatus0C1804",    "module",       "module status/control family 0C1804xx"},
    {true,  RemoteData, 0xFFFFF0FFu, 0x140C0001u, 0xFFFFF0FFu, "RNetMotorUnknown140C",      "motor",        "unknown motor-related 140C0X01 family"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C000302u, 0xFFFFFFFFu, "RNetModule3Interaction302", "ui",           "module 3 interaction"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C000304u, 0xFFFFFFFFu, "RNetModule3Interaction304", "ui",           "module 3 interaction"},
    {true,  RemoteData, 0xFFFFF0FFu, 0x1C200000u, 0xFFFFF0FFu, "RNetJsmDisplayStatus",      "ui",           "JSM display/UI status family"},
    {true,  RemoteData, 0xFFFFF0FFu, 0x1C240000u, 0xFFFFF0FFu, "RNetPowerDownJsm",          "ui",           "PM requests JSM power down"},

    // Fallbacks for exact frame IDs listed by open-rnet.
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C180101u, 0xFFFFFFFFu, "RNetUnknown0C180101",       "unknown",      "unknown module status frame"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C180201u, 0xFFFFFFFFu, "RNetUnknown0C180201",       "unknown",      "unknown module status frame"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x0C180401u, 0xFFFFFFFFu, "RNetUnknown0C180401",       "unknown",      "unknown module status frame"},
    {true,  RemoteData, 0xFFFFFFFFu, 0x1FA00000u, 0xFFFFFFFFu, "RNetColorJsmUnknown1FA",    "serial",       "cJSM authentication/error-state frame"},
}};

std::unique_ptr<RNetFrame> decodeOpenRNetKnownFrame(const CanFrame &frame)
{
    for (const OpenRNetRule &rule : kOpenRNetRules) {
        if (frame.extended != rule.extended)
            continue;
        if (rule.remote == RemoteData && frame.remote)
            continue;
        if (rule.remote == RemoteRtr && !frame.remote)
            continue;
        if ((frame.id & rule.mask) != rule.value)
            continue;
        return std::make_unique<RNetOpenRNetKnownFrame>(frame, rule);
    }
    return nullptr;
}

QString boolText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString stateForLampCommand(quint32 id)
{
    switch (id & 0xFFu) {
    case 0x01u: return QStringLiteral("left-start");
    case 0x02u: return QStringLiteral("right-start");
    case 0x03u: return QStringLiteral("hazard-start");
    case 0x04u: return QStringLiteral("flood-start");
    default: return QStringLiteral("unknown");
    }
}

} // namespace

RNetFrame::RNetFrame(const CanFrame &canframe, const char *name, quint32 idmask)
    : CanFrame(canframe)
    , name_(QString::fromLatin1(name ? name : "RNetFrame"))
    , idMask_(idmask)
{
}

const QString &RNetFrame::toString() const
{
    const QString details = detailsString();
    displayCache_ = name_;
    if (!details.isEmpty())
        displayCache_ += QStringLiteral(" (") + details + QStringLiteral(")");
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
    return QStringLiteral("0x%1").arg(int(value), 2, 16, QLatin1Char('0')).toUpper();
}

QString RNetFrame::hexBytes(const QByteArray &data)
{
    return data.isEmpty() ? QStringLiteral("-") : data.toHex(' ').toUpper();
}

std::unique_ptr<RNetFrame> RNetFrame::decodeRNetMessage(const CanFrame &frame)
{
    const quint32 id = frame.id;
    const QByteArray &d = frame.data;

    if (!frame.extended) {
        if (frame.remote && (id == 0x000u || id == 0x002u))
            return std::make_unique<RNetSleepAllDevices>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x000u)
            return std::make_unique<RNetJsmSleeping>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x004u)
            return std::make_unique<RNetJsmSleepCommencing>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x00Cu)
            return std::make_unique<RNetJsmCanBusTest>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && ((id & 0x7F0u) == 0x040u)) {
            if (!d.isEmpty() && u8(d, 0) == 0x80u)
                return std::make_unique<RNetJsmEndParameterExchange>(frame, static_cast<quint32>(0xFFFFFFFFu));
            return std::make_unique<RNetJsmSelectModeMap>(frame, static_cast<quint32>(0xFFFFFFFFu));
        }

        if (!frame.remote && id == 0x7B3u)
            return std::make_unique<RNetSerialExchangeRequest>(frame, static_cast<quint32>(0xFFFFFFFFu));
        if (frame.remote && id == 0x7B3u)
            return std::make_unique<RNetSerialExchangeReplyRtr>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x7B1u)
            return std::make_unique<RNetDropToConfigMode1>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x7B0u)
            return std::make_unique<RNetDropToConfigMode0>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && ((id & 0x7F0u) == 0x780u))
            return std::make_unique<RNetParameterRequest>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && ((id & 0x7F0u) == 0x790u))
            return std::make_unique<RNetParameterReply>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x00Eu)
            return std::make_unique<RNetJsmUniqueId>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x051u)
            return std::make_unique<RNetModeSelectProfile>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x050u)
            return std::make_unique<RNetModeResponse50>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x061u)
            return std::make_unique<RNetModeSelectOrSuspend>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (!frame.remote && id == 0x060u)
            return std::make_unique<RNetModeResponse60>(frame, static_cast<quint32>(0xFFFFFFFFu));

        if (auto openRNet = decodeOpenRNetKnownFrame(frame))
            return openRNet;

        return std::make_unique<RNetUnknownFrame>(frame, static_cast<quint32>(0xFFFFFFFFu));
    }

    if ((id & 0x2FFF0FFFu) == 0x02000000u || (id & 0x2FFF0FFFu) == 0x02000300u)
        return std::make_unique<RNetJoystickPosition>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x03C30F0Fu)
        return std::make_unique<RNetDeviceHeartbeat>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x0A040000u)
        return std::make_unique<RNetMotorMaxSpeed>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x0C000205u || id == 0x0C000301u || id == 0x0C000303u)
        return std::make_unique<RNetUiInteraction>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x0C000401u || id == 0x0C000402u || id == 0x0C000403u || id == 0x0C000404u)
        return std::make_unique<RNetLampCommand>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x0C000E00u)
        return std::make_unique<RNetLampStatus>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FEu) == 0x0C040000u)
        return std::make_unique<RNetHorn>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x0C140000u)
        return std::make_unique<RNetPmHeartbeat>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x0C280000u)
        return std::make_unique<RNetPmConnected>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x0C000005u || id == 0x0C000006u || (id & 0xFFFFF0FFu) == 0x06000000u)
        return std::make_unique<RNetPmMotorState>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if (id == 0x181C0D00u || id == 0x181C0100u)
        return std::make_unique<RNetPlayTone>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x14300000u)
        return std::make_unique<RNetDriveMotorCurrent>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x1C0C0000u)
        return std::make_unique<RNetBatteryLevel>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x1C300004u)
        return std::make_unique<RNetDistanceCounter>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x1C2C0000u)
        return std::make_unique<RNetTimeOfDay>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF0FFu) == 0x1C240001u)
        return std::make_unique<RNetReady>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFFF000u) == 0x0C180000u)
        return std::make_unique<RNetEnableMotorOutputFamily>(frame, static_cast<quint32>(0xFFFFFFFFu));

    if ((id & 0xFFFF0000u) == 0x1E420000u)
        return std::make_unique<RNetBlockTransferData>(frame, static_cast<quint32>(0xFFFF0000u));

    if ((id & 0xFFFF0000u) == 0x1E430000u)
        return std::make_unique<RNetBlockTransferTail>(frame, static_cast<quint32>(0xFFFF0000u));

    if ((id & 0xFFFF0000u) == 0x1E3F0000u)
        return std::make_unique<RNetBlockTransferAck>(frame, static_cast<quint32>(0xFFFF0000u));

    if (auto openRNet = decodeOpenRNetKnownFrame(frame))
        return openRNet;

    return std::make_unique<RNetUnknownFrame>(frame, static_cast<quint32>(0xFFFFFFFFu));
}

RNetJsmEndParameterExchange::RNetJsmEndParameterExchange(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJsmEndParameterExchange", idMask)
    , modeMap_(int(frame.id & 0x0F))
{
}
int RNetJsmEndParameterExchange::modeMap() const { return modeMap_; }
void RNetJsmEndParameterExchange::setModeMap(int value) { modeMap_ = value; }
QString RNetJsmEndParameterExchange::detailsString() const { return QStringLiteral("modeMap=%1").arg(modeMap_); }

RNetJsmSelectModeMap::RNetJsmSelectModeMap(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetJsmSelectModeMap", idMask)
    , modeMap_(int(frame.id & 0x0F))
{
}
int RNetJsmSelectModeMap::modeMap() const { return modeMap_; }
void RNetJsmSelectModeMap::setModeMap(int value) { modeMap_ = value; }
QString RNetJsmSelectModeMap::detailsString() const { return QStringLiteral("modeMap=%1").arg(modeMap_); }

RNetParameterRequest::RNetParameterRequest(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetParameterRequest", idMask)
    , module_(int(frame.id & 0x0F))
    , opcodeByte_(byteAt(frame.data, 0))
    , parameterByte_(byteAt(frame.data, 1))
    , commandByte_(byteAt(frame.data, 2))
    , pointer_(byteAt(frame.data, 4))
    , subIndex_(byteAt(frame.data, 6))
    , value16_(le16At(frame.data, 4))
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
    , value16_(le16At(frame.data, 4))
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
QString RNetJsmUniqueId::detailsString() const { return QStringLiteral("uniqueId=%1").arg(hexBytes(uniqueId_)); }

RNetModeSelectProfile::RNetModeSelectProfile(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeSelectProfile", idMask)
    , profile_(int(byteAt(frame.data, 1) & 0x0F))
{
}
int RNetModeSelectProfile::profile() const { return profile_; }
void RNetModeSelectProfile::setProfile(int value) { profile_ = value; }
QString RNetModeSelectProfile::detailsString() const { return QStringLiteral("profile=%1").arg(profile_); }

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
    return QStringLiteral("status=%1,mode=%2,value=%3").arg(hexByte(status_)).arg(mode_).arg(hexByte(valueByte_));
}

RNetModeSelectOrSuspend::RNetModeSelectOrSuspend(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetModeSelectOrSuspend", idMask)
    , suspend_(byteAt(frame.data, 0) == 0x40u)
    , mode_(int(byteAt(frame.data, 1) & 0x0F))
{
}
bool RNetModeSelectOrSuspend::suspend() const { return suspend_; }
void RNetModeSelectOrSuspend::setSuspend(bool value) { suspend_ = value; }
int RNetModeSelectOrSuspend::mode() const { return mode_; }
void RNetModeSelectOrSuspend::setMode(int value) { mode_ = value; }
QString RNetModeSelectOrSuspend::detailsString() const
{
    return QStringLiteral("action=%1,mode=%2").arg(suspend_ ? QStringLiteral("suspend") : QStringLiteral("select")).arg(mode_);
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
    return QStringLiteral("status=%1,mode=%2,value=%3").arg(hexByte(status_)).arg(mode_).arg(hexByte(valueByte_));
}

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
    return QStringLiteral("device=%1,x=%2,y=%3").arg(device_).arg(int(x_)).arg(int(y_));
}

RNetDeviceHeartbeat::RNetDeviceHeartbeat(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetDeviceHeartbeat", idMask)
    , payload_(frame.data)
{
}
QByteArray RNetDeviceHeartbeat::payload() const { return payload_; }
void RNetDeviceHeartbeat::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetDeviceHeartbeat::detailsString() const { return QStringLiteral("payload=%1").arg(hexBytes(payload_)); }

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
QString RNetMotorMaxSpeed::detailsString() const { return QStringLiteral("device=%1,percent=%2").arg(device_).arg(int(percent_)); }

RNetUiInteraction::RNetUiInteraction(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetUiInteraction", idMask)
    , module_(int((frame.id >> 8) & 0xFF))
{
}
int RNetUiInteraction::module() const { return module_; }
void RNetUiInteraction::setModule(int value) { module_ = value; }
QString RNetUiInteraction::detailsString() const { return QStringLiteral("module=%1").arg(module_); }

RNetLampCommand::RNetLampCommand(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetLampCommand", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , command_(stateForLampCommand(frame.id))
{
}
int RNetLampCommand::device() const { return device_; }
void RNetLampCommand::setDevice(int value) { device_ = value; }
QString RNetLampCommand::command() const { return command_; }
void RNetLampCommand::setCommand(const QString &value) { command_ = value; }
QString RNetLampCommand::detailsString() const { return QStringLiteral("device=%1,command=%2").arg(device_).arg(command_); }

RNetLampStatus::RNetLampStatus(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetLampStatus", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
    , mask_(byteAt(frame.data, 0))
    , bitmap_(byteAt(frame.data, 1))
    , left_((bitmap_ & 0x01u) != 0)
    , right_((bitmap_ & 0x04u) != 0)
    , flood_((bitmap_ & 0x80u) != 0)
    , hazard_((bitmap_ & 0x10u) != 0)
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
    return QStringLiteral("device=%1,mask=%2,bitmap=%3,left=%4,right=%5,hazard=%6,flood=%7")
        .arg(device_)
        .arg(hexByte(mask_))
        .arg(hexByte(bitmap_))
        .arg(boolText(left_))
        .arg(boolText(right_))
        .arg(boolText(hazard_))
        .arg(boolText(flood_));
}

RNetHorn::RNetHorn(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetHorn", idMask)
    , origin_(int((frame.id >> 8) & 0x0F))
    , active_((frame.id & 0x01u) == 0)
{
}
int RNetHorn::origin() const { return origin_; }
void RNetHorn::setOrigin(int value) { origin_ = value; }
bool RNetHorn::active() const { return active_; }
void RNetHorn::setActive(bool value) { active_ = value; }
QString RNetHorn::detailsString() const { return QStringLiteral("origin=%1,active=%2").arg(origin_).arg(boolText(active_)); }

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
QString RNetPmHeartbeat::detailsString() const { return QStringLiteral("device=%1,value=%2").arg(device_).arg(hexByte(valueByte_)); }

RNetPmConnected::RNetPmConnected(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPmConnected", idMask)
    , valueByte_(byteAt(frame.data, 0))
{
}
quint8 RNetPmConnected::valueByte() const { return valueByte_; }
void RNetPmConnected::setValueByte(quint8 value) { valueByte_ = value; }
QString RNetPmConnected::detailsString() const { return QStringLiteral("value=%1").arg(hexByte(valueByte_)); }

RNetPmMotorState::RNetPmMotorState(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetPmMotorState", idMask)
    , payload_(frame.data)
{
    if (frame.id == 0x0C000005u || (frame.data.size() >= 4 && byteAt(frame.data, 0) == 0x90u && byteAt(frame.data, 3) == 0x00u))
        state_ = QStringLiteral("stopped");
    else if (frame.id == 0x0C000006u)
        state_ = QStringLiteral("decelerating");
    else if (frame.data.size() >= 4 && byteAt(frame.data, 0) == 0x90u && byteAt(frame.data, 3) == 0x40u)
        state_ = QStringLiteral("running");
    else
        state_ = QStringLiteral("unknown");
}
QString RNetPmMotorState::state() const { return state_; }
void RNetPmMotorState::setState(const QString &value) { state_ = value; }
QByteArray RNetPmMotorState::payload() const { return payload_; }
void RNetPmMotorState::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetPmMotorState::detailsString() const { return QStringLiteral("state=%1,payload=%2").arg(state_).arg(hexBytes(payload_)); }

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
    return QStringLiteral("tones=[%1/%2,%3/%4,%5/%6,%7/%8]")
        .arg(int(duration1_)).arg(int(note1_))
        .arg(int(duration2_)).arg(int(note2_))
        .arg(int(duration3_)).arg(int(note3_))
        .arg(int(duration4_)).arg(int(note4_));
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
QString RNetDriveMotorCurrent::detailsString() const { return QStringLiteral("device=%1,currentRaw=%2").arg(device_).arg(currentRaw_); }

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
QString RNetBatteryLevel::detailsString() const { return QStringLiteral("device=%1,percent=%2").arg(device_).arg(int(percent_)); }

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
QString RNetTimeOfDay::detailsString() const { return QStringLiteral("device=%1,raw=%2").arg(device_).arg(hexBytes(rawTime_)); }

RNetReady::RNetReady(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetReady", idMask)
    , device_(int((frame.id >> 8) & 0x0F))
{
}
int RNetReady::device() const { return device_; }
void RNetReady::setDevice(int value) { device_ = value; }
QString RNetReady::detailsString() const { return QStringLiteral("device=%1").arg(device_); }

RNetEnableMotorOutputFamily::RNetEnableMotorOutputFamily(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetEnableMotorOutputFamily", idMask)
    , familyCode_(quint8(frame.id & 0xFFu))
    , payload_(frame.data)
{
}
quint8 RNetEnableMotorOutputFamily::familyCode() const { return familyCode_; }
void RNetEnableMotorOutputFamily::setFamilyCode(quint8 value) { familyCode_ = value; }
QByteArray RNetEnableMotorOutputFamily::payload() const { return payload_; }
void RNetEnableMotorOutputFamily::setPayload(const QByteArray &value) { payload_ = value; }
QString RNetEnableMotorOutputFamily::detailsString() const
{
    return QStringLiteral("family=%1,payload=%2").arg(hexByte(familyCode_)).arg(hexBytes(payload_));
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
QString RNetBlockTransferData::detailsString() const { return QStringLiteral("seq=%1,payload=%2").arg(sequence_).arg(hexBytes(payload_)); }

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
QString RNetBlockTransferTail::detailsString() const { return QStringLiteral("seq=%1,payload=%2").arg(sequence_).arg(hexBytes(payload_)); }

RNetBlockTransferAck::RNetBlockTransferAck(const CanFrame &frame, quint32 idMask)
    : RNetFrame(frame, "RNetBlockTransferAck", idMask)
    , sequence_(quint16(frame.id & 0xFFFFu))
{
}
quint16 RNetBlockTransferAck::sequence() const { return sequence_; }
void RNetBlockTransferAck::setSequence(quint16 value) { sequence_ = value; }
QString RNetBlockTransferAck::detailsString() const { return QStringLiteral("seq=%1").arg(sequence_); }
