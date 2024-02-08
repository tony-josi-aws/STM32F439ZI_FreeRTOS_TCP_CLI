/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef CORE_HTTP_CONFIG_H_
#define CORE_HTTP_CONFIG_H_

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging config definition and header files inclusion are required in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for HTTP.
 * 3. Include the header file "logging_stack.h", if logging is enabled for HTTP.
 */

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#ifndef LOG_METADATA_ARGS
    #define LOG_METADATA_ARGS    __FUNCTION__, __LINE__  /**< @brief Arguments into the metadata logging prefix format. */
#endif

/* Logging configuration for the HTTP library. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "HTTP"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/* Metadata information to prepend to every log message. */
#ifndef LOG_METADATA_FORMAT
    #define LOG_METADATA_FORMAT    "[%s:%d] "                  /**< @brief Format of metadata prefix in log messages. */
#endif

/* Only INFO, WARNING, ERROR, and ALWAYS messages will be logged. */
#define LogAlways( message )    SdkLog( ( "[ALWAYS] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#define LogError( message )     SdkLog( ( "[ERROR] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#define LogWarn( message )      SdkLog( ( "[WARN] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#define LogInfo( message )      SdkLog( ( "[INFO] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )
#define LogDebug( message )		SdkLog( ( "[Debug] [%s] "LOG_METADATA_FORMAT, LIBRARY_LOG_NAME, LOG_METADATA_ARGS ) ); SdkLog( message ); SdkLog( ( "\r\n" ) )

/************ End of logging configuration ****************/

#endif /* ifndef CORE_HTTP_CONFIG_H_ */
