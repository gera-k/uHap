/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_APPLE_SERVICES_H_
#define _HAP_APPLE_SERVICES_H_

#include "Hap.h"

// Apple-defined services and characteristics

#define HAP_NAME_UUID(_u_, _n_) \
    static inline const Uuid _uuid = Uuid(0x ## _u_, 0x0000, 0x1000, 0x8000, 0x0026BB765291); \
    static inline const char* _name = #_n_;

// Simple characteristic
// u    uuid
// n    name
// f    Format::
// p    Perm::
// v    Default value
#define HAP_CHAR_DEFINE(_u_, _n_, _f_, _p_, _v_) \
    struct _n_ : public Char<_f_> \
    { \
        HAP_NAME_UUID(_u_, _n_) \
        void init(Service* svc, iid_t& iid, T v = _v_) \
        { \
           Char::init(iid, _name, _uuid, _p_); \
           value = v; \
           svc->add(this); \
        } \
    } 

// String characteristic
// u    uuid
// n    name
// l    max length
// p    Perm::
#define HAP_CHAR_STR_DEFINE(_u_, _n_, _l_, _p_) \
    struct _n_ : public CharString<_l_> \
    { \
        HAP_NAME_UUID(_u_, _n_) \
        void init(Service* svc, iid_t& iid, const char* v = nullptr) \
        { \
           CharString::init(iid, _name, _uuid, _p_, v); \
           svc->add(this); \
        } \
    } 

namespace Hap
{
    HAP_CHAR_DEFINE(14, Identify, Format::Bool, Perm::pw, false);
    HAP_CHAR_STR_DEFINE(20, Manufacturer, 64, Perm::pr);
    HAP_CHAR_STR_DEFINE(21, Model, 64, Perm::pr);
    HAP_CHAR_STR_DEFINE(23, Name, 64, Perm::pr);
    HAP_CHAR_STR_DEFINE(30, SerialNumber, 64, Perm::pr);
    HAP_CHAR_STR_DEFINE(52, FirmwareRevision, 16, Perm::pr);
    HAP_CHAR_STR_DEFINE(53, HardwareRevision, 16, Perm::pr);
    HAP_CHAR_DEFINE(A6, AccessoryFlags, Format::Uint32, Perm::pr | Perm::ev, 0);

    struct AccessoryInfo : public Service
    {
        HAP_NAME_UUID(3E, AccessoryInfo)

        FirmwareRevision firmwareRevision;
        Identify identify;
        Manufacturer manufacturer;
        Model model;
        Name name;
        SerialNumber serialNumber;
        HardwareRevision hardwareRevision;
        AccessoryFlags accessoryFlags;

        void init (iid_t& iid, bool additionalSetup = false)
        {
            Service::init(iid, _name, _uuid);

            firmwareRevision.init(this, iid, Config::firmwareRevision);
            identify.init(this, iid);
            manufacturer.init(this, iid, Config::manufacturer);
            model.init(this, iid, Config::model);
            name.init(this, iid, Config::name);
            serialNumber.init(this, iid, Config::serialNumber);
            hardwareRevision.init(this, iid, Config::hardwareRevision);
            if (additionalSetup)
                accessoryFlags.init(this, iid, BIT_(0));
        }
    };

    HAP_CHAR_DEFINE(25, On, Format::Bool, Perm::pr | Perm::pw | Perm::ev | Perm::de | Perm::bn, false);

    struct Brightness : public Char<Format::Int>
    { 
        HAP_NAME_UUID(08, Brightness)

        PropMinValue<Format::Int> min;
        PropMaxValue<Format::Int> max;
        PropStepValue<Format::Int> step;
        PropUnit unit;

        void init(Service* svc, iid_t& iid, T v = 0)
        {
           Char::init(iid, _name, _uuid, Perm::pr | Perm::pw | Perm::ev | Perm::de | Perm::bn);
           value = v;
           min = 0;
           max = 100;
           step = 1;
           unit = Unit::Percentage;

           add(&min);
           add(&max);
           add(&step);
           add(&unit);

           svc->add(this);
        }
    };

    struct Hue : public Char<Format::Float>
    { 
        HAP_NAME_UUID(13, Hue)

        PropMinValue<Format::Float> min;
        PropMaxValue<Format::Float> max;
        PropStepValue<Format::Float> step;
        PropUnit unit;

        void init(Service* svc, iid_t& iid, T v = 0)
        {
           Char::init(iid, _name, _uuid, Perm::pr | Perm::pw | Perm::ev | Perm::de | Perm::bn);
           value = v;
           min = 0.;
           max = 360.;
           step = 1.;
           unit = Unit::Arcdegrees;

           add(&min);
           add(&max);
           add(&step);
           add(&unit);

           svc->add(this);
        }
    };

    struct Saturation : public Char<Format::Float>
    { 
        HAP_NAME_UUID(2F, Saturation)

        PropMinValue<Format::Float> min;
        PropMaxValue<Format::Float> max;
        PropStepValue<Format::Float> step;
        PropUnit unit;

        void init(Service* svc, iid_t& iid, T v = 0)
        {
           Char::init(iid, _name, _uuid, Perm::pr | Perm::pw | Perm::ev | Perm::de | Perm::bn);
           value = v;
           min = 0.;
           max = 100.;
           step = 1.;
           unit = Unit::Percentage;

           add(&min);
           add(&max);
           add(&step);
           add(&unit);

           svc->add(this);
        }
    };

    struct LightBulb : public Service
    {
        HAP_NAME_UUID(43, LightBulb)

        On on;
        Brightness brightness;
        Hue hue;
        Saturation saturation;

        void init (iid_t& iid, PropSvcProp::T prop = 0)
        {
            Service::init(iid, _name, _uuid, prop);

            on.init(this, iid);
            // do not add optional characteristics,
            //  implementation wil add them as necesasry
        }
    };

}

#endif /*_HAP_APPLE_SERVICES_H_*/

