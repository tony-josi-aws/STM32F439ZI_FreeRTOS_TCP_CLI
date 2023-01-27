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
 * @file FreeRTOS_Time.c
 * @brief Implements the .
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS_Time.h"

/*
 ********************************************************************************
 * CORTEX-M - Debug Watch and Trace (DWT) TIMER
 ********************************************************************************
 */

/* Debug Exception and Monitor Control Register */
#define ARM_REG_DEMCR         ( *( volatile uint32_t * ) 0xE000EDFC )

/* DWT Control register */
#define ARM_REG_DWT_CTRL      ( *( volatile uint32_t * ) 0xE0001000 )

/* DWT Cycle Count Register */
#define ARM_REG_DWT_CYCCNT    ( *( volatile uint32_t * ) 0xE0001004 )

/* CYCCNTENA bit in DWT_CONTROL register */
#define DWT_CYCCNTENA_BIT     ( 1UL << 0 )

/* Trace enable bit in DEMCR register */
#define DWT_TRCENA_BIT        ( 1UL << 24 )


/*
 ********************************************************************************
 * MODULE INITIALIZATION
 *
 * Important: This must be called before any of the other functions for cycle
 * count
 ********************************************************************************
 */

void vMeasureCycleCountInit( void )
{
    if( ARM_REG_DWT_CTRL != 0 )
    {
        /* See if DWT is available */
        ARM_REG_DEMCR |= DWT_TRCENA_BIT;       /* Set bit 24 */
        ARM_REG_DWT_CYCCNT = 0;
        ARM_REG_DWT_CTRL |= DWT_CYCCNTENA_BIT; /* Set bit 0 */
    }
}

/*
 ********************************************************************************
 * MODULE DEINITIALIZATION
 *
 * Important: If this will be called, it will disable Cycle Count in DWT module
 ********************************************************************************
 */

void vMeasureCycleCountDeinit( void )
{
    ARM_REG_DWT_CYCCNT = 0;                 /* Clear Cycle Count */
    ARM_REG_DWT_CTRL &= ~DWT_CYCCNTENA_BIT; /* Clear bit 0 */
}

/*
 ********************************************************************************
 * START THE CYCLE COUNT FOR A CODE SECTION
 ********************************************************************************
 */

void vMeasureCycleCountStart( MeasuredCycleCount_t * pMeasuredCycleCountData )
{
    pMeasuredCycleCountData->uiStart = ARM_REG_DWT_CYCCNT;
    pMeasuredCycleCountData->uiEnd = 0; /* not needed though */
}

/*
 ********************************************************************************
 * STOP THE CYCLE COUNT AFTER THE CODE SECTION AND RECORD STATS
 ********************************************************************************
 */

uint32_t uiMeasureCycleCountStop( MeasuredCycleCount_t * pMeasuredCycleCountData )
{
    uint32_t uiCycleCount = 0;
    uint32_t timeInMicroSec;

    pMeasuredCycleCountData->uiEnd = ARM_REG_DWT_CYCCNT;
    uiCycleCount = pMeasuredCycleCountData->uiEnd - pMeasuredCycleCountData->uiStart;
    timeInMicroSec = GET_TIME_MICROSEC_CYCLE_COUNT( uiCycleCount );

    return timeInMicroSec;
}
