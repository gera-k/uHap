/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _UHAP_UTILS_LBPWM_H_
#define _UHAP_UTILS_LBPWM_H_

#include "Platform.h"

// PWM-controlled light bulb
class LbPWM
{
protected:
	static const uint8_t _b2p[];	// 0..100% brightness to 0.255 pwm
	static const uint8_t _v2p[];	// 0..255 value to 0.255 pwm

	bool _on = false;
	uint8_t _v = 0;		// value (brightness) 0..100
	uint8_t _s = 0;		// saturation 0..100
	uint16_t _h = 0;	// hue 0..359
public:
	// convert brightness (0..100%) to pwm (0..255)
	//	pwm = 256^(br/100)-1
	uint8_t b2p(uint8_t br)
	{
		return _b2p[br % 101];
	}

	// convert rgb value (0..255) to pwm (0..255)
	//	pwm = 256^(v/256)-1
	uint8_t v2p(uint8_t v)
	{
		return _v2p[v];
	}

	void rgb(uint8_t& R, uint8_t& G, uint8_t& B);
};

#endif /*_UHAP_UTILS_LBPWM_H_*/