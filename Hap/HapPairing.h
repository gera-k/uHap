/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_PAIRING_H_
#define _HAP_PAIRING_H_

#include "Platform.h"

#include "Hap.h"
#include "HapTlv.h"

// Hap pairing

namespace Hap::Pairing
{
    const char* TlvTypeToStr();

	// Pairing Path	(BLE characeristic or HTTP path)
	enum class Path : uint8_t
	{
		Unknown = 0,
		Setup = 1,
		Verify = 2,
		Pairings = 3
	};

    // Pairing Method   (R2: 5.15)
   	enum class Method : uint8_t
	{
		PairSetup = 0,
		PairSetupWithAuth = 1,
		PairVerify = 2,
		AddPairing = 3,
		RemovePairing = 4,
		ListPairing = 5,
		Resume = 6,
		Unknown = 0xFF
	};
    const char* MethodToStr();

	enum class State : uint8_t
	{
		Unknown = 0,
		M1 = 1,
		M2 = 2,
		M3 = 3,
		M4 = 4,
		M5 = 5,
		M6 = 6,
	};

	enum class Error : uint8_t
	{
		Unknown = 0x01,
		Authentication = 0x02,
		Backoff = 0x03,
		MaxPeers = 0x04,
		MaxTries = 0x05,
		Unavailable = 0x06,
		Busy = 0x07,
	};

    namespace PairingType
    {
        using T = uint32_t;
        enum _flags : T
        {
            Transient = BIT_(4),
            Split = BIT_(24)
        };
    }

	// pairing TLVs
	struct TlvMethod : public Tlv
    {
        uint8_t method;

        TlvMethod(Method method_)
        : Tlv(TlvType::Method, Buf(&method, sizeof(method)))
        {
			method = (uint8_t)method_;
		}
    };

	struct TlvState : public Tlv
    {
        uint8_t state;

        TlvState(State state_)
        : Tlv(TlvType::State, Buf(&state, sizeof(state)))
        {
			state = (uint8_t)state_;
		}
    };

	struct TlvError : public Tlv
    {
        uint8_t error;

        TlvError(Error error_)
        : Tlv(TlvType::Error, Buf(&error, sizeof(error)))
        {
			error = (uint8_t)error_;
		}
    };

	struct TlvSeparator : public Tlv
	{
        TlvSeparator()
        : Tlv(TlvType::Separator, Buf())
        {
		}
	};

	void init();

	Hap::Status dispatch(Hap::Operation* op, Path path, Hap::Buf& req, Hap::Buf& rsp);
}

#endif /*_HAP_PAIRING_H_*/