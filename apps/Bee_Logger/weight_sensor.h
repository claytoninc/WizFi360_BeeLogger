/*---------------------------------------------------------------------------

    Weight Sensor
        Routines to read from the HX711 load cell sensor

    clayton@isnotcrazy.com

---------------------------------------------------------------------------*/

#ifndef WEIGHT_SENSORS_H
#define WEIGHT_SENSORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Functions

// Initialise the sensor
void WeightSensor_init( void );

// Read the sensor, producing a result in kg
bool WeightSensor_read( int count, double *result );

#ifdef __cplusplus
}
#endif

#endif      // WEIGHT_SENSORS_H
