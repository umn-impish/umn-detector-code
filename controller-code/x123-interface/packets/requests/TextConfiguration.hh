#pragma once

#include <string>
#include <common.hh>
#include <packets/BasePacket.hh>

namespace X123Driver { namespace Packets {
namespace Requests {

template<uint8_t pid1, uint8_t pid2>
class TextConfigurationBase : public BasePacket {
    std::string settingsStr;
public:
    static const size_t MAX_SETTINGS_LEN = 512;
    TextConfigurationBase(const std::string& set) :
        BasePacket({pid1, pid2}, MAX_SETTINGS_LEN),
        settingsStr{set}
    { }
    TextConfigurationBase() :
        TextConfigurationBase("")
    { }
    ~TextConfigurationBase()
    { }

    void updateSettingsString(std::string const& s)
    {
        if (settingsStr.size() > MAX_SETTINGS_LEN) {
            std::string emsg{"X-123 settings string length > max allowed"};
            throw GenericException(emsg);
        }
        settingsStr = s;
    }

    void transferFromParsed() override {
        auto add = BasePacket::additionalTransferBytes();
        transfer.resize(settingsStr.size() + add);
        parsed = buff_t(settingsStr.begin(), settingsStr.end());

        BasePacket::transferFromParsed();
    }
};

using TextConfigurationToNvram = TextConfigurationBase<0x20, 0x02>;
using TextConfigurationToRam = TextConfigurationBase<0x20, 0x04>;

using TextConfigurationReadback = TextConfigurationBase<0x20, 0x03>;

} } }
