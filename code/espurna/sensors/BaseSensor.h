// -----------------------------------------------------------------------------
// Abstract sensor class (other sensor classes extend this class)
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

typedef enum magnitude_t {

    MAGNITUDE_NONE = 0,

    MAGNITUDE_TEMPERATURE,
    MAGNITUDE_HUMIDITY,
    MAGNITUDE_PRESSURE,

    MAGNITUDE_CURRENT,
    MAGNITUDE_VOLTAGE,
    MAGNITUDE_POWER_ACTIVE,
    MAGNITUDE_POWER_APPARENT,
    MAGNITUDE_POWER_REACTIVE,
    MAGNITUDE_ENERGY,
    MAGNITUDE_ENERGY_DELTA,
    MAGNITUDE_POWER_FACTOR,

    MAGNITUDE_ANALOG,
    MAGNITUDE_DIGITAL,
    MAGNITUDE_EVENTS,

    MAGNITUDE_PM1dot0,
    MAGNITUDE_PM2dot5,
    MAGNITUDE_PM10,

    MAGNITUDE_CO2,

    MAGNITUDE_MAX,

} magnitude_t;

#define GPIO_NONE                   0x99

#define SENSOR_ERROR_OK             0       // No error
#define SENSOR_ERROR_OUT_OF_RANGE   1       // Result out of sensor range
#define SENSOR_ERROR_WARM_UP        2       // Sensor is warming-up
#define SENSOR_ERROR_TIMEOUT        3       // Response from sensor timed out
#define SENSOR_ERROR_UNKNOWN_ID     4       // Sensor did not report a known ID
#define SENSOR_ERROR_CRC            5       // Sensor data corrupted

class BaseSensor {

    public:

        // Constructor
        BaseSensor() {}

        // Destructor
        ~BaseSensor() {}

        // Initialization method, must be idempotent
        virtual void begin() {}

        // Loop-like method, call it in your main loop
        virtual void tick() {}

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre() {}

        // Post-read hook (usually to reset things)
        virtual void post() {}

        // Descriptive name of the sensor
        virtual String name() {}

        // Descriptive name of the slot # index
        virtual String slot(unsigned char index) {}

        // Type for slot # index
        virtual magnitude_t type(unsigned char index) {}

        // Current value for slot # index
        virtual double value(unsigned char index) {}

        // Return sensor status (true for ready)
        bool status() { return _error == 0; }

        // Return sensor last internal error
        int error() { return _error; }

        // Number of available slots
        unsigned char count() { return _count; }

        // Handle interrupt calls
        virtual void handleInterrupt(unsigned char gpio) {}

        // Interrupt attach callback
        void attached(unsigned char gpio) {
            #if SENSOR_DEBUG
                DEBUG_MSG("[SENSOR] GPIO%d interrupt attached to %s\n", gpio, name().c_str());
            #endif
        }

        // Interrupt detach callback
        void detached(unsigned char gpio) {
            #if SENSOR_DEBUG
                DEBUG_MSG("[SENSOR] GPIO%d interrupt detached from %s\n", gpio, name().c_str());
            #endif
        }

    protected:

        // Attach interrupt
        void attach(BaseSensor * instance, unsigned char gpio, unsigned char mode);

        // Detach interrupt
        void detach(unsigned char gpio);

        int _error = 0;
        bool _dirty = true;
        unsigned char _count = 0;


};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

BaseSensor * _isr_sensor_instance[16] = {NULL};

void _sensor_isr(unsigned char gpio) {
    if (_isr_sensor_instance[gpio]) {
        _isr_sensor_instance[gpio]->handleInterrupt(gpio);
    }
}

void _sensor_isr_0() { _sensor_isr(0); }
void _sensor_isr_2() { _sensor_isr(2); }
void _sensor_isr_4() { _sensor_isr(4); }
void _sensor_isr_5() { _sensor_isr(5); }
void _sensor_isr_12() { _sensor_isr(12); }
void _sensor_isr_13() { _sensor_isr(13); }
void _sensor_isr_14() { _sensor_isr(14); }
void _sensor_isr_15() { _sensor_isr(15); }

void (*_sensor_isrs[16])() = {
    _sensor_isr_0, NULL, _sensor_isr_2, NULL, _sensor_isr_4, _sensor_isr_5,
    NULL, NULL, NULL, NULL, NULL, NULL,
    _sensor_isr_12, _sensor_isr_13, _sensor_isr_14, _sensor_isr_15
};

void BaseSensor::attach(BaseSensor * instance, unsigned char gpio, unsigned char mode) {
    detach(gpio);
    if (_sensor_isrs[gpio]) {
        _isr_sensor_instance[gpio] = instance;
        attachInterrupt(gpio, _sensor_isrs[gpio], mode);
        instance->attached(gpio);
    }
}

void BaseSensor::detach(unsigned char gpio) {
    if (_isr_sensor_instance[gpio]) {
        detachInterrupt(gpio);
        _isr_sensor_instance[gpio]->detached(gpio);
        _isr_sensor_instance[gpio] = NULL;
    }
}
