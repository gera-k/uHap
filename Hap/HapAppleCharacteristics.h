/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_APPLE_CHARACTERISTICS_H_
#define _HAP_APPLE_CHARACTERISTICS_H_

namespace Hap::Characteristic
{
	// Firmware Revision
	//	UUID 00000052-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	class FirmwareRevision : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "52";

		FirmwareRevision(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// Hardware Revision
	//	UUID 00000053-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	class HardwareRevision : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "53";

		HardwareRevision(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// Identify
	//	UUID 00000014-0000-1000-8000-0026BB765291
	//	Permissions Paired Write
	//	Format		bool
	class Identify : public Simple< 0, FormatId::Bool >
	{
	public:
		static constexpr const char* Type = "14";

		Identify(Simple::V value = false) : Simple(
			Type,
			Property::Permissions::PairedWrite
		)
		{
		}

	};

	// Manufacturer
	//	UUID 00000020-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	//	Maximum Length	64
	class Manufacturer : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "20";

		Manufacturer(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// Model
	//	UUID 00000021-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	//	Maximum Length	64
	class Model : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "21";

		Model(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// Name
	//	UUID 00000023-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	//	Maximum Length	64
	class Name : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "23";

		Name(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// SerialNumber
	//	UUID 00000030-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	//	Maximum Length	64
	class SerialNumber : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "30";

		SerialNumber(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};

	// On
	//	UUID 00000025-0000-1000-8000-0026BB765291
	//	Permissions Paired Read, Paired Write, Notify
	//	Format		bool
	class On : public Simple< 0, FormatId::Bool >
	{
	public:
		static constexpr const char* Type = "25";

		On(Simple::V value = false) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::PairedWrite |
			Property::Permissions::Events
		)
		{
			Value(value);
		}
	};

	// Brightness
	//	UUID 00000008-0000-1000-8000-0026BB765291
	//	Permissions 	Paired Read, Paired Write, Notify
	//	Format 			int
	//	Minimum Value	0
	//	Maximum Value	100
	//	Step Value		1
	//	Unit 			percentage
	class Brightness : public Simple< 4, FormatId::Int>
	{
	protected:
		Property::MinValue<FormatId::Int> _min;
		Property::MaxValue<FormatId::Int> _max;
		Property::MinStep<FormatId::Int> _step;
		Property::Unit _unit;
	public:
		static constexpr const char* Type = "8";

		Brightness(Simple::V value = 0) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::PairedWrite |
			Property::Permissions::Events
		)
		{
			AddProperty(&_min);
			AddProperty(&_max);
			AddProperty(&_step);
			AddProperty(&_unit);

			_min.set(0);
			_max.set(100);
			_step.set(1);
			_unit.set(Property::Unit::T::percentage);

			Value(value);
		}
	};

	// Hue
	//	UUID 00000013-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read, Paired Write, Notify
	//	Format			float
	//	Minimum Value	0
	//	Maximum Value	360
	//	Step Value		1
	//	Unit			arcdegrees
	class Hue : public Simple< 4, FormatId::Float>
	{
	protected:
		Property::MinValue<FormatId::Float> _min;
		Property::MaxValue<FormatId::Float> _max;
		Property::MinStep<FormatId::Float> _step;
		Property::Unit _unit;
	public:
		static constexpr const char* Type = "13";

		Hue(Simple::V value = 0.) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::PairedWrite |
			Property::Permissions::Events
		)
		{
			AddProperty(&_min);
			AddProperty(&_max);
			AddProperty(&_step);
			AddProperty(&_unit);

			_min.set(0.);
			_max.set(360.);
			_step.set(1.);
			_unit.set(Property::Unit::T::arcdegrees);

			Value(value);
		}
	};

	// Saturation
	//	UUID 0000002F-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read, Paired Write, Notify
	//	Format			float
	//	Minimum Value	0
	//	Maximum Value	100
	//	Step Value		1
	//	Unit			percentage
	class Saturation : public Simple< 4, FormatId::Float>
	{
	protected:
		Property::MinValue<FormatId::Int> _min;
		Property::MaxValue<FormatId::Int> _max;
		Property::MinStep<FormatId::Int> _step;
		Property::Unit _unit;
	public:
		static constexpr const char* Type = "2F";

		Saturation(Simple::V value = 0) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::PairedWrite |
			Property::Permissions::Events
		)
		{
			AddProperty(&_min);
			AddProperty(&_max);
			AddProperty(&_step);
			AddProperty(&_unit);

			_min.set(0);
			_max.set(100);
			_step.set(1);
			_unit.set(Property::Unit::T::percentage);

			Value(value);
		}
	};

	// Color Temperature
	//	UUID 000000CE-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read, Paired Write, Notify
	//	Format			uint32
	//	Minimum Value	50
	//	Maximum Value	400
	//	Step Value		1
	class ColorTemp : public Simple< 3, FormatId::Uint32>
	{
	protected:
		Property::MinValue<FormatId::Uint32> _min;
		Property::MaxValue<FormatId::Uint32> _max;
		Property::MinStep<FormatId::Uint32> _step;
	public:
		static constexpr const char* Type = "CE";

		ColorTemp(Simple::V value = 0) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::PairedWrite |
			Property::Permissions::Events
		)
		{
			AddProperty(&_min);
			AddProperty(&_max);
			AddProperty(&_step);

			_min.set(50);
			_max.set(400);
			_step.set(1);

			Value(value);
		}
	};

	// Current Door State
	//	This characteristic describes the current state of a door.
	//	UUID 0000000E-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read, Notify
	//	Format			uint8
	//	Minimum Value	0
	//	Maximum Value	4
	//	Step Value		1
	//	Valid Values
	//					0 "Open. The door is fully open."
	//					1 "Closed. The door is fully closed."
	//					2 "Opening. The door is actively opening."
	//					3 "Closing. The door is actively closing."
	//					4 "Stopped. The door is not moving, and it is not fully open nor fully closed."
	//					5-255 "Reserved"
	class CurrentDoorState : public Simple< 3, FormatId::Uint8>
	{
	protected:
		Property::MinValue<FormatId::Uint8> _min;
		Property::MaxValue<FormatId::Uint8> _max;
		Property::MinStep<FormatId::Uint8> _step;
	public:
		static constexpr const char* Type = "E";

			enum State
			{
				Open = 0,
				Closed = 1,
				Opening = 2,
				Closing = 3,
				Stopped = 4,
				StateMax = 5
			};

			CurrentDoorState(Simple::V value = 0) : Simple(
				Type,
				Property::Permissions::PairedRead |
				Property::Permissions::Events
			)
			{
				AddProperty(&_min);
				AddProperty(&_max);
				AddProperty(&_step);

			_min.set(0);
			_max.set(4);
			_step.set(1);

			Value(value);
		}
	};

	// Target Door State
	//	This characteristic describes the target state of a door.
	//	UUID 00000032-0000-1000-8000-0026BB765291
	//	Permissions			Paired Read, Paired Write, Notify
	//	Format				uint8
	//	Minimum Value		0
	//	Maximum Value		1
	//	Step Value			1
	//	Valid Values
	//						0 "Open"
	//						1 "Closed"
	//						2 - 255 "Reserved"
	class TargetDoorState : public Simple< 3, FormatId::Uint8>
	{
	protected:
		Property::MinValue<FormatId::Uint8> _min;
		Property::MaxValue<FormatId::Uint8> _max;
		Property::MinStep<FormatId::Uint8> _step;
	public:
		static constexpr const char* Type = "32";

			enum State
			{
				Open = 0,
				Closed = 1,
				StateMax = 2
			};

			TargetDoorState(Simple::V value = 0) : Simple(
				Type,
				Property::Permissions::PairedRead |
				Property::Permissions::PairedWrite |
				Property::Permissions::Events
			)
			{
				AddProperty(&_min);
				AddProperty(&_max);
				AddProperty(&_step);

			_min.set(0);
			_max.set(1);
			_step.set(1);

			Value(value);
		}
	};

	// Obstruction Detected
	//	This characteristic describes the current state of an obstruction sensor,
	//	such as one that is used in a garage door.
	//	If the state is true then there is an obstruction detected.
	//	UUID 00000024-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read, Notify
	//	Format			bool
	class ObstructionDetected : public Simple< 0, FormatId::Bool >
	{
	public:
		static constexpr const char* Type = "24";

		ObstructionDetected(Simple::V value = false) : Simple(
			Type,
			Property::Permissions::PairedRead |
			Property::Permissions::Events
		)
		{
			Value(value);
		}
	};

	// Version
	//	This characteristic contains a version string.
	//	UUID 00000037-0000-1000-8000-0026BB765291
	//	Permissions		Paired Read
	//	Format			string
	//	Maximum Length	64
	class Version : public Simple< 0, FormatId::ConstStr>
	{
	public:
		static constexpr const char* Type = "37";

		Version(Simple::V value = nullptr) : Simple(
			Type,
			Property::Permissions::PairedRead
		)
		{
			Value(value);
		}
	};
}

#endif
