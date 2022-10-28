/*---------------------------------------------------------------------------

    MQTT Client
        Routines for a simple MQTT publish only client to thingsboard

    clayton@isnotcrazy.com

---------------------------------------------------------------------------*/

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//  Perform an MQTT connect operation
//
bool mqtt_connect( const char *id, const char *user ) ;

//
//  Disconnect the connection
//
void mqtt_disconnect( void ) ;

//
//  Sends MQTT float telementry
//
bool mqtt_send_float( const char *key, double value ) ;

#ifdef __cplusplus
}
#endif

#endif      // MQTT_CLIENT_H
