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


/* Timer  */
static void repeating_timer_callback(void);
static time_t millis(void);


/**
 * ----------------------------------------------------------------------------------------------------
 * demo
 * ----------------------------------------------------------------------------------------------------
 */
int demo( void )
{
    double      temperature;
    double      weight;
    bool        temperature_valid;
    bool        weight_valid;
    bool        retb;
    int         cycle_count;

    printf( "Temperature Sensor - Initialise\n" );
    TempSensor_init();
    printf( "Weight Sensor - Initialise\n" );
    WeightSensor_init();

    printf( "Start ...\n" );

    wizchip_1ms_timer_initialize(repeating_timer_callback);

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

        printf( "Read Temperature ...\n" );
        temperature_valid = TempSensor_read( 4, &temperature );
        if ( !temperature_valid )
        {
            printf( "ERROR - Temperature Read Failed\n" );
        }

        printf( "Read Weight ...\n" );
        weight_valid = WeightSensor_read( 4, &weight );
        if ( !weight_valid )
        {
            printf( "ERROR - Weight Read Failed\n" );
        }

        if ( !temperature_valid && !weight_valid )
        {   // no valid data - try again
            continue;
        }

        while ( 1 )
        {
            retb = mqtt_connect( ACCESS_ID, ACCESS_USER ) ;
            if ( !retb )
                break;
            if ( temperature_valid )
            {
                retb = mqtt_send_float( "Temperature", temperature ) ;
                if ( !retb )
                    break;
            }
            if ( weight_valid )
            {
                retb = mqtt_send_float( "Weight", weight ) ;
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

