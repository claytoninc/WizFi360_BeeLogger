/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */

#include <stdio.h>

#include "cmsis_os2.h"
#include "port_common.h"
#include "timer.h"
#include "mqtt_client.h"
#include "temperature_sensors.h"
#include "weight_sensor.h"
#include "humidity_temp_sensors.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */

// Access Details
#define ACCESS_ID       "client"
#define ACCESS_USER     "username"
#define ACCESS_TOKEN    "PaGTfP0IHpv6FuvWewXP"

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */


/* Timer  */
static volatile uint32_t g_msec_cnt = 0;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */

//  Initial setup of Wifi Connection
extern bool socket_startup( void );

//  Check Wifi Connection
extern bool socket_check( void );

// Timer
static void repeating_timer_callback(void);
static time_t millis(void);


/**
 * ----------------------------------------------------------------------------------------------------
 * demo
 * ----------------------------------------------------------------------------------------------------
 */
int demo( void )
{
    bool        temperature1_valid;
    double      temperature1;
    bool        weight_valid;
    double      weight;
    bool        humidity_valid;
    double      humidity;
    bool        ambient_temp_valid;
    double      ambient_temp;
    bool        retb;
    int         cycle_count;
    bool        wifi_ready;
    bool        wifi_state;

    wizchip_1ms_timer_initialize( repeating_timer_callback );

    printf( "Temperature Sensor - Initialise\n" );
    TempSensor_init();
    printf( "Weight Sensor - Initialise\n" );
    WeightSensor_init();
    printf( "Humidity/Temperature Sensor - Initialise\n" );
    HumidityTempSensor_init();

    // setup wifi
    wifi_ready = socket_startup();

    // main loop
    printf( "Start ...\n" );
    cycle_count = 0;
    osDelay( 2000 );
    while ( 1 )
    {
        if ( cycle_count!=0 )
        {   // pause between recordings
            printf( "Pause ...\n" );
            osDelay( 10000 );
        }

        cycle_count++;
        printf( "\n" );
        printf( "***********************\n" );
        printf( "Starting Cycle %d ...\n", cycle_count );

        printf( "Check Wifi ...\n" );
        wifi_state = socket_check();
        if ( wifi_state )
        {   // wifi ok
            printf( "Wifi Ok\n" );
        }
        else
        {   // no wifi
            if ( wifi_ready )
            {
                printf( "Wifi has been lost\n" );
                wifi_ready = false;
            }
            wifi_ready = socket_startup();
            if ( !wifi_ready )
            {   // no wifi - abort cycle
                continue;
            }
        }

        printf( "Read Temperature 1 ...\n" );
        temperature1_valid = TempSensor_read( 4, &temperature1 );
        if ( !temperature1_valid )
        {
            printf( "ERROR - Temperature1 Read Failed\n" );
        }

        printf( "Read Weight ...\n" );
        weight_valid = WeightSensor_read( 4, &weight );
        if ( !weight_valid )
        {
            printf( "ERROR - Weight Read Failed\n" );
        }

        printf( "Read Humidity ...\n" );
        humidity_valid = HumidityTempSensor_read( HUMIDITY_SENSOR, &humidity );
        printf( "Humidity: %.2f %%\n", humidity );
        if ( !humidity_valid )
        {
            printf( "ERROR - Humidity Read Failed\n" );
        }

        printf( "Read Ambient Temperature ...\n" );
        ambient_temp_valid = HumidityTempSensor_read( TEMP_SENSOR, &ambient_temp );
        printf( "Ambient Temperature: %.2f C\n", ambient_temp );
        if ( !ambient_temp_valid )
        {
            printf( "ERROR - Ambient Temperature Read Failed\n" );
        }

        // test for data
        if (    !temperature1_valid && 
                !weight_valid && 
                !humidity_valid && 
                !ambient_temp_valid )
        {   // no valid data - try again
            continue;
        }

        // MQTT operations
        while ( 1 )
        {
            retb = mqtt_connect( ACCESS_ID, ACCESS_USER ) ;
            if ( !retb )
                break;
            if ( temperature1_valid )
            {
                retb = mqtt_send_float( "Temperature1", temperature1 ) ;
                if ( !retb )
                    break;
            }
            if ( weight_valid )
            {
                retb = mqtt_send_float( "Weight", weight ) ;
                if ( !retb )
                    break;
            }
            if ( humidity_valid )
            {
                retb = mqtt_send_float( "Humidity", humidity ) ;
                if ( !retb )
                    break;
            }
            if ( ambient_temp_valid )
            {
                retb = mqtt_send_float( "AmbientTemperature", ambient_temp ) ;
                if ( !retb )
                    break;
            }
            break;
        }
        mqtt_disconnect();
    }
}




/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt++;
}

static time_t millis(void)
{
    return g_msec_cnt;
}

