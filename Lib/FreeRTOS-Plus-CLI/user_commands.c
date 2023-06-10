/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* STM includes. */

#include "main.h"

#include "stm32f4xx_hal.h"

#include "stm32fxx_hal_eth.h"

/* ST includes. */
#if defined( STM32F7xx )
    #include "stm32f7xx_hal.h"
    #define CACHE_LINE_SIZE    32u
#elif defined( STM32F4xx )
    #include "stm32f4xx_hal.h"
#elif defined( STM32F2xx )
    #include "stm32f2xx_hal.h"
#elif defined( STM32F1xx )
    #include "stm32f1xx_hal.h"
#elif !defined( _lint ) /* Lint does not like an #error */
    #error What part?
#endif /* if defined( STM32F7xx ) */


/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* Logging includes. */
#include "logging.h"

/* CLI includes. */
#include "FreeRTOS_CLI.h"

/* Pcap capture includes. */
#include "pcap_capture.h"

#include "iperf_task.h"

#include "user_commands.h"
#include "plus_tcp_demo_cli.h"

/* A semaphore to become idle. */
SemaphoreHandle_t xServerSemaphore;

BaseType_t xHandleTestingCommand( char * pcCommand,
                                  size_t uxLength );

const char * pcCommandList[] =
{
        "arpqc 192.168.1.5",    // a public IP-address */
    /*    "arpqc fe80::ba27:ebff:fe5a:d751", // a gateway */
    /*    "arpqc 192.168.2.1", */
    /*    "arpqc 172.217.194.100", */
    /*    "dnsq4 google.de", */
    /*    "dnsq6 google.nl", */

    /*    "arpqc 192.168.2.1", */
    /*    "arpqc 192.168.2.10", */
    /*    "arpqc 172.217.194.100", */
    /*    "arpqc 2404:6800:4003:c0f::5e", */
    "ifconfig",
    /*      "udp 192.168.2.255@2402 Hello", */
    /*      "udp 192.168.2.255@2402 Hello", */
    /*      "udp 192.168.2.255@2402 Hello", */
    /*      "udp 192.168.2.255@2402 Hello", */
    /*      "http 192.168.2.11 /index.html 33001", */
    /*      "http 2404:6800:4003:c05::5e /index.html 80", */
    /*      "ping6 2606:4700:f1::1", */
    /*      "ping6 2606:4700:f1::1", */
          "dnsq4  aws.amazon.com",
          "dnsq6  google.nl",
    /*      "dnsq4  google.es", */
    /*      "dnsq6  google.co.uk", */
    /*      "udp 192.168.2.11@7 Hello world 1\r\n", */
    /*      "udp fe80::715e:482e:4a3e:d081@7 Hello world 1\r\n", */
    /*      "dnsq4  google.de", */
    /*      "dnsq6  google.nl", */
    /*      "dnsq4  google.es", */
    /*      "dnsq6  google.co.uk", */

    /*      "ntp6a 2.europe.pool.ntp.org", */
    /*      "ping4c 74.125.24.94", */
    /*      "ping4c 192.168.2.1", */
    /*      "ping4c 192.168.2.10", */
    /*      "ping6c 2404:6800:4003:c11::5e", */
    /*      "ping6c 2404:6800:4003:c11::5e", */

    /*      "ping4 raspberrypi.local", */
    /*      "ping6 2404:6800:4003:c0f::5e", */

    /*      "http4 google.de /index.html", */
    /*      "http6 google.nl /index.html", */
    /*      "ping4 10.0.1.10", */
    /*      "ping4 192.168.2.1", */
    /*      "dnsq4 amazon.com", */
    /*      "ping6 google.de", */
    /*      "ntp6a 2.europe.pool.ntp.org", */
};

void show_single_addressinfo( const char * pcFormat,
                              const struct freertos_addrinfo * pxAddress )
{
    char cBuffer[ 40 ];
    const uint8_t * pucAddress;

    #if ( ipconfigUSE_IPv6 != 0 )
        if( pxAddress->ai_family == FREERTOS_AF_INET6 )
        {
            struct freertos_sockaddr * sockaddr6 = ( ( struct freertos_sockaddr * ) pxAddress->ai_addr );

            pucAddress = ( const uint8_t * ) &( sockaddr6->sin_address.xIP_IPv6 );
        }
        else
    #endif /* ( ipconfigUSE_IPv6 != 0 ) */
    {
        pucAddress = ( const uint8_t * ) &( pxAddress->ai_addr->sin_address.ulIP_IPv4 );
    }

    ( void ) FreeRTOS_inet_ntop( pxAddress->ai_family, ( const void * ) pucAddress, cBuffer, sizeof( cBuffer ) );

    if( pcFormat != NULL )
    {
        FreeRTOS_printf( ( pcFormat, cBuffer ) );
    }
    else
    {
        FreeRTOS_printf( ( "Address: %s\n", cBuffer ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief For testing purposes: print a list of DNS replies.
 * @param[in] pxAddress: The first reply received ( or NULL )
 */
void show_addressinfo( const struct freertos_addrinfo * pxAddress )
{
    const struct freertos_addrinfo * ptr = pxAddress;
    BaseType_t xIndex = 0;

    while( ptr != NULL )
    {
        show_single_addressinfo( "Found Address: %s\n", ptr );

        ptr = ptr->ai_next;
    }

    /* In case the function 'FreeRTOS_printf()` is not implemented. */
    ( void ) xIndex;
}

void handle_user_test( char * pcBuffer )
{
}

void prvServerWorkTask( void * pvArgument )
{
    BaseType_t xCommandIndex = 0;
    Socket_t xSocket;

    ( void ) pvArgument;
    FreeRTOS_printf( ( "prvServerWorkTask started\n" ) );

    xServerSemaphore = xSemaphoreCreateBinary();
    configASSERT( xServerSemaphore != NULL );

    /* pcap_prepare(); */

    /* Wait for all end-points to come up.
     * They're counted with 'uxNetworkisUp'. */
    // do
    // {
    //     vTaskDelay( pdMS_TO_TICKS( 100U ) );
    // } while( uxNetworkisUp != mainNETWORK_UP_COUNT );

    // xDNS_IP_Preference = xPreferenceIPv6;

    // {
    //     xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
    //     struct freertos_sockaddr xAddress;

    //     ( void ) memset( &( xAddress ), 0, sizeof( xAddress ) );
    //     xAddress.sin_family = FREERTOS_AF_INET6;
    //     xAddress.sin_port = FreeRTOS_htons( 5000U );

    //     BaseType_t xReturn = FreeRTOS_bind( xSocket, &xAddress, ( socklen_t ) sizeof( xAddress ) );
    //     FreeRTOS_printf( ( "Open socket %d bind = %d\n", xSocketValid( xSocket ), xReturn ) );
    //     TickType_t xTimeoutTime = pdMS_TO_TICKS( 10U );
    //     FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xTimeoutTime, sizeof( TickType_t ) );
    // }

    for( ; ; )
    {
        char pcCommand[ 129 ];
        TickType_t uxTickCount = pdMS_TO_TICKS( 200U );

        if( xCommandIndex < ARRAY_SIZE( pcCommandList ) )
        {
            // while( uxTickCount != 0 )
            // {
            //     xHandleTesting();
            //     xSemaphoreTake( xServerSemaphore, pdMS_TO_TICKS( 10 ) );
            //     uxTickCount--;
            // }

            /*          vTaskDelay( pdMS_TO_TICKS( 1000U ) ); */
            FreeRTOS_printf( ( "\n" ) );

            snprintf( pcCommand, sizeof( pcCommand ), "%s", pcCommandList[ xCommandIndex ] );
            FreeRTOS_printf( ( "\n" ) );
            FreeRTOS_printf( ( "/*==================== %s (%d/%d) ====================*/\n",
                               pcCommand, xCommandIndex + 1, ARRAY_SIZE( pcCommandList ) ) );
            FreeRTOS_printf( ( "\n" ) );
            xHandleTestingCommand( pcCommand, sizeof( pcCommand ) );
            xCommandIndex++;
        }
        else if( xCommandIndex == ARRAY_SIZE( pcCommandList ) )
        {
            FreeRTOS_printf( ( "Server task now ready.\n" ) );


            #if ( ipconfigUSE_NTP_DEMO != 0 )
                /* if (xNTPTaskIsRunning() != pdFALSE) */
                {
                    /* Ask once more for the current time. */
                    /*   vStartNTPTask(0U, 0U); */
                }
            #endif

            /*vTaskDelete( NULL ); */
            xCommandIndex++;
        }

        // {
        //     char pcBuffer[ 1500 ];
        //     struct freertos_sockaddr xSourceAddress;
        //     socklen_t xLength = sizeof( socklen_t );
        //     int32_t rc = FreeRTOS_recvfrom( xSocket, pcBuffer, sizeof( pcBuffer ), 0, &xSourceAddress, &xLength );

        //     if( rc > 0 )
        //     {
        //         if( xSourceAddress.sin_family == FREERTOS_AF_INET6 )
        //         {
        //             FreeRTOS_printf( ( "Recv UDP %d bytes from %pip port %u\n", rc, xSourceAddress.sin_address.xIP_IPv6.ucBytes, FreeRTOS_ntohs( xSourceAddress.sin_port ) ) );
        //         }
        //         else
        //         {
        //             FreeRTOS_printf( ( "Recv UDP %d bytes from %xip port %u\n", rc, FreeRTOS_ntohl( xSourceAddress.sin_address.ulIP_IPv4 ), FreeRTOS_ntohs( xSourceAddress.sin_port ) ) );
        //         }

        //         if( rc == 14 )
        //         {
        //             static BaseType_t xDone = 0;

        //             if( xDone == 3 )
        //             {
        //                 BaseType_t xIPv6 = ( xSourceAddress.sin_family == FREERTOS_AF_INET6 ) ? pdTRUE : pdFALSE;
        //                 FreeRTOS_printf( ( "%d: Clear %s table\n", xDone, xIPv6 ? "ND" : "ARP" ) );

        //                 if( xIPv6 == pdTRUE )
        //                 {
        //                     FreeRTOS_ClearND();
        //                 }
        //                 else
        //                 {
        //                     FreeRTOS_ClearARP( NULL );
        //                 }

        //                 xDone = 0;
        //             }
        //             else
        //             {
        //                 xDone++;
        //             }
        //         }
        //     }
        // }
    }
}

