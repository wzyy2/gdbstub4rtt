/*
 * I/O portion of GDB stub
 *
 * File      : hal_stub.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2014-07-04     Wzyy2      first version
 */
#ifndef __HAL_STUB_H__
#define __HAL_STUB_H__


int gdb_putc(char c);
int gdb_getc();
void gdb_start();
void gdb_set_device(const char* device_name);

#endif /* __HAL_STUB_H__ */
