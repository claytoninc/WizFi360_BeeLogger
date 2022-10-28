/*---------------------------------------------------------------------------

    Humidity & Temperature Sensors
        Routines to read from the HTU21D sensor

    clayton@isnotcrazy.com

---------------------------------------------------------------------------*/

#ifndef HUMIDITY_TEMP_SENSORS_H
#define HUMIDITY_TEMP_SENSORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sensor IDs
#define HUMIDITY_SENSOR     1
#define TEMP_SENSOR         2

// Functions

// Initialise the sensor
void HumidityTempSensor_init( void );

// Read a sensor
bool HumidityTempSensor_read( int sensor_id, double *result );

#ifdef __cplusplus
}
#endif

#endif      // HUMIDITY_TEMP_SENSORS_H
