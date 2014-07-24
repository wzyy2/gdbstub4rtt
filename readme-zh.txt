说明: 

一 当前版本进度
1).目前Gdb Stub实现的功能有断点,单步,打印变量
未实现的有数据断点(可以参考linux kernel/arch/X86下的GDB文件),多进程(主要是用于多核CPU的调试)
还有异常向量汇编部分写的太丑,可以抽象出来

2).目前已移植的平台有
1.arm9,cortex A系列(基于软件断点,需要程序运行于ram,默认断点最大20,改变数量需要更改RT_GDB_MAX_BREAKPOINTS)
//2.cortex M系列(基于硬件断点,使用需要注意断点的数量限制)

二 安装说明
1) 下载
下载最新RT-THREAD GDB STUB代码,并解压

2) 加入RT-Thread
先将得到的components文件夹覆盖RTT根目录
然后若要使用BBB板测试GDB,就用BSP/beaglebone和libpcu/am335x覆盖RTT下的同名文件夹

若要使用STM32F407测试GDB,就用BSP/xxx覆盖RTT下的同名文件夹

三 宏配置说明
1) RT_GDB_HAVE_HWBP
1开启硬件断点
注意平台是否支持!!!!!!!!!!!!!!


2) RT_GDB_HAVE_SWBP
1开启软件断点
注意平台是否支持!!!!!!!!!!!!!!


3) RT_GDB_MAX_BREAKPOINTS
最大软件断点数量
不加默认20

4) RT_GDB_ICACHE
是否使用ICACHE
若使用了ICACHE则要开启此宏,保证断点覆盖后及时刷新

5) RT_GDB_DEBUG
测试开发用
会打印GDB收发信息到默认串口上,以供获取数据
 
四 使用说明
1) 移植平台(BBB板直接跳至步骤4)
若是cortexA或者arm9架构的片子,参考BBB板DEMO
在udef异常的函数里,加入gdb_undef_hook的调用
还有将其start_gcc.s的相关异常向量汇编复制到自己的undef向量里
其作用是压入寄存器供GDB作参数调用和恢复寄存器 回到程序现场
可能会出现相关问题
(注意是否分配了适量的栈空间给undef模式)

若是cortexM架构的片子,参考STM32F407

2) 设置serial设备
首先需要调用
	void gdb_set_device(const char* device_name);    (gdb_stub.h)
设置相应的GDB通信端口(注意不能使用已经使用端口)
PS:注意参考BBB板serial设置!!!!!!!!!!
首先serial的驱动不能有中断
其次收发函数最好是poll的!!!!!!不然可能会出现问题
若出现问题,请打开 RT_GDB_DEBUG ,观察收发是否正常!!!!!!!!!

3) 进入GDB stub
调用
	void gdb_start();  (gdb_stub.h)
触发异常,即可进入等待状态 等待GDB连接
也可以将该函数加入按键中断中,可以实现外部打断程序的效果(条件有限,未测试)

4) pc连接
确认GCC 打开-g选项后编译
然后arm-none-eabi-gdb xxxx.elf
进如GDB界面后
set serial baud 115200
target remote /dev/ttyUSB0(linux)  或者 COM1 (windows)
即可实现GDB对接

PS:
	编译需要打开-g选项(最好是-g,不然可能出现未知错误)
	BBB板默认DEBUG串口输出,uart4作为GDB通信口


五 移植说明
若要移植到不同架构上
请参考gdb/libcpu下的文件
主要就是硬件断点相关和寄存器相关工作
需要rsp协议的话 请参考https://www.sourceware.org/gdb/current/onlinedocs/gdb/Remote-Protocol.html
