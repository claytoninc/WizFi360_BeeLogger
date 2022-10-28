/* -----------------------------------------------------------------------------
 * Copyright (c) 2020 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdio.h>
#include "cmsis_os2.h"
//#include "main.h"

#include "RTE_Components.h"
//#include  CMSIS_device_header
#include "Driver_I2C.h"                 // ::CMSIS Driver:I2C

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"


extern void app_initialize (void);
extern int32_t socket_startup (void);
extern int32_t demo (void);

static const osThreadAttr_t app_main_attr = 
{
  .stack_size = 4096U
};


/*-----------------------------------------------------------------------------
 * I2C Operations
 *----------------------------------------------------------------------------*/

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
static bool reserved_addr(uint8_t addr) 
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

static void do_I2C( void )
{
    int counter = 1;
    int addr = 0;
    uint init;

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/bus_scan example requires a board with I2C pins
  while (1)
  {
    printf("Default I2C pins were not defined\n");
    osDelay(1000);
  }
#endif

    osDelay(1000);
    printf("Setting up I2C..%d - %d %d.\n", PICO_DEFAULT_I2C, PICO_DEFAULT_I2C_SCL_PIN,PICO_DEFAULT_I2C_SDA_PIN);
    osDelay(1000);
    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a Pico)
    init = i2c_init(i2c_default, 100 * 1000);
    printf("Init returned %u\n", init);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    while ( true )
    {

        printf("\nI2C Bus Scan - %d\n", counter );
        printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

        for (addr = 0; addr < (1 << 7); ++addr) {
            if (addr % 16 == 0) {
                printf("%02x ", addr);
            }

            // Perform a 1-byte dummy read from the probe address. If a slave
            // acknowledges this address, the function returns the number of bytes
            // transferred. If the address byte is ignored, the function returns
            // -1.

            // Skip over any reserved addresses.
            int ret;
            uint8_t rxdata = 0xFE;
       //     if (reserved_addr(addr))
       //         ret = PICO_ERROR_GENERIC;
       //     else
                //ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);
                ret = i2c_write_blocking(i2c_default, addr, &rxdata, 1, false);
/*
            printf( "I2C Read\n" );
                ret = i2c_read_blocking(i2c_default, 0x40, &rxdata, 1, false);
            printf( "I2C Op: %d  data %d\n", ret, rxdata );
          osDelay(500);
            printf( "I2C Write\n" );
                rxdata = 0xFE;
                ret = i2c_write_blocking(i2c_default, 0x40, &rxdata, 1, false);
            printf( "I2C Op: %d\n", ret );
*/

            printf(ret < 0 ? "." : "@");
            printf(addr % 16 == 15 ? "\n" : "  ");
        }
        printf("Done.\n");
        osDelay(2000);
        counter++;
    }
}

#if 0

/* I2C A Driver, controls Slave Device 0, uses underlying Driver_I2C1 (see I2C_MultiSlave_Config.h) */
extern ARM_DRIVER_I2C         Driver_I2C10;
#define I2C_A               (&Driver_I2C10)
 

__NO_RETURN static void I2C_Thread(void *argument) 
{
  uint8_t addr;
  uint8_t reg;
  uint8_t val;
  (void)argument;
 
   printf("\n\n");
  osDelay(3000);
  printf("\n\n");
  printf("I2C_Thread is running\n");

  /* Initialize and configure I2C */
  I2C_A->Initialize  (NULL);
  I2C_A->PowerControl(ARM_POWER_FULL);
  I2C_A->Control     (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
  I2C_A->Control     (ARM_I2C_BUS_CLEAR, 0);
 
  /* Periodically read device register at address 0x0F */
  addr = 0x68;
  reg  = 0x0F;
 
  while(1) {
    I2C_A->MasterTransmit(addr, &reg, 1, true);
    while (I2C_A->GetStatus().busy);
 
    I2C_A->MasterReceive (addr, &val, 1, false);
    while (I2C_A->GetStatus().busy);
 
    osDelay(10);
  }
}
 #endif






extern double DoOneWire( void );




/*-----------------------------------------------------------------------------
 * Application main thread
 *----------------------------------------------------------------------------*/
static void app_main (void *argument) 
{
  int32_t status;

  printf("\n\n");
  osDelay(3000);
  osDelay(1000);
  printf("\n");
  osDelay(1000);
  printf("\n");
  osDelay(1000);
  printf("\n");
  osDelay(1000);
  printf("\n");
  printf("app_main Thread is running\n");

// #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
//     #warning i2c / bmp280_i2c example requires a board with I2C pins
//     printf("Default I2C pins were not defined\n");
// #else
//     printf("Hello, Setting up I2C...\n");

//     // useful information for picotool
//     bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
//     bi_decl(bi_program_description("Bee_Logger for Raspberry Pi Pico"));
// #endif
//     // I2C is "open drain", pull ups to keep signal high when no data is being sent
//     i2c_init(i2c_default, 100 * 1000);
//     gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
//     gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
//     gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
//     gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);


//do_I2C();



    while ( 0 )
    {
      osDelay(1000);
      printf("One Wire ...\n");
      osDelay(500);
      //osKernelLock();
      DoOneWire();
      //osKernelUnlock();
    }
  
  printf("Starting Socket\r\n");
  status = socket_startup();
  if (status == 0) 
  {
    demo();
  }
}

/*-----------------------------------------------------------------------------
 * Application initialization
 *----------------------------------------------------------------------------*/
void app_initialize (void) 
{
  osThreadNew(app_main, NULL, &app_main_attr);
  //osThreadNew(I2C_Thread, NULL, &app_main_attr);

  
}


