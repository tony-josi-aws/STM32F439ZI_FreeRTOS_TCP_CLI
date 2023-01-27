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

#ifndef FREERTOS_TIME_H
#define FREERTOS_TIME_H

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/*
 ********************************************************************************
 * MAXIMUM NUMBER OF MEASURED TIME SNIPPETS
 ********************************************************************************
 */

#define MEASURED_CYCLE_CNT_MAX_NUM    10

/* To be defined as per clock speed set */
#define CLOCK_SPEED_HTZ               ( 64000000L )

#define GET_TIME_MICROSEC_CYCLE_COUNT( cycles )    ( ( cycles * 1000000L ) / CLOCK_SPEED_HTZ )
#define GET_TIME_NANOSEC_CYCLE_COUNT( cycles )     ( ( cycles * 1000000000L ) / CLOCK_SPEED_HTZ )

/*
 ********************************************************************************
 * Data Structures
 ********************************************************************************
 */

typedef struct measured_cycle_count
{
    uint32_t uiStart;
    uint32_t uiEnd;
} MeasuredCycleCount_t;

/*
 ********************************************************************************
 * FUNCTION PROTOTYPES
 ********************************************************************************
 */

void vMeasureCycleCountInit( void );                                                /* Module initialization */
void vMeasureCycleCountDeinit( void );                                              /* Module deinitialization */
void vMeasureCycleCountStart( MeasuredCycleCount_t * pMeasuredCycleCountData );     /* Start measurement */
uint32_t uiMeasureCycleCountStop( MeasuredCycleCount_t * pMeasuredCycleCountData ); /* Stop measurement */

/* *INDENT-OFF* */
#ifdef __cplusplus
    } /* extern "C" */
#endif
/* *INDENT-ON* */

#endif /* FREERTOS_TIME_H */
