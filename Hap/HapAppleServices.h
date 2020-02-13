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

#ifndef _HAP_APPLE_SERVICES_H_
#define _HAP_APPLE_SERVICES_H_

namespace Hap
{
	// HAP Protocol Information
	//
	//	UUID 000000A2-0000-1000-8000-0026BB765291
	//	Required Characteristics
	//		0 Version
	//	Every accessory must expose a single instance of the HAP protocol information.For a bridge accessory,
	//	only the primary HAP accessory object must contain this service.The value is transport dependent.
	class ProtocolInformation : public Service<1>	// reserve space for 1 characteristic
	{
	protected:
		Characteristic::Version _version;

	public:
		static constexpr const char* Type = "A2";

		ProtocolInformation(
			Property::PrimaryService::T primary = false,
			Property::HiddenService::T hidden = false
		) : Service(Type)
		{
			AddCharacteristic(&_version, 0);
		}
	};

	// Accessory Information
	//	
	//	UUID 0000003E-0000-1000-8000-0026BB765291
	//	Required Characteristics 
	//		0 Identify
	//		1 Manufacturer
	//		2 Model
	//		3 Name
	//		4 Serial Number
	//		5 Firmware Revision
	//	Optional Characteristics
	//		6 Hardware Revision
	//		7 Accessory Flags
	// defines space for 8 characteristics and adds Required ones.
	//	custom extentions may add optonal ones 
	class AccessoryInformation : public Service<8>	// reserve space for 8 characteristics
	{
	protected:
		Characteristic::Identify _identify;
		Characteristic::Manufacturer _manufacturer;
		Characteristic::Model _model;
		Characteristic::Name _name;
		Characteristic::SerialNumber _serialNumber;
		Characteristic::FirmwareRevision _firmwareRevision;

		void Add(Characteristic::HardwareRevision& ch) { AddCharacteristic(&ch, 6); }

	public:
		static constexpr const char* Type = "3E";

		AccessoryInformation(
			Property::PrimaryService::T primary = false,
			Property::HiddenService::T hidden = false
		) : Service(Type)
		{
			AddCharacteristic(&_identify, 0);
			AddCharacteristic(&_manufacturer, 1);
			AddCharacteristic(&_model, 2);
			AddCharacteristic(&_name, 3);
			AddCharacteristic(&_serialNumber, 4);
			AddCharacteristic(&_firmwareRevision, 5);
		}
	};

	// Lightbulb
	//	This service describes a lightbulb.
	//	UUID 00000043-0000-1000-8000-0026BB765291
	//	Required Characteristics 
	//		0 On
	//	Optional Characteristics
	//		1 Brightness
	//		2 Name
	//		3 Hue or ColorTemp
	//		4 Saturation
	// defines space for 5 characteristics and adds Required ones.
	//	custom extentions may add optonal ones 
	// Note: if a Lighbulb defines ColorTemp it cannot define Hue/Saturation
	class Lightbulb : public Service<5>	// reserve space for 5 characteristics
	{
	protected:
		Characteristic::On _on;

		void AddBrightness(Characteristic::Brightness& ch) { AddCharacteristic(&ch, 1); }
		void AddName(Characteristic::Name& ch) { AddCharacteristic(&ch, 2); }
		void AddHue(Characteristic::Hue& ch) { AddCharacteristic(&ch, 3); }
		void AddSaturation(Characteristic::Saturation& ch) { AddCharacteristic(&ch, 4); }
		void AddColorTemp(Characteristic::ColorTemp& ch) { AddCharacteristic(&ch, 3); }

	public:
		static constexpr const char* Type = "43";

		Lightbulb(
			Characteristic::On::V v = false,
			Property::PrimaryService::T primary = false,
			Property::HiddenService::T hidden = false
		) : Service(Type),
			_on(v)
		{
			AddCharacteristic(&_on, 0);
		}
	};

	// Garage Door Opener
	//	This service describes a garage door opener that controls a single door.
	//	If a garage has more than one door,
	//	then each door should have its own Garage Door Opener Service.
	//	UUID 00000041-0000-1000-8000-0026BB765291
	//	Required Characteristics 
	//		0 Current Door State
	//		1 Target Door State
	//		2 Obstruction Detected
	//	Optional Characteristics
	//		3 Name
	//		4 Lock Current State
	//		5 Lock Target State
	class GarageDoorOpener : public Service<6>	// reserve space for 6 characteristics
	{
	protected:
		Characteristic::CurrentDoorState _current;
		Characteristic::TargetDoorState _target;
		Characteristic::ObstructionDetected _obstruction;

		void AddName(Characteristic::Name& ch) { AddCharacteristic(&ch, 3); }
		//void AddLockCurrent(Characteristic::LockCurrentState& ch) { AddCharacteristic(&ch, 4); }
		//void AddLockTarget(Characteristic::LockTargetState& ch) { AddCharacteristic(&ch, 5); }

	public:
		static constexpr const char* Type = "41";

		GarageDoorOpener(
			Property::PrimaryService::T primary = false,
			Property::HiddenService::T hidden = false
		) : Service(Type)
		{
			AddCharacteristic(&_current, 0);
			AddCharacteristic(&_target, 1);
			AddCharacteristic(&_obstruction, 2);
		}
	};

}

#endif
