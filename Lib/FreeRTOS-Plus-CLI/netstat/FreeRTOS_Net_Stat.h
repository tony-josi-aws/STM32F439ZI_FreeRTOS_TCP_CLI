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

#ifndef FREERTOS_NET_STAT_H
#define FREERTOS_NET_STAT_H

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern uint32_t request_stat;

typedef enum
{
    eStartStat = 0,
    eStopStat,
    eGetStat
} eState_t;

typedef enum
{
    eDataStat = 0,
    ePerfState,
    eAllStat
} eStatVal_t;

typedef enum
{
    eUDP = 0,
    eTCP,
    eICMP,
    eAllProt
} eProtocol_t;

typedef enum
{
    eIncorrectStat = 0,
    eIncorrectCast,
    eIncorrectStatType,
    eIncorrectProtocol,
    eIncorrectBuffer,
    eSuccessStat
} eErrorType_t;

typedef struct network_stats
{
    uint32_t pckt_rx;
    uint32_t pckt_tx;
    uint32_t pcket_drop_rx;
    uint32_t pcket_drop_tx;
    uint32_t bytes_rx;
    uint32_t bytes_tx;
} stats;

typedef struct TCP_stats
{
    stats stat;
} tcp;

typedef struct ICMP_stats
{
    stats stat;
} icmp;

typedef struct UDP_stats
{
    stats stat;
} udp;

typedef struct all_stats
{
    tcp tcp_stat;
    udp udp_stat;
    icmp icmp_stat;
    uint64_t rx_latency;
    uint64_t tx_latency;
} allstat;

/* Functions to collect UDP statistics */

void vUdpPacketRecvCount();
void vUdpPacketSendCount();
void vUdpRxPacketLossCount();
void vUdpTxPacketLossCount();
void vUdpDataRecvCount( size_t bytes );
void vUdpDataSendCount( size_t bytes );

/* Functions to collect TCP statistics */

void vTcpPacketRecvCount();
void vTcpPacketSendCount();
void vTcpTxPacketLossCount();
void vTcpRxPacketLossCount();
void vTcpDataRecvCount( size_t bytes );
void vTcpDataSendCount( size_t bytes );

/* Functions to collect ICMP statistics */

void vIcmpPacketRecvCount();
void vIcmpPacketSendCount();
void vIcmpRxPacketLossCount();
void vIcmpTxPacketLossCount();
void vIcmpDataRecvCount( size_t bytes );
void vIcmpDataSendCount( size_t bytes );

/* Functions to collect performance statistics */

void vGetRxLatency( uint64_t rtt );
void vGetTxLatency( uint64_t rtt );

/* Functions to send statistics */

eErrorType_t vGetNetStat( eState_t state,
                          allstat * result );

/* *INDENT-OFF* */
#ifdef __cplusplus
    } /* extern "C" */
#endif
/* *INDENT-ON* */

#endif /* FREERTOS_NET_STAT_H */
