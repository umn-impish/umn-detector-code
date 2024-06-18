#pragma once

#include <packets/BasePacket.hh>
#include <CharCast.hh>

namespace X123Driver { namespace Packets {
namespace Requests
{
/*
 * These packets are all of length zero, so only the packet IDs are needed.
 * WS Feb 2023
 * */

namespace { 
	template<char pid1, char pid2>
	class ZeroLengthPacket : public BasePacket
	{
		public:
			ZeroLengthPacket() : BasePacket({pid1, pid2}, 0ULL)
		{ }
	};
}

using CancelSequentialBuffering  = ZeroLengthPacket<0xf0_ch, 0x1f_ch>;
using ClearGeneralPurposeCounter = ZeroLengthPacket<0xf0_ch, 0x10_ch>;
using ClearSpectrum              = ZeroLengthPacket<0xf0_ch, 0x01_ch>;
using CommTestAck                = ZeroLengthPacket<0xf1_ch, 0x00_ch>;
using DiagnosticData             = ZeroLengthPacket<0x03_ch, 0x05_ch>;
using MCADisable                 = ZeroLengthPacket<0xf0_ch, 0x03_ch>;
using MCAEnable                  = ZeroLengthPacket<0xf0_ch, 0x02_ch>;
using RestartSequentialBuffering = ZeroLengthPacket<0xf0_ch, 0x1e_ch>;
using SpectrumPlusStatus         = ZeroLengthPacket<0x02_ch, 0x03_ch>;
using SpectrumPlusStatusClear    = ZeroLengthPacket<0x02_ch, 0x04_ch>;
using Status                     = ZeroLengthPacket<0x01_ch, 0x01_ch>;

}

}}
