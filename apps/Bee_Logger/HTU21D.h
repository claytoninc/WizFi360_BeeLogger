/* 

 HTU21D Humidity Sensor Library for RPi Pico
 Ported to pico by clayton@isnotcrazy.com

 Derived from the Arduino SparkFun_HTU21D_Humidity_and_Temperature_Sensor_Breakout library


 HTU21D Humidity Sensor Library
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 22nd, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Get humidity and temperature from the HTU21D sensor.
 
 This same library should work for the other similar sensors including the Si
 
 */
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

class HTU21D
{
  public:
    HTU21D();

    //Public Functions
    void begin( int i2cport, int SDA_Pin, int SCL_pin );
    bool readHumidity( double *value );
    bool readTemperature( double *value );
//    void setResolution( uint8_t resBits );

//    uint8_t readUserRegister( void );
//    void writeUserRegister( uint8_t val );

    //Public Variables

  private:
    //Private Functions
    uint8_t checkCRC( uint16_t message_from_sensor, uint8_t check_value_from_sensor );
    bool readValue( uint8_t cmd, uint16_t *value );

    //Private Variables
    i2c_inst_t *i2c_port;

};
