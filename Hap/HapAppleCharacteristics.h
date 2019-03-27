/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _HAP_APPLE_CHARACTERISTICS_H_
#define _HAP_APPLE_CHARACTERISTICS_H_

namespace Hap
{
	namespace Characteristic
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
	}
}

#endif
