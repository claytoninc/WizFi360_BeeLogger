/*---------------------------------------------------------------------------

    Humidity & Temperature Sensors
        Routines to read from the HTU21D sensor

    clayton@isnotcrazy.com

	Uses the HTU21D library files from Sparkfun, ported from Arduino

	Provides a C wrapper around the C++ library

---------------------------------------------------------------------------*/
#include "HTU21D.h"
#include "humidity_temp_sensors.h"

#define HTU21D_IC2_PORT     1       // I2C1
#define HTU21D_SDA_PIN      2       // GP2 = pin 4
#define HTU21D_SCL_PIN      3       // GP3 = pin 5

// Data
static HTU21D myHumidity;

// Initialise the sensor
void HumidityTempSensor_init( void )
{
    myHumidity.begin( HTU21D_IC2_PORT, HTU21D_SDA_PIN, HTU21D_SCL_PIN );
}

// Read a sensor
bool HumidityTempSensor_read( int sensor_id, double *result )
{
    bool retb;
    if ( sensor_id==HUMIDITY_SENSOR )
        retb = myHumidity.readHumidity( result );
    else
        retb = myHumidity.readTemperature( result );
  return retb;
}
