# FreeRTOS Kernel + TCP/IP Stack CLI

This project is a demo integration of the FreeRTOS Kernel and the FreeRTOS + TCP/IP (IPv4 for now) stack with FreeRTOS CLI over UDP/TCP on the STM32F439ZI NUCLEO board.

## Using CubeMX or .ioc file in the project

The Ethernet interface is completely decoupled from the auto generated code provided by the CubeIDE via .ioc file as we are using the NetworkInterface provided Eth init; hence the Ethernet interface is disabled in the .ioc file to disable unnecessary code being generated by the CubeIDE.

The hardware initialization part of the Ethernet is done in the `HAL_ETH_MspInit` and `HAL_ETH_MspDeInit` functions of the app_main.c which are copied versions of the CubeIDE generated code. These functions are called internally by the FreeRTOS + TCP/IP NetworkInterface layer during init.


## `iperf3` bandwidth test results:

### UDP

``` sh

C:\Users\tonyjosi\Downloads\iperf-3.1.3-win64\iperf-3.1.3-win64>iperf3.exe -c 192.168.2.114 --port 5001 --udp --bandwidth 0 --set-mss 1460 --bytes 100M
Connecting to host 192.168.2.114, port 5001
[  4] local 192.168.2.21 port 55954 connected to 192.168.2.114 port 5001
[ ID] Interval           Transfer     Bandwidth       Total Datagrams
[  4]   0.00-1.00   sec  11.6 MBytes  97.4 Mbits/sec  1490
[  4]   1.00-2.00   sec  11.4 MBytes  95.9 Mbits/sec  1460
[  4]   2.00-3.01   sec  11.5 MBytes  95.8 Mbits/sec  1470
[  4]   3.01-4.00   sec  11.4 MBytes  95.9 Mbits/sec  1460
[  4]   4.00-5.00   sec  11.4 MBytes  95.9 Mbits/sec  1460
[  4]   5.00-6.00   sec  11.4 MBytes  95.9 Mbits/sec  1460
[  4]   6.00-7.00   sec  11.4 MBytes  95.6 Mbits/sec  1460
[  4]   7.00-8.00   sec  11.5 MBytes  96.1 Mbits/sec  1470
[  4]   8.00-8.74   sec  8.36 MBytes  95.7 Mbits/sec  1070
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bandwidth       Jitter    Lost/Total Datagrams
[  4]   0.00-8.74   sec   100 MBytes  96.0 Mbits/sec  0.000 ms  0/0 (0%)
[  4] Sent 0 datagrams

iperf Done.

```

### TCP

``` sh
C:\Users\tonyjosi\Downloads\iperf-3.1.3-win64\iperf-3.1.3-win64>iperf3.exe -c 192.168.2.114 --port 5001 --bytes 100M -V
iperf 3.1.3
CYGWIN_NT-10.0 BLR135CG2096954 2.5.1(0.297/5/3) 2016-04-21 22:14 x86_64
Time: Thu, 20 Apr 2023 06:59:13 GMT
Connecting to host 192.168.2.114, port 5001
      Cookie: BLR135CG2096954.1681973953.826188.5c
      TCP MSS: 0 (default)
[  4] local 192.168.2.21 port 7632 connected to 192.168.2.114 port 5001
Starting Test: protocol: TCP, 1 streams, 131072 byte blocks, omitting 0 seconds, 104857600 bytes to send
[ ID] Interval           Transfer     Bandwidth
[  4]   0.00-1.01   sec  5.12 MBytes  42.7 Mbits/sec
[  4]   1.01-2.01   sec  5.25 MBytes  44.0 Mbits/sec
[  4]   2.01-3.00   sec  5.25 MBytes  44.3 Mbits/sec
[  4]   3.00-4.01   sec  5.38 MBytes  44.7 Mbits/sec
[  4]   4.01-5.00   sec  5.38 MBytes  45.5 Mbits/sec
[  4]   5.00-6.00   sec  5.25 MBytes  44.1 Mbits/sec
[  4]   6.00-7.01   sec  5.38 MBytes  44.9 Mbits/sec
[  4]   7.01-8.01   sec  5.25 MBytes  44.0 Mbits/sec
[  4]   8.01-9.01   sec  5.38 MBytes  45.2 Mbits/sec
[  4]   9.01-10.00  sec  5.12 MBytes  43.2 Mbits/sec
[  4]  10.00-11.01  sec  5.38 MBytes  44.6 Mbits/sec
[  4]  11.01-12.01  sec  5.25 MBytes  44.2 Mbits/sec
[  4]  12.01-13.01  sec  5.38 MBytes  45.3 Mbits/sec
[  4]  13.01-14.01  sec  5.38 MBytes  45.0 Mbits/sec
[  4]  14.01-15.01  sec  5.25 MBytes  43.9 Mbits/sec
[  4]  15.01-16.01  sec  5.38 MBytes  45.2 Mbits/sec
[  4]  16.01-17.02  sec  5.25 MBytes  43.7 Mbits/sec
[  4]  17.02-18.01  sec  5.25 MBytes  44.4 Mbits/sec
[  4]  18.01-18.88  sec  4.75 MBytes  45.5 Mbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
Test Complete. Summary Results:
[ ID] Interval           Transfer     Bandwidth
[  4]   0.00-18.88  sec   100 MBytes  44.4 Mbits/sec                  sender
[  4]   0.00-18.88  sec  99.8 MBytes  44.3 Mbits/sec                  receiver
CPU Utilization: local/sender 1.2% (0.3%u/0.9%s), remote/receiver 0.0% (0.0%u/0.0%s)

iperf Done.
```

## Notes:

TODO: When using loopback interface with TCP zero copy enabled only for TCP server, heap is getting corrupted and the TCP segments are being run out. Most likely an application issue with how application is using the zero copy. Pending debug...