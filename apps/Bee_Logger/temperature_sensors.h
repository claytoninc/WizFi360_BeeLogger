/*---------------------------------------------------------------------------

    Temperature Sensors
        Routines to read from the one-wire sensor

    clayton@isnotcrazy.com

---------------------------------------------------------------------------*/

#ifndef TEMPERATURE_SENSORS_H
#define TEMPERATURE_SENSORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sensor IDs
#define SENSORS_T1      1
#define SENSORS_T2      2
#define SENSORS_T3      3

// Functions

// Initialise all sensor channels
void TempSensor_init( void );

// Read a sensor
bool TempSensor_read( int sensor_id, double *result );

#ifdef __cplusplus
}
#endif

#endif      // TEMPERATURE_SENSORS_H
