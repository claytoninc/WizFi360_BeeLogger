/*---------------------------------------------------------------------------

    MQTT Client
        Simple MQTT publishing to thinksboard
        Uses IoT_Socket library
        Designed for Wizi360 Pico

---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "iot_socket.h"

#include "mqtt_client.h"

// Macros


#define MQTT_MAX_PACKET_SIZE 256

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE 5

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTT_KEEPALIVE 15

#define MQTT_VERSION_3_1_1    4
#define MQTT_VERSION MQTT_VERSION_3_1_1


#define CONNECT_RECV_TIMEOUT_MS     5000

#define MQTT_TELEMETRY_TOPIC        "v1/devices/me/telemetry"

#define MQTT_PORT                   1883

// Data

static char mqtt_server[] = "mqtt.thingsboard.cloud";
static char mqtt_telemetry_topic[] = "v1/devices/me/telemetry";

static uint8_t mqtt_tx_buf[MQTT_MAX_PACKET_SIZE];
static uint8_t mqtt_rx_buf[MQTT_MAX_PACKET_SIZE];
static uint8_t payload_buf[MQTT_MAX_PACKET_SIZE];

static bool connected = false;
static uint8_t sock_id;

// Prototypes







// Functions




//
//  Appends a string onto an MQTT buffer, including the field length field
//  Assumes buffer is big enough
//
static uint16_t append_string_field( const char* string, uint8_t* buf, uint16_t pos ) 
{
    const char* idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp) 
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos-i-2] = (i >> 8);
    buf[pos-i-1] = (i & 0xFF);
    return pos;
}

//
//  Adds the header onto a message buffer
//  Assumes the buffer is passed with a MQTT_MAX_HEADER_SIZE space in front for the header
//  Length is the payload length only
//  Creates the header including the length fields
//  Returns the final header length
//
static uint16_t build_header( uint8_t header, uint8_t* buf, uint16_t length) 
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;

    // create length bytes
    do {
        // create a 7-bit length field
        digit = len  & 127; //digit = len %128
        len >>= 7; //len = len / 128
        if (len > 0) 
        {   // more length - set 'MORE' flag
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while(len>0);

    // copy header into buffer
    buf[4-llen] = header;
    for ( int i=0; i<llen; i++ ) 
    {
        buf[MQTT_MAX_HEADER_SIZE-llen+i] = lenBuf[i];
    }
    // return header size - full header size is variable length bit plus the 1-byte fixed header
    return llen+1;
}


//
//  Display a buffer
//
static void display_buffer( const char *txt, const uint8_t * buf, uint16_t length ) 
{
    uint16_t    ii;
    char        ch;

    printf("%s - %d long\n", txt, length);
    printf("  <");
    for ( ii=0; ii<length; ii++ )
    {
        ch = buf[ii];
        if ( (isprint(ch)==0)  || (ch==' ') )
        {   // unprintable
            printf("\\x%02X", ch );
        }
        else
        {
            printf("%c", ch );
        }
    }
    printf(">\n");
}





// TCP Receive with timeout
static int recv_with_timeout( uint8_t ctx, unsigned char *buf, size_t len, uint32_t timeout )
{
    int32_t ret;
    uint32_t n = timeout;

    ret = iotSocketSetOpt ( ctx, IOT_SOCKET_SO_RCVTIMEO, &n, sizeof(n) );
    if (ret < 0)
        return -999;

    ret = iotSocketRecv ((uint8_t)ctx, buf, len);
    if ( ret == IOT_SOCKET_EAGAIN )
    {   // timeout - received 0 data
        return 0;
    }
    return ret;
}


//
//  Perform an MQTT connect operation
//
bool mqtt_connect( const char *id, const char *user ) 
{
    uint16_t    length;
    uint16_t    header_length;
    uint8_t     *msg_ptr;
    uint16_t    ii;
    char        ch;
    int32_t     af;
    uint8_t     target_ip[4];
    int32_t     size;
    int32_t     retval;

    // disconnect if required
    if ( connected )
    {
        mqtt_disconnect();
    }

    // lookup IP address
    af = IOT_SOCKET_AF_INET;
    size = sizeof(target_ip);
    retval = iotSocketGetHostByName ( mqtt_server, af, target_ip,&size );
    //printf("iotSocketGetHostByName returned %d.   %d.%d.%d.%d\r\n", retval, 
    //                    target_ip[0],target_ip[1],target_ip[2],target_ip[3] );
    if ( retval!=0 )
    {
        printf("IP Address lookup failed (%d)\r\n", retval );
        return false;
    }

    // open connection
    printf("Connecting to %d.%d.%d.%d\n", target_ip[0],target_ip[1],target_ip[2],target_ip[3] );
    sock_id = iotSocketCreate( af, IOT_SOCKET_SOCK_STREAM, IOT_SOCKET_IPPROTO_TCP );
    if ( sock_id<0 )
    {
        printf("Socket Create failed (%d)\r\n", sock_id);
        return false;
    }
    retval = iotSocketConnect (sock_id, target_ip, 4, MQTT_PORT );
    if (retval < 0)
    {
        iotSocketClose( sock_id );
        printf("Socket Connect failed (%d)\r\n", retval);
        return false;
    }

    // build connect message
    length = MQTT_MAX_HEADER_SIZE;
    mqtt_tx_buf[length++] = 0x00;
    mqtt_tx_buf[length++] = 0x04;
    mqtt_tx_buf[length++] = 'M';
    mqtt_tx_buf[length++] = 'Q';
    mqtt_tx_buf[length++] = 'T';
    mqtt_tx_buf[length++] = 'T';
    mqtt_tx_buf[length++] = MQTT_VERSION;
    mqtt_tx_buf[length++] = 0x82;      // bitmap of fields
    mqtt_tx_buf[length++] = MQTT_KEEPALIVE >> 8;
    mqtt_tx_buf[length++] = MQTT_KEEPALIVE & 0xFF;
    length = append_string_field( id, mqtt_tx_buf,length );
    length = append_string_field( user, mqtt_tx_buf,length );
    length -= MQTT_MAX_HEADER_SIZE;
    header_length = build_header( MQTTCONNECT, mqtt_tx_buf, length );
    msg_ptr = &(mqtt_tx_buf[MQTT_MAX_HEADER_SIZE-header_length]);
    length += header_length;

    display_buffer( "MQTT Connect message", msg_ptr, length );

    // send message
    retval = iotSocketSend( sock_id, msg_ptr, length );
    //printf("iotSocketSend retval = %d\n", retval);
    if ( retval!=length )
    {
        printf("Failed to send connect message (%d)\n", retval);
        iotSocketClose( sock_id );
        return false;
    }

    // Wait for response
    retval = recv_with_timeout( sock_id, mqtt_rx_buf,sizeof(mqtt_rx_buf), CONNECT_RECV_TIMEOUT_MS );
    if ( retval < 0 )
    {   // no response
        printf("No connection response received - connection failed\n" );
        iotSocketClose( sock_id );
        return false;
    }
    else if ( retval==0 )
    {   // no response
        printf("No connection response received - timeout\n" );
        iotSocketClose( sock_id );
        return false;
    }
    // otherwise a message is present
    display_buffer( "MQTT Response message", mqtt_rx_buf, retval );
    if ( retval!=4 )
    {   // bad response
        printf("Bad connection response received - bad length\n" );
        iotSocketClose( sock_id );
        return false;
    }
    if ( mqtt_rx_buf[0]!=MQTTCONNACK )
    {   // bad response
        printf("Bad connection response received - bad message type\n" );
        iotSocketClose( sock_id );
        return false;
    }
    if ( mqtt_rx_buf[3]!=0 )
    {   // error
        printf("Bad connection response received - bad return code\n" );
        iotSocketClose( sock_id );
        return false;
    }
    // Connected!
    printf("MQTT connection established\n" );
    connected = true;
    return true;
}

//
//  Disconnect the connection
//
void mqtt_disconnect( void ) 
{
    if ( connected )
    {
        printf("MQTT disconnected\n" );
        iotSocketClose( sock_id );
        connected = false;
    }
}

//
//  Sends MQTT float telementry
//
bool mqtt_send_float( const char *key, double value ) 
{
    uint16_t    length;
    uint16_t    header_length;
    uint8_t     *msg_ptr;
    uint16_t    ii;
    char        ch;
    int32_t     retval;

    if ( !connected )
    {
        printf("Attended to send data when not connected\n" );
        return false;
    }

    // Build message
    sprintf( payload_buf, "{\"%s\":%.3f}", key, value );
    length = MQTT_MAX_HEADER_SIZE;
    length = append_string_field( mqtt_telemetry_topic, mqtt_tx_buf,length );
    msg_ptr = payload_buf;
    while (*msg_ptr) 
    {
        mqtt_tx_buf[length++] = *msg_ptr++;
    }
    length -= MQTT_MAX_HEADER_SIZE;
    header_length = build_header( MQTTPUBLISH, mqtt_tx_buf, length );
    msg_ptr = &(mqtt_tx_buf[MQTT_MAX_HEADER_SIZE-header_length]);
    length += header_length;
    display_buffer( "MQTT Telementry message", mqtt_tx_buf, length );

    // send message
    retval = iotSocketSend( sock_id, msg_ptr, length );
    //printf("iotSocketSend retval = %d\r\n", retval);
    if ( retval!=length )
    {
        printf("Failed to telementry message (%d) - disconnected\n", retval);
        mqtt_disconnect();
        return false;
    }
    return true;
}
