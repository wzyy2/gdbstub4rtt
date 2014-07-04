/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-11-20     Bernard    the first version
 */

#include <stdlib.h>
#include <rtthread.h>
#include <components.h>
#include "gdb_stub.h"



void rt_init_thread_entry(void* parameter)
{
    gdb_set_device("uart4");
    gdb_start();

    while(1){
        rt_kprintf("RT_THREAD run now!!!!!!!!!!!!!!!!!!!!!!!!! \n");
        rt_thread_delay(1000);
    }
}

int rt_application_init()
{
    rt_thread_t tid;

    /* do component initialization */
    rt_components_init();
#ifdef RT_USING_NEWLIB
	libc_system_init(RT_CONSOLE_DEVICE_NAME);
#endif

    tid = rt_thread_create("init",
        rt_init_thread_entry, RT_NULL,
        2048, RT_THREAD_PRIORITY_MAX/3, 20);

    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}
