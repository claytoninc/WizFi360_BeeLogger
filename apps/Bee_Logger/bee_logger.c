/*---------------------------------------------------------------------------

    Beehive Logging Application
        Reads weight, temperature and humidity data, and 
        records it to an MQTT server

    clayton@isnotcrazy.com

    WizFi360-EVB-Pico

---------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
#include "port_common.h"
#include "FreeRTOSConfig.h"
#include "timer.h"
#include "Driver_WiFi.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"

#include "mqtt_client.h"
#include "temperature_sensors.h"
#include "weight_sensor.h"
#include "humidity_temp_sensors.h"

// ----------------------------------------------------------------------------------------------------
//  MACROS
// ----------------------------------------------------------------------------------------------------

// Clock
#define PLL_SYS_KHZ (133 * 1000)

//  WiFi Connection Details
#define SSID            "TheCloud"
#define PASSWORD        "letITrain"
#define SECURITY_TYPE   ARM_WIFI_SECURITY_WPA2

// MQTT Access Details
#define ACCESS_ID       "beehive001"
#define ACCESS_USER     "beekeeper1"

#define ADC_PIN         28

// ----------------------------------------------------------------------------------------------------
//  DATA
// ----------------------------------------------------------------------------------------------------

static const osThreadAttr_t app_main_attr = 
{
    .stack_size = 4096U
};

// Timer
static volatile uint32_t msec_counter = 0;

// Wifi device
extern ARM_DRIVER_WIFI Driver_WiFi1;

// ----------------------------------------------------------------------------------------------------
//  FUNCTIONS
// ----------------------------------------------------------------------------------------------------

// Private
static void set_clock_khz(void);
static void app_main (void *argument);
static void application( void );
static bool socket_check( void );
static bool socket_startup( void );

// Timer
static void repeating_timer_callback(void)
{
    msec_counter++;
}
static time_t millis(void)
{
    return msec_counter;
}

// ----------------------------------------------------------------------------------------------------
//  MAIN Functions
// ----------------------------------------------------------------------------------------------------

//
//  Main Function
//    Set up and start OS
//
int main (void)
{
    set_clock_khz();

    SystemCoreClockUpdate();
    stdio_init_all();

    // set up watchdog
    watchdog_enable( 8000, true );
    watchdog_update();
 
    // Initialize CMSIS-RTOS
    osKernelInitialize();
    // Create Thread
    osThreadNew( app_main, NULL, &app_main_attr );
    // Start Kernel
    osKernelStart();
    // wait (should never get here)
    for (;;)
        {}
}

//  Set up Clock
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
                  clk_peri,
                  0,                                                // No glitchless mux
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
                  PLL_SYS_KHZ * 1000,                               // Input frequency
                  PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
                  );
}

//  Application main thread
static void app_main (void *argument)
{
    printf("\n\n");
    osDelay(1000);
    printf("\n");
    watchdog_update();
    osDelay(1000);
    printf("\n");
    osDelay(1000);
    printf("\n");
    osDelay(1000);
    watchdog_update();
    printf("\n");
    printf("=======================================================\n");
    printf("\n");
    printf("      Beehive Logging Application                      \n");
    printf("        Reads weight, temperature and humidity data,   \n");
    printf("           and records it to an MQTT server            \n");
    printf("      Hardware:  WizFi360-EVB-Pico                     \n");
    printf("\n");
    printf("      clayton@isnotcrazy.com                           \n");
    printf("\n");
    printf("=======================================================\n");
    printf("\n");
    printf("\n");

    wizchip_1ms_timer_initialize( repeating_timer_callback );

    watchdog_update();
    application();
}

// ----------------------------------------------------------------------------------------------------
//  APPLICATION Functions
// ----------------------------------------------------------------------------------------------------

void application( void )
{
    double      voltage;
    double      scaled_voltage;
    bool        temperature1_valid;
    double      temperature1;
    bool        temperature2_valid;
    double      temperature2;
    bool        temperature3_valid;
    double      temperature3;
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
    uint16_t    reading;

    printf( "ADC - Initialise\n" );
    adc_init();
    adc_gpio_init( ADC_PIN );
    adc_select_input( ADC_PIN-26 );
    printf( "Temperature Sensor - Initialise\n" );
    TempSensor_init();
    printf( "Weight Sensor - Initialise\n" );
    WeightSensor_init();
    printf( "Humidity/Temperature Sensor - Initialise\n" );
    HumidityTempSensor_init();

    watchdog_update();

    // setup wifi
    printf("Connecting to WiFi ...\n");
    wifi_ready = socket_startup();
    watchdog_update();

    // main loop
    printf( "Start ...\n" );
    cycle_count = 0;
    osDelay( 2000 );
    watchdog_update();
    while ( 1 )
    {
        if ( cycle_count!=0 )
        {   // pause between recordings
            printf( "Pause ...\n" );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
            watchdog_update();
            osDelay( 2000 );
        }

        cycle_count++;
        printf( "\n" );
        printf( "***********************\n" );
        printf( "Starting Cycle %d ...\n", cycle_count );
        watchdog_update();

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
        watchdog_update();

        // Vin to ADC pin = 200k
        // ADC to GND = 22k
        //  Vref = 3.3V
        // however measurements give:
        //      Vref = 3.33
        //      Ratio = 9.728
        //      Vdiode = 0.804
        printf( "Read Voltage ...\n" );
        reading = adc_read();
        voltage = reading * 3.33 / (1 << 12);
        scaled_voltage = (voltage * 9.728) + 0.804;        // scale for resistors and diode-drop
        printf( "  Reading %u   Voltage %.3f  Scaled-Voltage %.2f\n", reading, voltage, scaled_voltage );

        printf( "Read Temperature 1 ...\n" );
        temperature1_valid = TempSensor_read( SENSORS_T1, &temperature1 );
        if ( !temperature1_valid )
        {
            printf( "ERROR - Temperature1 Read Failed\n" );
        }

        printf( "Read Temperature 2 ...\n" );
        temperature2_valid = TempSensor_read( SENSORS_T2, &temperature2 );
        if ( !temperature2_valid )
        {
            printf( "ERROR - Temperature2 Read Failed\n" );
        }

        printf( "Read Temperature 3 ...\n" );
        temperature3_valid = TempSensor_read( SENSORS_T3, &temperature3 );
        if ( !temperature3_valid )
        {
            printf( "ERROR - Temperature3 Read Failed\n" );
        }
        watchdog_update();

        printf( "Read Weight ...\n" );
        weight_valid = WeightSensor_read( 4, &weight );
        if ( !weight_valid )
        {
            printf( "ERROR - Weight Read Failed\n" );
        }
        watchdog_update();

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
        watchdog_update();

        // test for data
        if (    !temperature1_valid && 
                !temperature2_valid && 
                !temperature3_valid && 
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
            watchdog_update();
            if ( !retb )
                break;
            retb = mqtt_send_float( "Voltage1", voltage ) ;
            if ( !retb )
                break;
            watchdog_update();
            if ( temperature1_valid )
            {
                retb = mqtt_send_float( "Temperature1", temperature1 ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            if ( temperature2_valid )
            {
                retb = mqtt_send_float( "Temperature2", temperature2 ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            if ( temperature3_valid )
            {
                retb = mqtt_send_float( "Temperature3", temperature3 ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            if ( weight_valid )
            {
                retb = mqtt_send_float( "Weight", weight ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            if ( humidity_valid )
            {
                retb = mqtt_send_float( "Humidity", humidity ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            if ( ambient_temp_valid )
            {
                retb = mqtt_send_float( "AmbientTemperature", ambient_temp ) ;
                if ( !retb )
                    break;
            }
            watchdog_update();
            break;
        }
        mqtt_disconnect();
        watchdog_update();
    }
}

//
//  Initial setup of Wifi Connection
//
bool socket_startup( void )
{
    ARM_WIFI_CONFIG_t config;
    int32_t ret;
    uint8_t net_info[4];
    int32_t len;

    ret = Driver_WiFi1.Initialize(NULL);
    printf("Driver_WiFix.Initialize(NULL) = %d\n", ret);

    ret = Driver_WiFi1.PowerControl(ARM_POWER_FULL);
    printf("Driver_WiFix.PowerControl(ARM_POWER_FULL) = %d\n", ret);

    memset((void *)&config, 0, sizeof(config));
    config.ssid     = SSID;
    config.pass     = PASSWORD;
    config.security = SECURITY_TYPE;
    config.ch       = 0U;

    ret = Driver_WiFi1.Activate(0U, &config);
    printf("Driver_WiFix.Activate(0U, &config) = %d\n", ret);

    ret = Driver_WiFi1.IsConnected();  
    printf("Driver_WiFix.IsConnected() = %d\n", ret);

    if ( ret==0U ) 
    {
        printf("WiFi network connection failed!\n");
        return false;
    }

    // Connected
    printf("WiFi network connection succeeded!\n");

    len = sizeof(net_info);
    Driver_WiFi1.GetOption(0, ARM_WIFI_IP, net_info, &len );
    printf("ARM_WIFI_IP = %d.%d.%d.%d\n", net_info[0], net_info[1], net_info[2], net_info[3]);

    len = sizeof(net_info);
    Driver_WiFi1.GetOption(0, ARM_WIFI_IP_SUBNET_MASK, net_info, &len );
    printf("ARM_WIFI_IP_SUBNET_MASK = %d.%d.%d.%d\n", net_info[0], net_info[1], net_info[2], net_info[3]);

    len = sizeof(net_info);
    Driver_WiFi1.GetOption(0, ARM_WIFI_IP_GATEWAY, net_info, &len );
    printf("ARM_WIFI_IP_GATEWAY = %d.%d.%d.%d\n", net_info[0], net_info[1], net_info[2], net_info[3]);

    return true;
}

//
//  Check Wifi Connection
//
bool socket_check( void )
{
    int32_t ret;

    ret = Driver_WiFi1.IsConnected();  
    //printf("Driver_WiFix.IsConnected() = %d\n", ret);
    if ( ret==0U ) 
    {
        printf("WiFi network connection failed!\n");
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------------------------------
//  END
// ----------------------------------------------------------------------------------------------------
