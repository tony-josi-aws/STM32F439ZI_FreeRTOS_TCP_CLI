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

/* Demo definitions. */
#define mainCLI_TASK_STACK_SIZE             512
#define mainCLI_TASK_PRIORITY               (tskIDLE_PRIORITY)

#define mainUSER_COMMAND_TASK_STACK_SIZE    2048
#define mainUSER_COMMAND_TASK_PRIORITY      (tskIDLE_PRIORITY)
/*-----------------------------------------------------------*/
/*-------------  ***  DEMO DEFINES   ***   ------------------*/
/*-----------------------------------------------------------*/

#define USE_IPv6_END_POINTS                 0

#define USE_UDP			 		     		1

#define USE_TCP			 		     		1

#if ( BUILD_IPERF3 == 1 )
    #define USE_IPERF3                          0
#endif

#define USE_ZERO_COPY 						1

#define USE_TCP_ZERO_COPY 		     		1

#define USE_USER_COMMAND_TASK               0

#define USE_TCP_ECHO_CLIENT                 0

#define USE_UDP_ECHO_SERVER                 0

#define USE_TCP_ECHO_SERVER                 0

#define USE_CORE_HTTP_DEMO					0

#if ( ipconfigUSE_IPv6 != 0 && USE_IPv6_END_POINTS != 0 && ipconfigUSE_IPv4 != 0 )
    #define TOTAL_ENDPOINTS                 3
#elif ( ipconfigUSE_IPv4 == 0 && ipconfigUSE_IPv6 != 0 )
    #define TOTAL_ENDPOINTS                 2
#else
    #define TOTAL_ENDPOINTS                 1
#endif /* ( ipconfigUSE_IPv6 != 0 && USE_IPv6_END_POINTS != 0 ) */

#define UDP_ECHO_PORT                       5005
static void prvSimpleServerTask( void *pvParameters );

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/* Logging module configuration. */
#define mainLOGGING_TASK_STACK_SIZE         256
#define mainLOGGING_TASK_PRIORITY           (tskIDLE_PRIORITY + 1)
#define mainLOGGING_QUEUE_LENGTH            100

#define mainMAX_UDP_RESPONSE_SIZE           1024
/*-----------------------------------------------------------*/

#include "pack_struct_start.h"
struct xPacketHeader
{
    uint8_t ucStartMarker;
    uint8_t ucPacketNumber;
    uint16_t usPayloadLength;
    uint8_t ucRequestId[4];
}
#include "pack_struct_end.h"
typedef struct xPacketHeader PacketHeader_t;

#define PACKET_HEADER_LENGTH    sizeof( PacketHeader_t )
#define PACKET_START_MARKER     0x55
/*-----------------------------------------------------------*/

uint32_t ulTim7Tick = 0;

extern UART_HandleTypeDef huart3;

BaseType_t xEndPointCount = 0;
BaseType_t xUpEndPointCount = 0;

const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

static BaseType_t xTasksAlreadyCreated = pdFALSE;

extern RNG_HandleTypeDef hrng;

static char cInputCommandString_UDP[ configMAX_COMMAND_INPUT_SIZE + 1 ];
static char cInputCommandString_TCP[ configMAX_COMMAND_INPUT_SIZE + 1 ];

TaskHandle_t network_up_task_handle;
BaseType_t network_up_task_create_ret_status, network_up;

#if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )

	NetworkInterface_t * pxSTM32Fxx_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                         NetworkInterface_t * pxInterface );

#endif

/*-----------------------------------------------------------*/



#if USE_TCP

    #define TCP_ECHO_RECV_DATA 0

    static uint8_t ucTcpResponseBuffer[ mainMAX_UDP_RESPONSE_SIZE + PACKET_HEADER_LENGTH ];

    static void prvCliTask_TCP( void *pvParameters );
    static BaseType_t prvSendResponseEndMarker_TCP( Socket_t xCLIServerSocket,
                                                struct freertos_sockaddr * pxSourceAddress,
                                                socklen_t xSourceAddressLength,
                                                uint8_t *pucPacketNumber,
                                                uint8_t *pucRequestId );
    static BaseType_t prvSendCommandResponse_TCP( Socket_t xCLIServerSocket,
                                            struct freertos_sockaddr * pxSourceAddress,
                                            socklen_t xSourceAddressLength,
                                            uint8_t *pucPacketNumber,
                                            uint8_t *pucRequestId,
                                            const uint8_t * pucResponse,
                                            uint32_t ulResponseLength );
    static void TCP_Server_Task(void *pvParams);

    #if TCP_ECHO_RECV_DATA 

        static BaseType_t prvSendResponseBytes_TCP( Socket_t xCLIServerSocket,
                                                const uint8_t * pucResponse,
                                                uint32_t ulResponseLength );
    #endif /* TCP_ECHO_RECV_DATA */

#endif /* USE_TCP */


#if USE_UDP

    #if USE_TCP
        #define configCLI_SERVER_PORT_UDP 1235
    #else
        #define configCLI_SERVER_PORT_UDP configCLI_SERVER_PORT
    #endif /* USE_TCP */

    static uint8_t ucUdpResponseBuffer[ mainMAX_UDP_RESPONSE_SIZE + PACKET_HEADER_LENGTH ];

    static void prvCliTask( void * pvParameters );

    static BaseType_t prvSendResponseEndMarker( Socket_t xCLIServerSocket,
                                                struct freertos_sockaddr * pxSourceAddress,
                                                socklen_t xSourceAddressLength,
                                                uint8_t *pucPacketNumber,
                                                uint8_t *pucRequestId );

    static BaseType_t prvSendCommandResponse( Socket_t xCLIServerSocket,
                                            struct freertos_sockaddr * pxSourceAddress,
                                            socklen_t xSourceAddressLength,
                                            uint8_t *pucPacketNumber,
                                            uint8_t *pucRequestId,
                                            const uint8_t * pucResponse,
                                            uint32_t ulResponseLength );

#endif /* USE_UDP */

static void prvConfigureMPU( void );

static void prvRegisterCLICommands( void );

static BaseType_t prvIsValidRequest( const uint8_t * pucPacket, uint32_t ulPacketLength, uint8_t * pucRequestId );

static void network_up_status_thread_fn(void *io_params);

uint32_t time_check = 0;

/*-----------------------------------------------------------*/

void app_main( void )
{
    BaseType_t xRet;
    const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
    const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
    const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
    const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

    prvConfigureMPU();

    /* Register all the commands with the FreeRTOS+CLI command
     * interpreter. */
    prvRegisterCLICommands();

    xRet = xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                                   mainLOGGING_TASK_PRIORITY,
                                   mainLOGGING_QUEUE_LENGTH );
    configASSERT( xRet == pdPASS );

    configPRINTF( ( "Calling FreeRTOS_IPInit...\n" ) );
    //FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );
	#if ( ipconfigMULTI_INTERFACE == 1 ) && ( ipconfigCOMPATIBLE_WITH_SINGLE == 0 )
    	static NetworkInterface_t xInterfaces[1];
    	static NetworkEndPoint_t xEndPoints[4];
	#endif
    //FreeRTOS_debug_printf((“FreeRTOS_IPInit\r\n”));
	memcpy(ipLOCAL_MAC_ADDRESS, ucMACAddress, sizeof ucMACAddress);

	/* Initialize the network interface.*/
    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
        /* Initialize the interface descriptor for WinPCap. */
        pxSTM32Fxx_FillInterfaceDescriptor(0, &(xInterfaces[0]));

        /* === End-point 0 === */
        #if ( ipconfigUSE_IPv4 != 0 )
            {
                FreeRTOS_FillEndPoint(&(xInterfaces[0]), &(xEndPoints[xEndPointCount]), ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
                #if ( ipconfigUSE_DHCP != 0 )
                {
                    /* End-point 0 wants to use DHCPv4. */
                    xEndPoints[xEndPointCount].bits.bWantDHCP = pdFALSE; // pdFALSE; // pdTRUE;
                }
                #endif /* ( ipconfigUSE_DHCP != 0 ) */

                xEndPointCount += 1;
            }
        #endif /* ( ipconfigUSE_IPv4 != 0 ) */

        /*
        *     End-point-1 : public
        *     Network: 2001:470:ed44::/64
        *     IPv6   : 2001:470:ed44::4514:89d5:4589:8b79/128
        *     Gateway: fe80::ba27:ebff:fe5a:d751  // obtained from Router Advertisement
        */
        #if ( ipconfigUSE_IPv6 != 0 && USE_IPv6_END_POINTS != 0 )
            {
                IPv6_Address_t xIPAddress;
                IPv6_Address_t xPrefix;
                IPv6_Address_t xGateWay;
                IPv6_Address_t xDNSServer1, xDNSServer2;

                FreeRTOS_inet_pton6( "2001:470:ed44::", xPrefix.ucBytes );

                FreeRTOS_CreateIPv6Address( &xIPAddress, &xPrefix, 64, pdTRUE );
                FreeRTOS_inet_pton6( "fe80::ba27:ebff:fe5a:d751", xGateWay.ucBytes );

                FreeRTOS_FillEndPoint_IPv6( &( xInterfaces[ 0 ] ),
                                            &( xEndPoints[ xEndPointCount ] ),
                                            &( xIPAddress ),
                                            &( xPrefix ),
                                            64uL, /* Prefix length. */
                                            &( xGateWay ),
                                            NULL, /* pxDNSServerAddress: Not used yet. */
                                            ucMACAddress );
                FreeRTOS_inet_pton6( "2001:4860:4860::8888", xEndPoints[ xEndPointCount ].ipv6_settings.xDNSServerAddresses[ 0 ].ucBytes );
                FreeRTOS_inet_pton6( "fe80::1", xEndPoints[ xEndPointCount ].ipv6_settings.xDNSServerAddresses[ 1 ].ucBytes );
                FreeRTOS_inet_pton6( "2001:4860:4860::8888", xEndPoints[ xEndPointCount ].ipv6_defaults.xDNSServerAddresses[ 0 ].ucBytes );
                FreeRTOS_inet_pton6( "fe80::1", xEndPoints[ xEndPointCount ].ipv6_defaults.xDNSServerAddresses[ 1 ].ucBytes );

                #if ( ipconfigUSE_RA != 0 )
                    {
                        /* End-point 1 wants to use Router Advertisement */
                        xEndPoints[ xEndPointCount ].bits.bWantRA = pdTRUE;
                    }
                #endif /* #if( ipconfigUSE_RA != 0 ) */
                #if ( ipconfigUSE_DHCPv6 != 0 )
                    {
                        /* End-point 1 wants to use DHCPv6. */
                        xEndPoints[ xEndPointCount ].bits.bWantDHCP = pdTRUE;
                    }
                #endif /* ( ipconfigUSE_DHCPv6 != 0 ) */

                xEndPointCount += 1;

            }

            /*
            *     End-point-3 : private
            *     Network: fe80::/10 (link-local)
            *     IPv6   : fe80::d80e:95cc:3154:b76a/128
            *     Gateway: -
            */
            {
                IPv6_Address_t xIPAddress;
                IPv6_Address_t xPrefix;

                FreeRTOS_inet_pton6( "fe80::", xPrefix.ucBytes );
                FreeRTOS_inet_pton6( "fe80::7009", xIPAddress.ucBytes );

                FreeRTOS_FillEndPoint_IPv6(
                    &( xInterfaces[ 0 ] ),
                    &( xEndPoints[ xEndPointCount ] ),
                    &( xIPAddress ),
                    &( xPrefix ),
                    10U,  /* Prefix length. */
                    NULL, /* No gateway */
                    NULL, /* pxDNSServerAddress: Not used yet. */
                    ucMACAddress );

                xEndPointCount += 1;
            }

        #endif /* ( ipconfigUSE_IPv6 != 0 && USE_IPv6_END_POINTS != 0 ) */

        FreeRTOS_IPInit_Multi();
    #else
        /* Using the old /single /IPv4 library, or using backward compatible mode of the new /multi library. */
        FreeRTOS_debug_printf(("FreeRTOS_IPInit\r\n"));
        FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
    #endif /* defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 ) */

    /* Task to display network status */
    network_up_task_create_ret_status = xTaskCreate(network_up_status_thread_fn, "network_up_status", 200, "HW from 2", ipconfigIP_TASK_PRIORITY - 1, &network_up_task_handle);
    configASSERT(network_up_task_create_ret_status == pdPASS);

    /* Start the RTOS scheduler. */
    vTaskStartScheduler();

    /* Infinite loop */
    for(;;)
    {
        
    }
}
/*-----------------------------------------------------------*/

#if USE_UDP

    static void prvCliTask( void *pvParameters )
    {
        uint32_t ulResponseLength;
        BaseType_t xCount, xResponseRemaining, xResponseSent;
        Socket_t xCLIServerSocket = FREERTOS_INVALID_SOCKET;
        struct freertos_sockaddr xSourceAddress, xServerAddress;
        socklen_t xSourceAddressLength = sizeof( xSourceAddress );
        TickType_t xCLIServerRecvTimeout = portMAX_DELAY;
        char * pcOutputBuffer = FreeRTOS_CLIGetOutputBuffer();

        ( void ) pvParameters;

        xCLIServerSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                            FREERTOS_SOCK_DGRAM,
                                            FREERTOS_IPPROTO_UDP );
        configASSERT( xCLIServerSocket != FREERTOS_INVALID_SOCKET );

        /* No need to return from FreeRTOS_recvfrom until a message
        * is received. */
        FreeRTOS_setsockopt( xCLIServerSocket,
                            0,
                            FREERTOS_SO_RCVTIMEO,
                            &( xCLIServerRecvTimeout ),
                            sizeof( TickType_t ) );

        xServerAddress.sin_port = FreeRTOS_htons( configCLI_SERVER_PORT_UDP );

        #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
            xServerAddress.sin_address.ulIP_IPv4 = FreeRTOS_GetIPAddress();
        #else
            xServerAddress.sin_addr = FreeRTOS_GetIPAddress();
        #endif

        xServerAddress.sin_family = FREERTOS_AF_INET;

        FreeRTOS_bind( xCLIServerSocket, &( xServerAddress ), sizeof( xServerAddress ) );

        configPRINTF( ( "Waiting for requests...\n" ) );

        for( ;; )
        {
            uint8_t ucRequestId[ 4 ];

    #if USE_ZERO_COPY

                uint8_t *pucReceivedUDPPayload = NULL;
                xCount = FreeRTOS_recvfrom( xCLIServerSocket,
                                            &pucReceivedUDPPayload,
                                            0,
                                            FREERTOS_ZERO_COPY,
                                            &( xSourceAddress ),
                                            &( xSourceAddressLength ) );

                configASSERT( (pucReceivedUDPPayload != NULL) && (xCount < configMAX_COMMAND_INPUT_SIZE));

                memcpy(( void * )( &( cInputCommandString_UDP[ 0 ] ) ), pucReceivedUDPPayload, xCount);

                FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucReceivedUDPPayload );

    #else

                xCount = FreeRTOS_recvfrom( xCLIServerSocket,
                                            ( void * )( &( cInputCommandString_UDP[ 0 ] ) ),
                                            configMAX_COMMAND_INPUT_SIZE,
                                            0,
                                            &( xSourceAddress ),
                                            &( xSourceAddressLength ) );

    #endif /* USE_ZERO_COPY */

            /* Since we set the receive timeout to portMAX_DELAY, the
            * above call should only return when a command is received. */
            configASSERT( xCount > 0 );
            cInputCommandString_UDP[ xCount ] = '\0';

            if( prvIsValidRequest( ( const uint8_t * ) &( cInputCommandString_UDP[ 0 ] ), xCount, &( ucRequestId[ 0 ] ) ) == pdTRUE )
            {
                uint8_t ucPacketNumber = 1;
    #ifdef TIM7_TEST
                extern uint32_t tim7_period;

                #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                    configPRINTF( ( "Received command. IP:%x Port:%u Content:%s TIM7 period ms: %u \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                    xSourceAddress.sin_port,
                                                                                    &( cInputCommandString_UDP[ PACKET_HEADER_LENGTH ] ), tim7_period ) );
                #else
                    configPRINTF( ( "Received command. IP:%x Port:%u Content:%s TIM7 period ms: %u \n", xSourceAddress.sin_addr,
                                                                                    xSourceAddress.sin_port,
                                                                                    &( cInputCommandString_UDP[ PACKET_HEADER_LENGTH ] ), tim7_period ) );
                #endif

    #else

                #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                    configPRINTF( ( "Received command. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                    xSourceAddress.sin_port,
                                                                                    &( cInputCommandString_UDP[ PACKET_HEADER_LENGTH ] ) ) );
                #else
                    configPRINTF( ( "Received command. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_addr,
                                                                                    xSourceAddress.sin_port,
                                                                                    &( cInputCommandString_UDP[ PACKET_HEADER_LENGTH ] ) ) );
                #endif

    #endif
                do
                {
                    /* Send the received command to the FreeRTOS+CLI. */
                    xResponseRemaining = FreeRTOS_CLIProcessCommand( &( cInputCommandString_UDP[ PACKET_HEADER_LENGTH ] ),
                                                                        pcOutputBuffer,
                                                                        configCOMMAND_INT_MAX_OUTPUT_SIZE - 1 );

                    /* Ensure null termination so that the strlen below does not
                    * end up reading past bounds. */
                    pcOutputBuffer[ configCOMMAND_INT_MAX_OUTPUT_SIZE - 1 ] = '\0';

                    ulResponseLength = strlen( pcOutputBuffer );

                    /* HACK - Check if the output buffer contains one of our special
                    * markers indicating the need of a special response and process
                    * accordingly. */
                    if( strncmp( pcOutputBuffer, "PCAP-GET", ulResponseLength ) == 0 )
                    {
                        const uint8_t * pucPcapData;
                        uint32_t ulPcapDataLength;

                        pcap_capture_get_captured_data( &( pucPcapData ),
                                                        &( ulPcapDataLength) );

                        xResponseSent = prvSendCommandResponse( xCLIServerSocket,
                                                                &( xSourceAddress ),
                                                                xSourceAddressLength,
                                                                &( ucPacketNumber ),
                                                                &( ucRequestId [ 0 ] ),
                                                                pucPcapData,
                                                                ulPcapDataLength );

                        /* Next fetch should not get the same capture but the capture
                        * after this point. */
                        pcap_capture_reset();
                    }
                    else
                    {
                        /* Send the command response. */
                        xResponseSent = prvSendCommandResponse( xCLIServerSocket,
                                                                &( xSourceAddress ),
                                                                xSourceAddressLength,
                                                                &( ucPacketNumber ),
                                                                &( ucRequestId [ 0 ] ),
                                                                ( const uint8_t * ) pcOutputBuffer,
                                                                ulResponseLength );
                    }

                    if( xResponseSent == pdPASS )
                    {
                        configPRINTF( ( "Response sent successfully. \n" ) );
                    }
                    else
                    {
                        configPRINTF( ( "[ERROR] Failed to send response. \n" ) );
                    }
                } while( xResponseRemaining == pdTRUE );

                /* Send the last packet with zero payload length. */
                ( void ) prvSendResponseEndMarker( xCLIServerSocket,
                                                &( xSourceAddress ),
                                                xSourceAddressLength,
                                                &( ucPacketNumber ),
                                                &( ucRequestId [ 0 ] ) );
            }
            else
            {

                #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                    configPRINTF( ( "[ERROR] Malformed request. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                            xSourceAddress.sin_port,
                                                                                            cInputCommandString_UDP ) );
                #else
                    configPRINTF( ( "[ERROR] Malformed request. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_addr,
                                                                                            xSourceAddress.sin_port,
                                                                                            cInputCommandString_UDP ) );
                #endif
                

            }
        }
    }

#endif /* USE_UDP */

#if USE_TCP

    static void prvCliTask_TCP( void *pvParameters )
    {
        struct freertos_sockaddr xClient;
        socklen_t xSize = sizeof( xClient );
        Socket_t xListeningSocket = FREERTOS_INVALID_SOCKET, xConnectedSocket = FREERTOS_INVALID_SOCKET;
        struct freertos_sockaddr xServerAddress;
        TickType_t xCLIServerRecvTimeout = portMAX_DELAY;
        const BaseType_t xBacklog = 20;

        ( void ) pvParameters;

        xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                            FREERTOS_SOCK_STREAM,
                                            FREERTOS_IPPROTO_TCP );
        configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

        /* No need to return from FreeRTOS_recvfrom until a message
        * is received. */
        FreeRTOS_setsockopt( xListeningSocket,
                            0,
                            FREERTOS_SO_RCVTIMEO,
                            &( xCLIServerRecvTimeout ),
                            sizeof( TickType_t ) );

        xServerAddress.sin_port = FreeRTOS_htons( configCLI_SERVER_PORT );

        #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
            xServerAddress.sin_address.ulIP_IPv4 = FreeRTOS_GetIPAddress();
        #else
            xServerAddress.sin_addr = FreeRTOS_GetIPAddress();
        #endif

        xServerAddress.sin_family = FREERTOS_AF_INET;

        FreeRTOS_bind( xListeningSocket, &( xServerAddress ), sizeof( xServerAddress ) );

        /* start listening */
        FreeRTOS_listen( xListeningSocket, xBacklog );

        for( ;; )
        {
            /* Wait for a client to connect. */
            xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
            configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );

            /* Spawn a task to handle the connection. */
            xTaskCreate( TCP_Server_Task, "CLI TCP Server Instance", mainCLI_TASK_STACK_SIZE, ( void * ) xConnectedSocket, mainCLI_TASK_PRIORITY, NULL );
        }

    }

    static void TCP_Server_Task(void *pvParams)
    {

        uint32_t ulResponseLength;
        BaseType_t xCount, xResponseRemaining, xResponseSent;
        Socket_t xCLIServerSocket = (Socket_t) pvParams;
        struct freertos_sockaddr xSourceAddress;
        socklen_t xSourceAddressLength = sizeof( xSourceAddress );
        char * pcOutputBuffer = FreeRTOS_CLIGetOutputBuffer();
        static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 5000 );
        static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 5000 );
        uint8_t *pucRxBuffer = NULL;

        /* Attempt to create the buffer used to receive the string to be echoed
        back.  This could be avoided using a zero copy interface that just returned
        the same buffer. */
        pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );
        configASSERT(pucRxBuffer != NULL);

        FreeRTOS_setsockopt( xCLIServerSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
        FreeRTOS_setsockopt( xCLIServerSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );
        
        for( ;; )
        {

            /* Zero out the receive array so there is NULL at the end of the string
            when it is printed out. */
            memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

            uint8_t ucRequestId[ 4 ];

    #if USE_TCP_ZERO_COPY

                uint8_t *pucZeroCopyRxBuffPtr = NULL;
                xCount = FreeRTOS_recv( xCLIServerSocket, &pucZeroCopyRxBuffPtr, ipconfigTCP_MSS, FREERTOS_ZERO_COPY );
                if( pucZeroCopyRxBuffPtr != NULL )
                {
                    memcpy( pucRxBuffer, pucZeroCopyRxBuffPtr, xCount );
                    FreeRTOS_ReleaseTCPPayloadBuffer( xCLIServerSocket, pucZeroCopyRxBuffPtr, xCount );
                }
                else
                {
                    xCount = -1;
                }

    #else

                xCount = FreeRTOS_recv( xCLIServerSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );

    #endif /* USE_TCP_ZERO_COPY */

            /* Since we set the receive timeout to portMAX_DELAY, the
            * above call should only return when a command is received. */
            //configASSERT( xCount > 0 );

            if( xCount > 0 )
            {
                configASSERT( xCount < configMAX_COMMAND_INPUT_SIZE);

                memcpy(cInputCommandString_TCP, pucRxBuffer, xCount);
                cInputCommandString_TCP[ xCount ] = '\0';

                /* Echo the response back */
                /* prvSendResponseBytes_TCP(xCLIServerSocket, cInputCommandString_TCP, xCount + 1); */

                if( prvIsValidRequest( ( const uint8_t * ) &( cInputCommandString_TCP[ 0 ] ), xCount, &( ucRequestId[ 0 ] ) ) == pdTRUE )
                {
                    uint8_t ucPacketNumber = 1;
    #ifdef TIM7_TEST
                    extern uint32_t tim7_period;

                    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                        configPRINTF( ( "Received command. IP:%x Port:%u Content:%s TIM7 period ms: %u \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                        xSourceAddress.sin_port,
                                                                                        &( cInputCommandString_TCP[ PACKET_HEADER_LENGTH ] ), tim7_period ) );
                    #else
                        configPRINTF( ( "Received command. IP:%x Port:%u Content:%s TIM7 period ms: %u \n", xSourceAddress.sin_addr,
                                                                                        xSourceAddress.sin_port,
                                                                                        &( cInputCommandString_TCP[ PACKET_HEADER_LENGTH ] ), tim7_period ) );
                    #endif

    #else

                    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                        configPRINTF( ( "Received command. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                        xSourceAddress.sin_port,
                                                                                        &( cInputCommandString_TCP[ PACKET_HEADER_LENGTH ] ) ) );
                    #else
                        configPRINTF( ( "Received command. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_addr,
                                                                                        xSourceAddress.sin_port,
                                                                                        &( cInputCommandString_TCP[ PACKET_HEADER_LENGTH ] ) ) );
                    #endif

    #endif
                    do
                    {
                        /* Send the received command to the FreeRTOS+CLI. */
                        xResponseRemaining = FreeRTOS_CLIProcessCommand( &( cInputCommandString_TCP[ PACKET_HEADER_LENGTH ] ),
                                                                            pcOutputBuffer,
                                                                            configCOMMAND_INT_MAX_OUTPUT_SIZE - 1 );

                        /* Ensure null termination so that the strlen below does not
                        * end up reading past bounds. */
                        pcOutputBuffer[ configCOMMAND_INT_MAX_OUTPUT_SIZE - 1 ] = '\0';

                        ulResponseLength = strlen( pcOutputBuffer );

                        /* HACK - Check if the output buffer contains one of our special
                        * markers indicating the need of a special response and process
                        * accordingly. */
                        if( strncmp( pcOutputBuffer, "PCAP-GET", ulResponseLength ) == 0 )
                        {
                            const uint8_t * pucPcapData;
                            uint32_t ulPcapDataLength;

                            pcap_capture_get_captured_data( &( pucPcapData ),
                                                            &( ulPcapDataLength) );

                            xResponseSent = prvSendCommandResponse_TCP( xCLIServerSocket,
                                                                    &( xSourceAddress ),
                                                                    xSourceAddressLength,
                                                                    &( ucPacketNumber ),
                                                                    &( ucRequestId [ 0 ] ),
                                                                    pucPcapData,
                                                                    ulPcapDataLength );

                            /* Next fetch should not get the same capture but the capture
                            * after this point. */
                            pcap_capture_reset();
                        }
                        else
                        {
                            /* Send the command response. */
                            xResponseSent = prvSendCommandResponse_TCP( xCLIServerSocket,
                                                                    &( xSourceAddress ),
                                                                    xSourceAddressLength,
                                                                    &( ucPacketNumber ),
                                                                    &( ucRequestId [ 0 ] ),
                                                                    ( const uint8_t * ) pcOutputBuffer,
                                                                    ulResponseLength );
                        }

                        if( xResponseSent == pdPASS )
                        {
                            configPRINTF( ( "Response sent successfully. \n" ) );
                        }
                        else
                        {
                            configPRINTF( ( "[ERROR] Failed to send response. \n" ) );
                        }
                    } while( xResponseRemaining == pdTRUE );

                    /* Send the last packet with zero payload length. */
                    ( void ) prvSendResponseEndMarker_TCP( xCLIServerSocket,
                                                    &( xSourceAddress ),
                                                    xSourceAddressLength,
                                                    &( ucPacketNumber ),
                                                    &( ucRequestId [ 0 ] ) );
                }
                else
                {

                    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                        configPRINTF( ( "[ERROR] Malformed request. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_address.ulIP_IPv4,
                                                                                                xSourceAddress.sin_port,
                                                                                                cInputCommandString_TCP ) );
                    #else
                        configPRINTF( ( "[ERROR] Malformed request. IP:%x Port:%u Content:%s \n", xSourceAddress.sin_addr,
                                                                                                xSourceAddress.sin_port,
                                                                                                cInputCommandString_TCP ) );
                    #endif
                    

                }
            }
        }
    }

#endif
/*-----------------------------------------------------------*/

static void prvRegisterCLICommands( void )
{
extern void vRegisterPingCommand( void );
extern void vRegisterPcapCommand( void );
extern void vRegisterNetStatCommand( void );
extern void vRegisterTopCommand( void );

    vRegisterPingCommand();
    vRegisterPcapCommand();
    vRegisterNetStatCommand();
    vRegisterTopCommand();
}
/*-----------------------------------------------------------*/

static void prvConfigureMPU( void )
{
    MPU_Region_InitTypeDef MPU_InitStruct;

    HAL_MPU_Disable();

    /*  Configure the MPU attributes of RAM_D2 ( which contains .ethernet_data )
     * as Write back, Read allocate, Write allocate. */
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress      = 0x30000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_256KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion( &( MPU_InitStruct ) );

    HAL_MPU_Enable( MPU_PRIVILEGED_DEFAULT );
}
/*-----------------------------------------------------------*/

static BaseType_t prvIsValidRequest( const uint8_t * pucPacket, uint32_t ulPacketLength, uint8_t * pucRequestId )
{
    BaseType_t xValidRequest = pdFALSE;
    PacketHeader_t * header;
    uint16_t usPayloadLength;

    if( ulPacketLength > PACKET_HEADER_LENGTH )
    {
        header = ( PacketHeader_t * )pucPacket;
        usPayloadLength = FreeRTOS_ntohs( header->usPayloadLength );

        if( ( header->ucStartMarker == PACKET_START_MARKER ) &&
            ( header->ucPacketNumber == 1 ) &&
            ( ( usPayloadLength + PACKET_HEADER_LENGTH ) == ulPacketLength ) )
        {
            xValidRequest = pdTRUE;
            memcpy( pucRequestId, &( header->ucRequestId[ 0 ] ), 4 );
        }
    }

    return xValidRequest;
}
/*-----------------------------------------------------------*/

#if USE_TCP

    static BaseType_t prvSendResponseEndMarker_TCP( Socket_t xCLIServerSocket,
                                                struct freertos_sockaddr * pxSourceAddress,
                                                socklen_t xSourceAddressLength,
                                                uint8_t *pucPacketNumber,
                                                uint8_t *pucRequestId )
    {
        BaseType_t ret = pdPASS;
        int32_t lBytes, lSent, lTotalSent;
        PacketHeader_t header;

        header.ucStartMarker = PACKET_START_MARKER;
        header.ucPacketNumber = *pucPacketNumber;
        header.usPayloadLength = 0U;
        memcpy( &( header.ucRequestId[ 0 ] ), pucRequestId, 4 );

    #if USE_TCP_ZERO_COPY

            /* Send response. */

            lBytes = sizeof( PacketHeader_t );
            lSent = 0;
            lTotalSent = 0;

            /* Call send() until all the data has been sent. */
            while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
            {
                BaseType_t xAvlSpace = 0;
                BaseType_t xBytesToSend = 0;
                uint8_t *pucTCPZeroCopyStrmBuffer = FreeRTOS_get_tx_head( xCLIServerSocket, &xAvlSpace );

                if(pucTCPZeroCopyStrmBuffer)
                {
                    if((lBytes - lTotalSent) > xAvlSpace)
                    {
                        xBytesToSend = xAvlSpace;
                    }
                    else
                    {
                        xBytesToSend = (lBytes - lTotalSent);
                    }
                    memcpy(pucTCPZeroCopyStrmBuffer, ( void * ) (( (uint8_t *) &header ) + lTotalSent),  xBytesToSend);
                }
                else
                {
                    ret = pdFAIL;
                    break;
                }
                
                lSent = FreeRTOS_send( xCLIServerSocket, NULL, xBytesToSend, 0 );
                lTotalSent += lSent;
            }

            if( lSent < 0 )
            {
                /* Socket closed? */
                ret = pdFAIL;
            }

    #else

            /* Send response. */

            lBytes = sizeof( PacketHeader_t );
            lSent = 0;
            lTotalSent = 0;

            /* Call send() until all the data has been sent. */
            while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
            {
                lSent = FreeRTOS_send( xCLIServerSocket, ( void * ) (( (uint8_t *) &header ) + lTotalSent), lBytes - lTotalSent, 0 );
                lTotalSent += lSent;
            }

            if( lSent < 0 )
            {
                /* Socket closed? */
                ret = pdFAIL;
            }

    #endif /* USE_TCP_ZERO_COPY */

        if( lTotalSent != PACKET_HEADER_LENGTH )
        {
            configPRINTF( ("[ERROR] Failed to last response header.\n" ) );
            ret = pdFAIL;
        }

        return ret;
    }

#endif /* USE_TCP */

/*-----------------------------------------------------------*/

#if USE_UDP

    static BaseType_t prvSendResponseEndMarker( Socket_t xCLIServerSocket,
                                                struct freertos_sockaddr * pxSourceAddress,
                                                socklen_t xSourceAddressLength,
                                                uint8_t *pucPacketNumber,
                                                uint8_t *pucRequestId )
    {
        BaseType_t ret;
        PacketHeader_t header;
        int32_t lBytesSent;

        header.ucStartMarker = PACKET_START_MARKER;
        header.ucPacketNumber = *pucPacketNumber;
        header.usPayloadLength = 0U;
        memcpy( &( header.ucRequestId[ 0 ] ), pucRequestId, 4 );

    #if USE_ZERO_COPY
            /*
            * First obtain a buffer of adequate length from the TCP/IP stack into which
            the string will be written. */
    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
		#if 0
        	/*Test to validate if adding 2 internally to requested size cause an overflow in pxGetNetworkBufferWithDescriptor */
        	uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( SIZE_MAX - sizeof( UDPPacket_t ) - 1, portMAX_DELAY, ipTYPE_IPv4 );
        	configASSERT(pucBuffer == NULL);

        	/*Test to validate if adding 2 internally to requested size cause an overflow in pxGetNetworkBufferWithDescriptor */
        	pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( SIZE_MAX - sizeof( UDPPacket_t ) - 2, portMAX_DELAY, ipTYPE_IPv4 );
        	configASSERT(pucBuffer == NULL);

        	/*Test to validate if adding 2 internally to requested size cause an overflow in pxGetNetworkBufferWithDescriptor */
        	pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( SIZE_MAX - sizeof( UDPPacket_t ) - 7, portMAX_DELAY, ipTYPE_IPv4 );
        	configASSERT(pucBuffer == NULL);

        	pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( sizeof( PacketHeader_t ), portMAX_DELAY, ipTYPE_IPv4 );
        	uint8_t *pucBuffer2 = pxResizeNetworkBufferWithDescriptor(pucBuffer, SIZE_MAX - ipBUFFER_PADDING + 1);
        	configASSERT(pucBuffer2 == NULL);
        	//FreeRTOS_ReleaseUDPPayloadBuffer(pucBuffer);
		#endif
        	uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( sizeof( PacketHeader_t ), portMAX_DELAY, ipTYPE_IPv4 );
    #else
            uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer( sizeof( PacketHeader_t ), portMAX_DELAY );
    #endif
            configASSERT( pucBuffer != NULL );
            memcpy( pucBuffer , &header, sizeof( PacketHeader_t ) );


            time_check = ARM_REG_DWT_CYCCNT;
            lBytesSent = FreeRTOS_sendto( xCLIServerSocket,
                                        ( void * ) ( pucBuffer ),
                                        sizeof( PacketHeader_t ),
                                        FREERTOS_ZERO_COPY,
                                        pxSourceAddress,
                                        xSourceAddressLength );
            time_check = ARM_REG_DWT_CYCCNT - time_check;


    #else

            lBytesSent = FreeRTOS_sendto( xCLIServerSocket,
                                        ( void * ) ( &header ),
                                        sizeof( PacketHeader_t ),
                                        0,
                                        pxSourceAddress,
                                        xSourceAddressLength );

    #endif /* USE_ZERO_COPY */

        if( lBytesSent != PACKET_HEADER_LENGTH )
        {
            configPRINTF( ("[ERROR] Failed to last response header.\n" ) );
            ret = pdFAIL;
        }

        return ret;
    }

    /*-----------------------------------------------------------*/

    static BaseType_t prvSendCommandResponse( Socket_t xCLIServerSocket,
                                            struct freertos_sockaddr * pxSourceAddress,
                                            socklen_t xSourceAddressLength,
                                            uint8_t *pucPacketNumber,
                                            uint8_t *pucRequestId,
                                            const uint8_t * pucResponse,
                                            uint32_t ulResponseLength )
    {
        BaseType_t ret = pdPASS;
        PacketHeader_t header;
        int32_t lBytesSent;
        uint32_t ulBytesToSend, ulRemainingBytes, ulBytesSent;

        ulRemainingBytes = ulResponseLength;
        ulBytesSent = 0;

        while( ulRemainingBytes > 0 )
        {
            ulBytesToSend = ulRemainingBytes;

            if( ulBytesToSend > mainMAX_UDP_RESPONSE_SIZE )
            {
                ulBytesToSend = mainMAX_UDP_RESPONSE_SIZE;
            }

            /* Write header to the response buffer. */
            header.ucStartMarker = PACKET_START_MARKER;
            header.ucPacketNumber = *pucPacketNumber;
            *pucPacketNumber = ( *pucPacketNumber ) + 1;
            header.usPayloadLength = FreeRTOS_htons( ( uint16_t ) ulBytesToSend );
            memcpy( &( header.ucRequestId[ 0 ] ), pucRequestId, 4 );

            memcpy( &( ucUdpResponseBuffer[ 0 ] ),
                    &( header ),
                    PACKET_HEADER_LENGTH );

            /* Write actual response to the buffer. */
            memcpy( &( ucUdpResponseBuffer[ PACKET_HEADER_LENGTH] ),
                    &( pucResponse[ ulBytesSent ] ),
                    ulBytesToSend );



    #if USE_ZERO_COPY
                /*
                * First obtain a buffer of adequate length from the TCP/IP stack into which
                the string will be written. */
    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( ulBytesToSend + PACKET_HEADER_LENGTH, portMAX_DELAY, ipTYPE_IPv4 );
    #else
                uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer( ulBytesToSend + PACKET_HEADER_LENGTH, portMAX_DELAY );
    #endif
                configASSERT( pucBuffer != NULL );
                memcpy( pucBuffer , &( ucUdpResponseBuffer[ 0 ] ), ulBytesToSend + PACKET_HEADER_LENGTH );

                /* Send response. */
                lBytesSent = FreeRTOS_sendto( xCLIServerSocket,
                                            ( void * ) pucBuffer,
                                            ulBytesToSend + PACKET_HEADER_LENGTH,
                                            FREERTOS_ZERO_COPY,
                                            pxSourceAddress,
                                            xSourceAddressLength );
    #else

                /* Send response. */
                lBytesSent = FreeRTOS_sendto( xCLIServerSocket,
                                            ( void * ) &( ucUdpResponseBuffer[ 0 ] ),
                                            ulBytesToSend + PACKET_HEADER_LENGTH,
                                            0,
                                            pxSourceAddress,
                                            xSourceAddressLength );

    #endif /* USE_ZERO_COPY */

            if( lBytesSent != ( ulBytesToSend + PACKET_HEADER_LENGTH ) )
            {
                configPRINTF( ("[ERROR] Failed to send response.\n" ) );
                ret = pdFAIL;
                break;
            }

            ulRemainingBytes -= ulBytesToSend;
            ulBytesSent += ulBytesToSend;
        }

        return ret;
    }

#endif /* USE UDP */

#if USE_TCP

    static BaseType_t prvSendCommandResponse_TCP( Socket_t xCLIServerSocket,
                                            struct freertos_sockaddr * pxSourceAddress,
                                            socklen_t xSourceAddressLength,
                                            uint8_t *pucPacketNumber,
                                            uint8_t *pucRequestId,
                                            const uint8_t * pucResponse,
                                            uint32_t ulResponseLength )
    {
        BaseType_t ret = pdPASS;
        PacketHeader_t header;
        uint32_t ulBytesToSend, ulRemainingBytes, ulBytesSent;
        int32_t lBytes, lSent, lTotalSent;

        ulRemainingBytes = ulResponseLength;
        ulBytesSent = 0;

        while( ulRemainingBytes > 0 )
        {
            ulBytesToSend = ulRemainingBytes;


            /* Write header to the response buffer. */
            header.ucStartMarker = PACKET_START_MARKER;
            header.ucPacketNumber = *pucPacketNumber;
            *pucPacketNumber = ( *pucPacketNumber ) + 1;
            header.usPayloadLength = FreeRTOS_htons( ( uint16_t ) ulBytesToSend );
            memcpy( &( header.ucRequestId[ 0 ] ), pucRequestId, 4 );

            memcpy( &( ucTcpResponseBuffer[ 0 ] ),
                    &( header ),
                    PACKET_HEADER_LENGTH );

            /* Write actual response to the buffer. */
            memcpy( &( ucTcpResponseBuffer[ PACKET_HEADER_LENGTH] ),
                    &( pucResponse[ ulBytesSent ] ),
                    ulBytesToSend );



    #if USE_TCP_ZERO_COPY

                /* Send response. */

                lBytes = ulBytesToSend + PACKET_HEADER_LENGTH;
                lSent = 0;
                lTotalSent = 0;

                /* Call send() until all the data has been sent. */
                while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
                {

                    BaseType_t xAvlSpace = 0;
                    BaseType_t xBytesToSend = 0;
                    uint8_t *pucTCPZeroCopyStrmBuffer = FreeRTOS_get_tx_head( xCLIServerSocket, &xAvlSpace );

                    if(pucTCPZeroCopyStrmBuffer)
                    {
                        if((lBytes - lTotalSent) > xAvlSpace)
                        {
                            xBytesToSend = xAvlSpace;
                        }
                        else
                        {
                            xBytesToSend = (lBytes - lTotalSent);
                        }
                        memcpy(pucTCPZeroCopyStrmBuffer, ( void * ) (( (uint8_t *) ucTcpResponseBuffer ) + lTotalSent),  xBytesToSend);
                    }
                    else
                    {
                        ret = pdFAIL;
                        break;
                    }

                    lSent = FreeRTOS_send( xCLIServerSocket, NULL, xBytesToSend, 0 );
                    lTotalSent += lSent;
                }

                if( lSent < 0 )
                {
                    /* Socket closed? */
                    break;
                }

    #else

                /* Send response. */

                lBytes = ulBytesToSend + PACKET_HEADER_LENGTH;
                lSent = 0;
                lTotalSent = 0;

                /* Call send() until all the data has been sent. */
                while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
                {
                    lSent = FreeRTOS_send( xCLIServerSocket, ucTcpResponseBuffer + lTotalSent, lBytes - lTotalSent, 0 );
                    lTotalSent += lSent;
                }

                if( lSent < 0 )
                {
                    /* Socket closed? */
                    break;
                }

    #endif /* USE_TCP_ZERO_COPY */

            if( lTotalSent != ( ulBytesToSend + PACKET_HEADER_LENGTH ) )
            {
                configPRINTF( ("[ERROR] Failed to send response.\n" ) );
                ret = pdFAIL;
                break;
            }

            ulRemainingBytes -= (lTotalSent - PACKET_HEADER_LENGTH);
        }

        return ret;
    }

#if TCP_ECHO_RECV_DATA
    static BaseType_t prvSendResponseBytes_TCP( Socket_t xCLIServerSocket,
                                            const uint8_t * pucResponse,
                                            uint32_t ulResponseLength )
    {
        BaseType_t ret = pdPASS;
        int32_t lBytes, lSent, lTotalSent;

        lBytes = ulResponseLength;
        lSent = 0;
        lTotalSent = 0;

        /* Call send() until all the data has been sent. */
        while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
        {
            lSent = FreeRTOS_send( xCLIServerSocket, pucResponse, lBytes - lTotalSent, 0 );
            lTotalSent += lSent;
        }

        if( lSent < 0 )
        {
            /* Socket closed? */
            ret = pdFAIL;
        }
        
        return ret;

    }

#endif /* TCP_ECHO_RECV_DATA */

#endif

/*-----------------------------------------------------------*/

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                             uint16_t usSourcePort,
                                             uint32_t ulDestinationAddress,
                                             uint16_t usDestinationPort )
{
    uint32_t ulReturn;

    ( void ) ulSourceAddress;
    ( void ) usSourcePort;
    ( void ) ulDestinationAddress;
    ( void ) usDestinationPort;

    xApplicationGetRandomNumber( &ulReturn );

    return ulReturn;
}
/*-----------------------------------------------------------*/

static void network_up_status_thread_fn(void *io_params) {
	while(1) {
		if (network_up == 0)
		{
			ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		}

		for (int i = 0; i < 4; ++i)
		{
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
			vTaskDelay(50);
		}
		vTaskDelay(200);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
		vTaskDelay(20);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
	}
}

#if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
	void vApplicationIPNetworkEventHook_Multi( eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint* pxEndPoint )
#else
	void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent)
#endif /* defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 ) */
{
    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
        char cBuffer[ 16 ];

        xUpEndPointCount += 1;

        if( xUpEndPointCount >= TOTAL_ENDPOINTS )
        {

            /* Create the tasks that use the IP stack if they have not already been
            * created. */
            if( xTasksAlreadyCreated == pdFALSE )
            {
                xTasksAlreadyCreated = pdTRUE;

                #if ( ipconfigUSE_IPv4 != 0 )

                    #if USE_UDP
                    
                        /* Sockets, and tasks that use the TCP/IP stack can be created here. */
                        xTaskCreate( prvCliTask,
                                    "CLI_UDP_Server",
                                    mainCLI_TASK_STACK_SIZE,
                                    NULL,
                                    mainCLI_TASK_PRIORITY,
                                    NULL );
                    
                    #endif

                    #if USE_TCP

                        /* Sockets, and tasks that use the TCP/IP stack can be created here. */
                        xTaskCreate( prvCliTask_TCP,
                                    "CLI_TCP_Server",
                                    mainCLI_TASK_STACK_SIZE,
                                    NULL,
                                    mainCLI_TASK_PRIORITY,
                                    NULL );
                    
                    #endif

                    #if ( BUILD_IPERF3 == 1 )

                        #if USE_IPERF3

                            vIPerfInstall();

                        #endif /* USE_IPERF3 */

                    #endif /* ( BUILD_IPERF3 == 1 ) */

                #endif /* ( ipconfigUSE_IPv4 != 0) */

                #if USE_USER_COMMAND_TASK

                    /* Sockets, and tasks that use the TCP/IP stack can be created here. */
                    xTaskCreate( prvServerWorkTask,
                                "user_cmnd_task",
                                mainUSER_COMMAND_TASK_STACK_SIZE,
                                NULL,
                                mainUSER_COMMAND_TASK_PRIORITY,
                                NULL );

                #endif


                #if USE_TCP_ECHO_CLIENT

                    void vStartTCPEchoClientTasks_SingleTasks( uint16_t usTaskStackSize, UBaseType_t uxTaskPriority );
                    vStartTCPEchoClientTasks_SingleTasks(mainCLI_TASK_STACK_SIZE, mainCLI_TASK_PRIORITY);

                #endif


                #if USE_UDP_ECHO_SERVER
                    xTaskCreate( prvSimpleServerTask, "SimpCpySrvr", 2048, UDP_ECHO_PORT, tskIDLE_PRIORITY, NULL );
                #endif /* USE_UDP_ECHO_SERVER */

                #if USE_TCP_ECHO_SERVER
                    extern void vStartSimpleTCPServerTasks( uint16_t usStackSize, UBaseType_t uxPriority );
                    vStartSimpleTCPServerTasks(mainCLI_TASK_STACK_SIZE, mainCLI_TASK_PRIORITY);
                #endif /* USE_TCP_ECHO_SERVER */

                #if USE_CORE_HTTP_DEMO
                    extern void vStartSimpleHTTPDemo( void );
                    vStartSimpleHTTPDemo();
                #endif /* USE_CORE_HTTP_DEMO */    


                network_up = 1;
                xTaskNotifyGive( network_up_task_handle );

            }

        }

        #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )

            showEndPoint( pxEndPoint );
        
        #else
        
            /* Print out the network configuration, which may have come from a DHCP
            * server. */
            #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                FreeRTOS_GetEndPointConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress, pxNetworkEndPoints );
            #else
                FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
            #endif /* defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 ) */

            FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
            configPRINTF( ( "IP Address: %s\n", cBuffer ) );

            FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
            configPRINTF( ( "Subnet Mask: %s\n", cBuffer ) );

            FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
            configPRINTF( ( "Gateway Address: %s\n", cBuffer ) );

            FreeRTOS_inet_ntoa( ulDNxzaSServerAddress, cBuffer );
            configPRINTF( ( "DNS Server Address: %s\n", cBuffer ) );
        
        #endif /* defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 ) */

    }
    else if( eNetworkEvent == eNetworkDown )
    {
    	network_up = 0;
    }
}
/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

#if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
	BaseType_t xApplicationDNSQueryHook_Multi( struct xNetworkEndPoint * pxEndPoint, const char * pcName )
#else
	BaseType_t xApplicationDNSQueryHook( const char * pcName )
#endif /* defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 ) */
{
    BaseType_t xReturn = pdFAIL;

    /* Determine if a name lookup is for this node.  Two names are given
     * to this node: that returned by pcApplicationHostnameHook() and that set
     * by mainDEVICE_NICK_NAME. */
    if( strcasecmp( pcName, pcApplicationHostnameHook() ) == 0 )
    {
        xReturn = pdPASS;
    }
    return xReturn;
}

#endif /* ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) */

/*-----------------------------------------------------------*/

const char *pcApplicationHostnameHook( void )
{
    /* Assign the name "STM32H7" to this network node.  This function will be
     * called during the DHCP: the machine will be registered with an IP address
     * plus this name. */
    return "STM32H7";
}
/*-----------------------------------------------------------*/

BaseType_t xApplicationGetRandomNumber( uint32_t *pulValue )
{
BaseType_t xReturn;

    if( HAL_RNG_GenerateRandomNumber( &hrng, pulValue ) == HAL_OK )
    {
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

void vPrintStringToUart( const char *str )
{
    HAL_UART_Transmit( &( huart3 ), ( const uint8_t * )str, strlen( str ), 1000 );
}
/*-----------------------------------------------------------*/

void vIncrementTim7Tick( void )
{
    ulTim7Tick++;
}
/*-----------------------------------------------------------*/

uint32_t ulGetTim7Tick( void )
{
    return ulTim7Tick;
}
/*-----------------------------------------------------------*/

// void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
// {
//     /* If configCHECK_FOR_STACK_OVERFLOW is set to either 1 or 2 then this
//      * function will automatically get called if a task overflows its stack. */
//     ( void ) pxTask;
//     ( void ) pcTaskName;
//     for( ;; );
// }
// /*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* If configUSE_MALLOC_FAILED_HOOK is set to 1 then this function will
     * be called automatically if a call to pvPortMalloc() fails.  pvPortMalloc()
     * is called automatically when a task, queue or semaphore is created. */
    for( ;; );
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/**************  ST CUBE IDE GENERATED CODE  *****************/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/**
* @brief ETH MSP Initialization
* This function configures the hardware resources used in this example
* @param heth: ETH handle pointer
* @retval None
*/
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(heth->Instance==ETH)
  {
  /* USER CODE BEGIN ETH_MspInit 0 */

  /* USER CODE END ETH_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_ETH_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* ETH interrupt Init */
    HAL_NVIC_SetPriority(ETH_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
    HAL_NVIC_SetPriority(ETH_WKUP_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ETH_WKUP_IRQn);
  /* USER CODE BEGIN ETH_MspInit 1 */

  /* USER CODE END ETH_MspInit 1 */
  }

}

/**
* @brief ETH MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param heth: ETH handle pointer
* @retval None
*/
void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  if(heth->Instance==ETH)
  {
  /* USER CODE BEGIN ETH_MspDeInit 0 */

  /* USER CODE END ETH_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ETH_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    HAL_GPIO_DeInit(GPIOC, RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin);

    HAL_GPIO_DeInit(GPIOA, RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin);

    HAL_GPIO_DeInit(RMII_TXD1_GPIO_Port, RMII_TXD1_Pin);

    HAL_GPIO_DeInit(GPIOG, RMII_TX_EN_Pin|RMII_TXD0_Pin);

    /* ETH interrupt DeInit */
    HAL_NVIC_DisableIRQ(ETH_IRQn);
    HAL_NVIC_DisableIRQ(ETH_WKUP_IRQn);
  /* USER CODE BEGIN ETH_MspDeInit 1 */

  /* USER CODE END ETH_MspDeInit 1 */
  }

}

#if USE_UDP_ECHO_SERVER
    static void prvSimpleServerTask( void *pvParameters )
    {
    int32_t lBytes;
    uint8_t cReceivedString[ 512 ];
    struct freertos_sockaddr xClient, xBindAddress;
    uint32_t xClientLength = sizeof( xClient );
    Socket_t xListeningSocket;
    int32_t lBytesSent;
    TickType_t xCLIServerRecvTimeout = portMAX_DELAY;
    uint8_t cPrio = 3;
    BaseType_t xSocketRet;

        /* Just to prevent compiler warnings. */
        ( void ) pvParameters;

        /* Attempt to open the socket. */
        xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );

        configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

        xSocketRet = FreeRTOS_setsockopt(xListeningSocket, 0, FREERTOS_SO_SOCKET_PRIORITY, &cPrio, sizeof(cPrio));

        configASSERT( xSocketRet == pdPASS );

        /* No need to return from FreeRTOS_recvfrom until a message
        * is received. */
        FreeRTOS_setsockopt( xListeningSocket,
                            0,
                            FREERTOS_SO_RCVTIMEO,
                            &( xCLIServerRecvTimeout ),
                            sizeof( TickType_t ) );

        /* This test receives data sent from a different port on the same IP
        address.  Configure the freertos_sockaddr structure with the address being
        bound to.  The strange casting is to try and remove	compiler warnings on 32
        bit machines.  Note that this task is only created after the network is up,
        so the IP address is valid here. */
        xBindAddress.sin_port = ( uint16_t ) ( ( uint32_t ) pvParameters ) & 0xffffUL;
        xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
        xBindAddress.sin_family = FREERTOS_AF_INET;

        /* Bind the socket to the port that the client task will send to. */
        FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

        FreeRTOS_printf(("********* UDP ECHO ***********\n"));

        for( ;; )
        {
            lBytes = 0;
            uint8_t cRXString[ 512 ] = {'\0'};
            char * cRX_Prefix = "TI AM243x >>> ";
            /* Zero out the receive array so there is NULL at the end of the string
            when it is printed out. */
            memset( cReceivedString, 0x00, sizeof( cReceivedString ) );

            /* Receive data on the socket.  ulFlags is zero, so the zero copy option
            is not set and the received data is copied into the buffer pointed to by
            cReceivedString.  By default the block time is portMAX_DELAY.
            xClientLength is not actually used by FreeRTOS_recvfrom(), but is set
            appropriately in case future versions do use it. */

            #if USE_ZERO_COPY

                uint8_t *pucReceivedUDPPayload = NULL;

                lBytes = FreeRTOS_recvfrom( xListeningSocket, &pucReceivedUDPPayload, 0, FREERTOS_ZERO_COPY, &xClient, &xClientLength );

                configASSERT( (pucReceivedUDPPayload != NULL) );

                memcpy(( void * )( cReceivedString ), pucReceivedUDPPayload, lBytes);

                FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucReceivedUDPPayload );

            #else

                lBytes = FreeRTOS_recvfrom( xListeningSocket, cReceivedString, sizeof( cReceivedString ), 0, &xClient, &xClientLength );

            #endif /* USE_ZERO_COPY */


            if (lBytes > 0)
            {
                configASSERT(lBytes < (512 - strlen(cRX_Prefix) ) );
                memcpy(cRXString, cRX_Prefix, strlen(cRX_Prefix));
                memcpy(cRXString + strlen(cRX_Prefix), cReceivedString, lBytes );

                #if USE_ZERO_COPY
                    /*
                     * First obtain a buffer of adequate length from the TCP/IP stack into which
                     * the string will be written. 
                     */
                    #if defined(ipconfigIPv4_BACKWARD_COMPATIBLE) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 0 )
                        uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer_Multi( lBytes + strlen(cRX_Prefix), portMAX_DELAY, ipTYPE_IPv4 );
                    #else
                        uint8_t *pucBuffer = FreeRTOS_GetUDPPayloadBuffer( lBytes + strlen(cRX_Prefix), portMAX_DELAY );
                    #endif
                    
                    configASSERT( pucBuffer != NULL );
                    memcpy( pucBuffer , cRXString, lBytes + strlen(cRX_Prefix) );

                    /* Send response. */
                    //vTaskSuspendAll();
                    //time_check = ARM_REG_DWT_CYCCNT;
                    lBytesSent = FreeRTOS_sendto( xListeningSocket,
                                                ( void * ) pucBuffer,
                                                lBytes + strlen(cRX_Prefix),
												FREERTOS_ZERO_COPY,
                                                &xClient, xClientLength );
                    //time_check = ARM_REG_DWT_CYCCNT - time_check;
                    //xTaskResumeAll();
                    FreeRTOS_debug_printf(("FreeRTOS_sendto: DELTA: %u\r\n", time_check));

                #else

                    /* Send response. */
                    FreeRTOS_sendto(xListeningSocket, cRXString, lBytes + strlen(cRX_Prefix), 0, &xClient, xClientLength );

                #endif /* USE_ZERO_COPY */

            }
        }
    }
#endif /* USE_UDP_ECHO_SERVER */

    void vApplicationGetRandomHeapCanary(portPOINTER_SIZE_TYPE* pxHeapCanary ) {
    	if (pxHeapCanary != NULL) {
    		xApplicationGetRandomNumber(pxHeapCanary);
    	}
    }
