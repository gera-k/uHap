/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

class Joystick
{
public:
	enum Channel
	{
		Vert = 0,
		Horiz = 1
	};

	enum Position
	{
		Middle = 0,
		Low1,
		Low2,
		High1,
		High2,
	};

	enum Button
	{
		Low = 0,
		High
	};

	virtual Position Get(Channel ch) = 0;
	virtual Button Get() = 0;

	virtual void Test() = 0;
};

#endif /*JOYSTICK_H*/