/*---------------------------------------------------------------------------

    Temperature Sensors
        Routines to read from the one-wire DS18B20 sensor

    clayton@isnotcrazy.com

	Uses the one-wire library files (with minor tweaks) from:
		https://github.com/adamboardman/pico-onewire

	Provides a C wrapper around the C++ library

---------------------------------------------------------------------------*/
#include <stdio.h>
#include "one_wire.h"
#include "temperature_sensors.h"

// Data
One_wire TSensor1( 10 );		// GP10
One_wire TSensor2( 11 );		// GP11
One_wire TSensor3( 12 );		// GP12
One_wire TSensor4( 15 );		// GP15 (test sensor)

// Initialise all sensor channels
void TempSensor_init( void )
{
    rom_address_t 		address;

	TSensor1.init();
	TSensor2.init();
	TSensor3.init();
	TSensor4.init();
	// do an initial read
	TSensor1.single_device_read_rom( address );
	TSensor1.convert_temperature( address, true, false );
	TSensor1.temperature( address );
	TSensor2.single_device_read_rom( address );
	TSensor2.convert_temperature( address, true, false );
	TSensor2.temperature( address );
	TSensor3.single_device_read_rom( address );
	TSensor3.convert_temperature( address, true, false );
	TSensor3.temperature( address );
	TSensor4.single_device_read_rom( address );
	TSensor4.convert_temperature( address, true, false );
	TSensor4.temperature( address );
}

// Read a sensor
bool TempSensor_read( int sensor_id, double *result )
{
	One_wire 			*sensor;
	bool				retb;
    rom_address_t 		address;
	double 				temperature;

	// select sensor
	switch ( sensor_id )
	{
		case SENSORS_T1:	sensor = &TSensor1;		break;
		case SENSORS_T2:	sensor = &TSensor2;		break;
		case SENSORS_T3:	sensor = &TSensor3;		break;
		case 4:				sensor = &TSensor4;		break;
		default:			return false;
	}

	// find sensor
	retb = sensor->single_device_read_rom( address );
	if ( !retb )
	{
		printf("Sensor T%d not found\n", sensor_id );
		return false;
	}
	printf( "Sensor T%d Address: %02x%02x%02x%02x%02x%02x%02x%02x\n", sensor_id,
					address.rom[0], address.rom[1], address.rom[2], address.rom[3], 
					address.rom[4], address.rom[5], address.rom[6], address.rom[7] );

	// read sensor
	sensor->convert_temperature( address, true, false );
	temperature = sensor->temperature( address );
	printf( "Sensor T%d Temperature: %3.1f C\n", sensor_id, temperature );
	*result = temperature;
	if ( (temperature<-100.0) || (temperature>200.0) )
	{	// reject out of range temperatures
		return false;
	}
	return true;
}
