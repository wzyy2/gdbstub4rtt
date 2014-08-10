# GDB stub for RT-Thread #

GDB Stub 是 GDB 在进行远程调试的时候，在目标机上运行的一套代码。其功能描述可以参 考 https://sourceware.org/gdb/onlinedocs/gdb/Remote-Stub.html 。简单的说来，本 项目就是在 RT-Thread 中实现类似于 KGDB 的功能，用来在没有仿真器(JTAG)的情况下调 试RT-Thread 内核和应用程序。

RT-Thread is an open source real-time operating system for embedded devices from China. RT-Thread RTOS is a scalable real-time operating system: a tiny kernel for ARM Cortex-M0, Cortex-M3/4, or a full feature system in ARM Cortex-A8, ARM Cortex-A9 DualCore etc. 


## Usage ##

1. use it in RT-Thread
 
	you have to copy the code to the RTT-ROOT folder.
	you can find demo from the bsp of beaglebone.

## Help ##

    more in readme-zh

