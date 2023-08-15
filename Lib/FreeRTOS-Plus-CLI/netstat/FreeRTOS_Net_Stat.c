/*
 * FreeRTOS+TCP <DEVELOPMENT BRANCH>
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
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
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_Net_Stat.c
 * @brief Implements the function to get Network statistics.
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "FreeRTOS_Net_Stat.h"
#include "FreeRTOS_Time.h"

/* Count values for number of averaged latency */
static uint64_t txCount;
static uint64_t rxCount;

/* request_stat = 1 indicate start collecting logs
 * request_stat = 0 indicates stop collecting logs.  */
uint32_t request_stat = 1;

/* Protocol and performance statistics */
static udp udp_info;
static tcp tcp_info;
static icmp icmp_info;
static uint64_t rxLatency_info;
static uint64_t txLatency_info;

/**
 * @brief Print a summary of all sockets and their connections.
 */
void reset_stat()
{
    /* Reset all strctures to zero */
    ( void ) memset( &udp_info, 0, sizeof( udp ) );
    ( void ) memset( &tcp_info, 0, sizeof( tcp ) );
    ( void ) memset( &icmp_info, 0, sizeof( icmp ) );
    rxLatency_info = 0;
    rxLatency_info = 0;
    txCount = 0;
    rxCount = 0;
}

/* void vGetNetStat( eState_t state, uint32_t cast, uint32_t stat_type, uint32_t protocol, allstat result  ) */
eErrorType_t vGetNetStat( eState_t state,
                          allstat * result )
{
    vMeasureCycleCountInit();

    if( result == NULL )
    {
        /* Incorrect Result buffer */
        return eIncorrectBuffer;
    }

    if( state == eStartStat )
    {
        /* Start collecting logs */
        reset_stat();
        request_stat = 1;
    }
    else if( state == eStopStat )
    {
        /* Stop collecting logs */
        request_stat = 0;
    }
    else if( state == eGetStat )
    {
        /* Dump all data */
        ( void ) memcpy( &( result->udp_stat ), &udp_info, sizeof( udp ) );
        ( void ) memcpy( &( result->tcp_stat ), &tcp_info, sizeof( tcp ) );
        ( void ) memcpy( &( result->icmp_stat ), &icmp_info, sizeof( icmp ) );
        result->rx_latency = rxLatency_info;
        result->tx_latency = txLatency_info;
    }
    else
    {
        /* Incorrect Stat type */
        return eIncorrectStat;
    }

    return eSuccessStat;
}


/* Functions to collect UDP statistics */
void vUdpPacketRecvCount()
{
    udp_info.stat.pckt_rx++;
}

void vUdpPacketSendCount()
{
    udp_info.stat.pckt_tx++;
}

void vUdpRxPacketLossCount()
{
    udp_info.stat.pcket_drop_rx++;
}

void vUdpTxPacketLossCount()
{
    udp_info.stat.pcket_drop_tx++;
}

void vUdpDataRecvCount( size_t bytes )
{
    udp_info.stat.bytes_rx += bytes;
}

void vUdpDataSendCount( size_t bytes )
{
    udp_info.stat.bytes_tx += bytes;
}

/* Functions to collect TCP statistics */

void vTcpPacketRecvCount()
{
    tcp_info.stat.pckt_rx++;
}

void vTcpPacketSendCount()
{
    tcp_info.stat.pckt_tx++;
}


void vTcpTxPacketLossCount()
{
    tcp_info.stat.pcket_drop_tx++;
}

void vTcpRxPacketLossCount()
{
    tcp_info.stat.pcket_drop_rx++;
}

void vTcpDataRecvCount( size_t bytes )
{
    tcp_info.stat.bytes_rx += bytes;
}

void vTcpDataSendCount( size_t bytes )
{
    tcp_info.stat.bytes_tx += bytes;
}

/* Functions to collect TCP statistics */

void vIcmpPacketRecvCount()
{
    icmp_info.stat.pckt_rx++;
}

void vIcmpPacketSendCount()
{
    icmp_info.stat.pckt_tx++;
}

void vIcmpRxPacketLossCount()
{
    icmp_info.stat.pcket_drop_rx++;
}

void vIcmpTxPacketLossCount()
{
    icmp_info.stat.pcket_drop_tx++;
}

void vIcmpDataRecvCount( size_t bytes )
{
    icmp_info.stat.bytes_rx += bytes;
}

void vIcmpDataSendCount( size_t bytes )
{
    icmp_info.stat.bytes_tx += bytes;
}

/* Functions to collect Performance stat*/

void vGetRxLatency( uint64_t rtt )
{
    rxLatency_info =  (  rxLatency_info + rtt  * 100 ) / 2 ;

}

void vGetTxLatency( uint64_t rtt )
{
    txLatency_info =  (  txLatency_info + rtt  * 100 ) / 2 ;

}
/*-----------------------------------------------------------*/
