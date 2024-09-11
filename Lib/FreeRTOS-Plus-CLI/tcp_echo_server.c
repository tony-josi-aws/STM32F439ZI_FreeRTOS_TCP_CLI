/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * FreeRTOS tasks are used with FreeRTOS+TCP to create a TCP echo server on the
 * standard echo port number (7).
 *
 * See the following web page for essential demo usage and configuration
 * details:
 * https://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Echo_Server.html
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* Enable to use TCP zero copy interface for echo server */
#define USE_TCP_ZERO_COPY 		     		0

/* Remove the whole file if FreeRTOSIPConfig.h is set to exclude TCP. */
#if( ipconfigUSE_TCP == 1 )

/* The maximum time to wait for a closing socket to close. */
#define tcpechoSHUTDOWN_DELAY	( pdMS_TO_TICKS( 5000 ) )

/* If ipconfigUSE_TCP_WIN is 1 then the Tx sockets will use a buffer size set by
ipconfigTCP_TX_BUFFER_LENGTH, and the Tx window size will be
configECHO_SERVER_TX_WINDOW_SIZE times the buffer size.  Note
ipconfigTCP_TX_BUFFER_LENGTH is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
stack constant, whereas configECHO_SERVER_TX_WINDOW_SIZE is set in
FreeRTOSConfig.h as it is a demo application constant. */
#ifndef configECHO_SERVER_TX_WINDOW_SIZE
	#define configECHO_SERVER_TX_WINDOW_SIZE	2
#endif

/* If ipconfigUSE_TCP_WIN is 1 then the Rx sockets will use a buffer size set by
ipconfigTCP_RX_BUFFER_LENGTH, and the Rx window size will be
configECHO_SERVER_RX_WINDOW_SIZE times the buffer size.  Note
ipconfigTCP_RX_BUFFER_LENGTH is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
stack constant, whereas configECHO_SERVER_RX_WINDOW_SIZE is set in
FreeRTOSConfig.h as it is a demo application constant. */
#ifndef configECHO_SERVER_RX_WINDOW_SIZE
	#define configECHO_SERVER_RX_WINDOW_SIZE	2
#endif

/*-----------------------------------------------------------*/

/*
 * Uses FreeRTOS+TCP to listen for incoming echo connections, creating a task
 * to handle each connection.
 */
static void prvConnectionListeningTask( void *pvParameters );

/*
 * Created by the connection listening task to handle a single connection.
 */
static void prvServerConnectionInstance( void *pvParameters );

/*-----------------------------------------------------------*/

/* Stores the stack size passed into vStartSimpleTCPServerTasks() so it can be
reused when the server listening task creates tasks to handle connections. */
static uint16_t usUsedStackSize = 0;

/*-----------------------------------------------------------*/

void vStartSimpleTCPServerTasks( uint16_t usStackSize, UBaseType_t uxPriority, struct freertos_sockaddr *pxParams )
{
	/* Create the TCP echo server. */
	xTaskCreate( prvConnectionListeningTask, "ServerListener", usStackSize, pxParams, uxPriority, NULL );

	/* Remember the requested stack size so it can be re-used by the server
	listening task when it creates tasks to handle connections. */
	usUsedStackSize = usStackSize;
}
/*-----------------------------------------------------------*/

static void prvConnectionListeningTask( void *pvParameters )
{
struct freertos_sockaddr xClient, xBindAddress;
Socket_t xListeningSocket, xConnectedSocket;
socklen_t xSize = sizeof( xClient );
static const TickType_t xReceiveTimeOut = portMAX_DELAY;
const BaseType_t xBacklog = 20;
struct freertos_sockaddr *pxParamsSocket;

#if( ipconfigUSE_TCP_WIN == 1 )
	WinProperties_t xWinProps;

	/* Fill in the buffer and window sizes that will be used by the socket. */
	xWinProps.lTxBufSize = ipconfigTCP_TX_BUFFER_LENGTH;
	xWinProps.lTxWinSize = configECHO_SERVER_TX_WINDOW_SIZE;
	xWinProps.lRxBufSize = ipconfigTCP_RX_BUFFER_LENGTH;
	xWinProps.lRxWinSize = configECHO_SERVER_RX_WINDOW_SIZE;
#endif /* ipconfigUSE_TCP_WIN */

	pxParamsSocket =  (struct freertos_sockaddr *) pvParameters;

	/* Attempt to open the socket. */
	xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );

	/* Set the window and buffer sizes. */
	#if( ipconfigUSE_TCP_WIN == 1 )
	{
		FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
	}
	#endif /* ipconfigUSE_TCP_WIN */

	/* Bind the socket to the port that the client task will send to, then
	listen for incoming connections. */
	xBindAddress.sin_address.ulIP_IPv4 = pxParamsSocket->sin_address.ulIP_IPv4;
	xBindAddress.sin_port = pxParamsSocket->sin_port;
	xBindAddress.sin_family = pxParamsSocket->sin_family;
	FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );
	FreeRTOS_listen( xListeningSocket, xBacklog );

	for( ;; )
	{
		/* Wait for a client to connect. */
		xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
		configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );

		/* Spawn a task to handle the connection. */
		xTaskCreate( prvServerConnectionInstance, "EchoServer", usUsedStackSize, ( void * ) xConnectedSocket, tskIDLE_PRIORITY, NULL );
	}
}
/*-----------------------------------------------------------*/

static void prvServerConnectionInstance( void *pvParameters )
{
int32_t lBytes, lSent, lTotalSent;
Socket_t xConnectedSocket;
static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 5000 );
static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 5000 );
TickType_t xTimeOnShutdown;
uint8_t *pucRxBuffer;

	xConnectedSocket = ( Socket_t ) pvParameters;

	/* Attempt to create the buffer used to receive the string to be echoed
	back.  This could be avoided using a zero copy interface that just returned
	the same buffer. */
	pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );

	if( pucRxBuffer != NULL )
	{
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );

		for( ;; )
		{
			/* Zero out the receive array so there is NULL at the end of the string
			when it is printed out. */
			memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

			/* Receive data on the socket. */
            #if USE_TCP_ZERO_COPY

                uint8_t *pucZeroCopyRxBuffPtr = NULL;
                lBytes = FreeRTOS_recv( xConnectedSocket, &pucZeroCopyRxBuffPtr, ipconfigTCP_MSS, FREERTOS_ZERO_COPY );
                if( pucZeroCopyRxBuffPtr != NULL )
                {
                    memcpy( pucRxBuffer, pucZeroCopyRxBuffPtr, lBytes );
                    FreeRTOS_ReleaseTCPPayloadBuffer( xConnectedSocket, pucZeroCopyRxBuffPtr, lBytes );
                }
                else
                {
                    lBytes = -1;
                }

            #else

                lBytes = FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );

            #endif /* USE_TCP_ZERO_COPY */

			/* If data was received, echo it back. */
			if( lBytes >= 0 )
			{

                lSent = 0;
                lTotalSent = 0;
                
                #if USE_TCP_ZERO_COPY

                    while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
                    {

                        BaseType_t xAvlSpace = 0;
                        BaseType_t xBytesToSend = 0;
                        /* Get the stream buffer */
                        uint8_t *pucTCPZeroCopyStrmBuffer = FreeRTOS_get_tx_head( xConnectedSocket, &xAvlSpace );

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
                            memcpy(pucTCPZeroCopyStrmBuffer, ( void * ) (( (uint8_t *) pucRxBuffer ) + lTotalSent),  xBytesToSend);
                        }
                        else
                        {
                            break;
                        }

                        /* Sent using zero copy, NOTE: buffer is NULL */
                        lSent = FreeRTOS_send( xConnectedSocket, NULL, xBytesToSend, 0 );
                        lTotalSent += lSent;
                    }

                    if( lSent < 0 )
                    {
                        /* Socket closed? */
                        break;
                    }

                #else

                    /* Call send() until all the data has been sent. */
                    while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
                    {
                        lSent = FreeRTOS_send( xConnectedSocket, pucRxBuffer, lBytes - lTotalSent, 0 );
                        lTotalSent += lSent;
                    }

                    if( lSent < 0 )
                    {
                        /* Socket closed? */
                        break;
                    }

                #endif
			}
			else
			{
				/* Socket closed? */
				break;
			}
		}
	}

	/* Initiate a shutdown in case it has not already been initiated. */
	FreeRTOS_shutdown( xConnectedSocket, FREERTOS_SHUT_RDWR );

	/* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
	returning an error. */
	xTimeOnShutdown = xTaskGetTickCount();
	do
	{
		if( FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
		{
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

	/* Finished with the socket, buffer, the task. */
	vPortFree( pucRxBuffer );
	FreeRTOS_closesocket( xConnectedSocket );

	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

/* The whole file is excluded if TCP is not compiled in. */
#endif /* ipconfigUSE_TCP */

