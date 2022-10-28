/*---------------------------------------------------------------------------

    Weight Sensor
        Routines to read from the HX711 load cell sensor

    clayton@isnotcrazy.com

---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "hardware/gpio.h"
#include "pico/time.h"
#include "weight_sensor.h"

// Macros

#define HX711_CLOCK         8
#define HX711_DATA          9

#define HX711_GAIN          3           // GAIN: 1=128  2=32  3=64

#define HX711_TIMEOUT_US    200000

#define WEIGHT_SCALEFACTOR  (1.0/2000)  // Scale factor to apply to the output reading
#define WEIGHT_OFFSET       0.0         // Kg offset to subtract (after scaling)



// Data







// Private Functions

static void HX711_reset( void )
{
    sleep_us(500);
    gpio_put( HX711_CLOCK, true );
    sleep_us(200);
    gpio_put( HX711_CLOCK, false );
    sleep_us(500);
}


static void HX711_setGainFactor( void )
{
    int     ii;

	// Set the channel and the gain factor for the next reading using the clock pin.
	for ( ii=0; ii<HX711_GAIN; ii++ )
    {
        gpio_put( HX711_CLOCK, true );
        sleep_us(10);
        gpio_put( HX711_CLOCK, false );
        sleep_us(10);
	}
}

static uint8_t HX711_shiftInData( void )
{
    uint8_t value = 0;
    uint8_t i;

    for( i=0; i<8; i++ ) 
    {
        gpio_put( HX711_CLOCK, true );
        sleep_us(10);
        if ( gpio_get(HX711_DATA) )
        {
            value |= 1 << (7-i);
        }
        gpio_put( HX711_CLOCK, false );
        sleep_us(10);
    }
    return value;
}

static bool HX711_waitForReady( int timeoutuSec )
{
    int             timecount;

    // wait for data pin to be low (or timeout)
    timecount = timeoutuSec;
    while ( timecount>0 )
    {
        // check for data ready
        if ( !gpio_get(HX711_DATA) )
        {   // data ready
          	//printf( "HX711 ready after %.2f mSec\n", 0.001*(timeoutuSec-timecount) );
            break;
        }
        // pause
        sleep_us(25);
        timecount -= 25;
    }

    if ( timecount<=0 )
    {
        return false;
    }
    return true;
}

// perform one reading, after waiting for it
static bool HX711_read( bool wait, int32_t *result )
{
    int             timecount;
	uint32_t        uvalue;
    bool            retb;

    // always wait, but ignore this or not depending on flag
    retb = HX711_waitForReady( HX711_TIMEOUT_US );
    if ( wait && !retb )
    {
        return false;
    }
    // Pulse the clock pin 24 times to read the data.
    uvalue = HX711_shiftInData();
    uvalue = (uvalue << 8) | HX711_shiftInData();
    uvalue = (uvalue << 8) | HX711_shiftInData();

	// Set the channel and the gain factor for the next reading using the clock pin.
    HX711_setGainFactor();

    // convert to 24-bit signed value
    // shift to top of word
    uvalue = (uvalue << 8);
    // change to signed and return to 24 bits
    *result = ((long)uvalue) / 256;

    // always return wait result
	return retb;
}


// Public Functions

// Initialise the sensor
void WeightSensor_init( void )
{
    int32_t reading;

	gpio_init(HX711_DATA);
	gpio_set_dir( HX711_DATA, GPIO_IN );
	gpio_init(HX711_CLOCK);
	gpio_set_dir( HX711_CLOCK, GPIO_OUT );
	gpio_put( HX711_CLOCK, false );
    // reset the device - >60uS of high
    HX711_reset();    
    // dummy reading
    HX711_read( false, &reading );
}

// Read the sensor, producing a result in kg
bool WeightSensor_read( int count, double *result )
{
    double  raw_reading_total;
    double  raw_reading;
    double  scaled_reading;
    int32_t reading;
    bool    retb;
    int     ii;

    // do initial read, but discard
    retb = HX711_read( false, &reading );
    // repeat if it wasn't ready
    if ( !retb )
    {
        printf( "Weight dummy reading retry\n" );
        retb = HX711_read( false, &reading );
    }
    if ( !retb )
    {
        printf( "Weight dummy reading retry\n" );
        retb = HX711_read( false, &reading );
    }
    if ( !retb )
    {
        printf( "Weight reading timed out at dummy reading\n" );
        return false;
    }
    printf( "Weight dummy reading %d\n", reading );

    // build up an average total
    raw_reading_total = 0.0;
    for ( ii=0; ii<count; ii++ )
    {
      	//printf( "Weight reading %d of %d ...\n", ii+1, count );
        retb = HX711_read( true, &reading );
        if ( !retb )
        {
        	printf( "Weight reading timed out at reading %d of %d\n", ii+1, count );
            HX711_reset();
            return false;
        }
        printf( "Weight reading %d was %d\n", ii+1, reading );
        raw_reading_total += 1.0*reading;
    }
    // calculations
    raw_reading = raw_reading_total / count;
    scaled_reading = ( raw_reading * WEIGHT_SCALEFACTOR ) - WEIGHT_OFFSET;
    printf( "Weight total:  %.2f of %d readings\n", raw_reading_total, count );
    printf( "Raw Reading:  %.2f\n", raw_reading );
    printf( "Scaled Reading:  %.3f kg\n", scaled_reading );

    *result = scaled_reading;
    return true;
}

