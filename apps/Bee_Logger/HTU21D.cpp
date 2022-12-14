/*
  HTU21D Humidity Sensor Library for RPi Pico
  Ported to pico by clayton@isnotcrazy.com

  Derived from the Arduino SparkFun_HTU21D_Humidity_and_Temperature_Sensor_Breakout library


  HTU21D Humidity Sensor Library
  By: Nathan Seidle
  SparkFun Electronics
  Date: September 22nd, 2013
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  This library allows an Arduino to read from the HTU21D low-cost high-precision humidity sensor.

  If you have feature suggestions or need support please use the github support page: https://github.com/sparkfun/HTU21D

  Hardware Setup: The HTU21D lives on the I2C bus. Attach the SDA pin to A4, SCL to A5. If you are using the SparkFun
  breakout board you *do not* need 4.7k pull-up resistors on the bus (they are built-in).

  Link to the breakout board product:

  Software:
  Call HTU21D.Begin() in setup.
  HTU21D.ReadHumidity() will return a float containing the humidity. Ex: 54.7
  HTU21D.ReadTemperature() will return a float containing the temperature in Celsius. Ex: 24.1
  HTU21D.SetResolution(byte: 0b.76543210) sets the resolution of the readings.
  HTU21D.check_crc(message, check_value) verifies the 8-bit CRC generated by the sensor
  HTU21D.read_user_register() returns the user register. Used to set resolution.
*/

#include <stdint.h>
#include <stdio.h>
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "HTU21D.h"

#define HTU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor

#define ERROR_I2C_TIMEOUT 	998
#define ERROR_BAD_CRC		999

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

#define USER_REGISTER_RESOLUTION_MASK 0x81
#define USER_REGISTER_RESOLUTION_RH12_TEMP14 0x00
#define USER_REGISTER_RESOLUTION_RH8_TEMP12 0x01
#define USER_REGISTER_RESOLUTION_RH10_TEMP13 0x80
#define USER_REGISTER_RESOLUTION_RH11_TEMP11 0x81

#define USER_REGISTER_END_OF_BATTERY 0x40
#define USER_REGISTER_HEATER_ENABLED 0x04
#define USER_REGISTER_DISABLE_OTP_RELOAD 0x02

HTU21D::HTU21D()
{
  //Set initial values for private vars
}

//Begin
/*******************************************************************************************/
//Start I2C communication
void HTU21D::begin( int i2cport, int SDA_Pin, int SCL_pin )
{
  int  retval;

  // select port
  if ( i2cport==1 )
    i2c_port = i2c1;
  else
    i2c_port = i2c0;
  // init port and pins
  retval = i2c_init( i2c_port, 100 * 1000 );
  //printf("Init returned %d\n", retval );
  gpio_set_function( SDA_Pin, GPIO_FUNC_I2C );
  gpio_pull_up( SDA_Pin );
  gpio_set_function( SCL_pin, GPIO_FUNC_I2C );
  gpio_pull_up( SCL_pin );
}

#define MAX_WAIT 100
#define DELAY_INTERVAL 10
#define MAX_COUNTER (MAX_WAIT/DELAY_INTERVAL)

//Given a command, reads a given 2-byte value with CRC from the HTU21D
bool HTU21D::readValue( uint8_t cmd, uint16_t *value )
{
  int       retval;
  uint8_t   read_buf[3] = {0};
  uint16_t  rawValue;
  uint32_t  timeout;

  // Request a reading. Read 3 bytes - high-byte low-byte crc

  // issue command
  retval = i2c_write_timeout_us( i2c_port, HTU21D_ADDRESS, &cmd,1, false, 1000 );
  if ( retval!=1 )
  {
    printf("i2c_write_timeout_us returned %d\n", retval );
    return false;
  }
  // wait for valid response when ready
  timeout = 100000;
  while ( timeout>0 )
  {
    retval = i2c_read_timeout_us( i2c_port, HTU21D_ADDRESS, read_buf,3, false, 1000 );
    if ( retval>0 )
      break;
    sleep_us(50);
    timeout -= 50;
  }
  if ( retval!=3 )
  {
    printf("i2c_read_timeout_us returned %d\n", retval );
    return false;
  }
  rawValue = ( ((uint16_t)(read_buf[0])) << 8) | ((uint16_t)(read_buf[1]));
  //printf("raw value %d\n", rawValue );
  if ( checkCRC(rawValue, read_buf[2]) != 0 )
  {
    printf("checksum failed (%d,%d,%d)\n", read_buf[0],read_buf[1],read_buf[2] );
    return false;
  }
  *value = rawValue & 0xFFFC; // Zero out the status bits
  return true;
}

//Read the humidity
bool HTU21D::readHumidity( double *value )
{
  bool      retb;
  uint16_t  rawHumidity;
  double    tempHumidity;
  
  retb = readValue( TRIGGER_HUMD_MEASURE_NOHOLD, &rawHumidity );
  if ( !retb )
    return false;

  //Given the raw humidity data, calculate the actual relative humidity
  tempHumidity = rawHumidity * (125.0 / 65536.0);     //2^16 = 65536
  *value = tempHumidity - 6.0;                        //From page 14
  return true;
}

//Read the temperature
bool HTU21D::readTemperature( double *value )
{
  bool      retb;
  uint16_t  rawTemperature;
  double    tempTemperature;
  
  retb = readValue( TRIGGER_TEMP_MEASURE_NOHOLD, &rawTemperature );
  if ( !retb )
    return false;

  // Given the raw temperature data, calculate the actual temperature
  tempTemperature = rawTemperature * (175.72 / 65536.0);  //2^16 = 65536
  *value = tempTemperature - 46.85;                       //From page 14
  return true;
}

#if 0

//Set sensor resolution
/*******************************************************************************************/
//Sets the sensor resolution to one of four levels
//Page 12:
// 0/0 = 12bit RH, 14bit Temp
// 0/1 = 8bit RH, 12bit Temp
// 1/0 = 10bit RH, 13bit Temp
// 1/1 = 11bit RH, 11bit Temp
//Power on default is 0/0

void HTU21D::setResolution(uint8_t resolution)
{
  uint8_t userRegister = readUserRegister(); //Go get the current register state
  userRegister &= 0x7E;   // B01111110; //Turn off the resolution bits
  resolution &= 0x81;     // B10000001; //Turn off all other bits but resolution bits
  userRegister |= resolution; //Mask in the requested resolution bits

  //Request a write to user register
  writeUserRegister(userRegister);
}

//Read the user register
uint8_t HTU21D::readUserRegister(void)
{
  uint8_t userRegister;

  //Request the user register
  _i2cPort->beginTransmission(HTU21D_ADDRESS);
  _i2cPort->write(READ_USER_REG); //Read the user register
  _i2cPort->endTransmission();

  //Read result
  _i2cPort->requestFrom(HTU21D_ADDRESS, 1);

  userRegister = _i2cPort->read();

  return (userRegister);
}

void HTU21D::writeUserRegister(uint8_t val)
{
  _i2cPort->beginTransmission(HTU21D_ADDRESS);
  _i2cPort->write(WRITE_USER_REG); //Write to the user register
  _i2cPort->write(val); //Write the new resolution bits
  _i2cPort->endTransmission();
}

#endif


//Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted
//From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
//POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
#define SHIFTED_DIVISOR 0x988000 //This is the 0x0131 polynomial shifted to farthest left of three bytes

uint8_t HTU21D::checkCRC(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
  //Test cases from datasheet:
  //message = 0xDC, checkvalue is 0x79
  //message = 0x683A, checkvalue is 0x7C
  //message = 0x4E85, checkvalue is 0x6B

  uint32_t remainder = (uint32_t)message_from_sensor << 8; //Pad with 8 bits because we have to add in the check value
  remainder |= check_value_from_sensor; //Add on the check value

  uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;

  for (int i = 0 ; i < 16 ; i++) //Operate on only 16 positions of max 24. The remaining 8 are our remainder and should be zero when we're done.
  {
    //Serial.print("remainder: ");
    //Serial.println(remainder, BIN);
    //Serial.print("divsor:    ");
    //Serial.println(divsor, BIN);
    //Serial.println();

    if ( remainder & (uint32_t)1 << (23 - i) ) //Check if there is a one in the left position
      remainder ^= divsor;

    divsor >>= 1; //Rotate the divsor max 16 times so that we have 8 bits left of a remainder
  }

  return (uint8_t)remainder;
}