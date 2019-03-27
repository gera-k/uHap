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
	// Accessory Information
	//	
	//	UUID 0000003E-0000-1000-8000-0026BB765291
	//	Type public.hap.service.accessory-information
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
	//	Type public.hap.service.lightbulb
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
}


#endif
