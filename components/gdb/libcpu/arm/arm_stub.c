/*
 * ARM GDB support
 * arch-specific portion of GDB stub
 * 
 * File      : arm_stub.c
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
#include <rtthread.h>
#include <gdb_stub.h>

#include <arch_gdb.h>


static int compiled_break = 0;

/*struct gdb_arch - Describe architecture specific values.*/
struct gdb_arch arch_gdb_ops = {
	.gdb_bpt_instr		= {0xe7, 0xff, 0xde, 0xfe}
};


/**
 * gdb_breakpoint - generate a compiled_breadk
 * It is used to sync up with a debugger and stop progarm
 */
void gdb_breakpoint()
{
    compiled_break = 1;
    asm(".word 0xe7ffdeff");
}


struct rt_gdb_register
{
    rt_uint32_t r0;
    rt_uint32_t r1;
    rt_uint32_t r2;
    rt_uint32_t r3;
    rt_uint32_t r4;
    rt_uint32_t r5;
    rt_uint32_t r6;
    rt_uint32_t r7;
    rt_uint32_t r8;
    rt_uint32_t r9;
    rt_uint32_t r10;
    rt_uint32_t fp;
    rt_uint32_t ip;
    rt_uint32_t sp;
    rt_uint32_t lr;
    rt_uint32_t pc;
    rt_uint32_t cpsr;
    rt_uint32_t ORIG_r0;
}*regs;



void gdb_set_register(void *hw_regs)
{
    regs = (struct rt_gdb_register *)hw_regs;
}




void gdb_get_register(unsigned long *gdb_regs)
{
    int regno;
    /* Initialize all to zero. */
    for (regno = 0; regno < 17; regno++)
        gdb_regs[regno] = 0;

    gdb_regs[GDB_R0]		= regs->r0;
    gdb_regs[GDB_R1]		= regs->r1;
    gdb_regs[GDB_R2]		= regs->r2;
    gdb_regs[GDB_R3]		= regs->r3;
    gdb_regs[GDB_R4]		= regs->r4;
    gdb_regs[GDB_R5]		= regs->r5;
    gdb_regs[GDB_R6]		= regs->r6;
    gdb_regs[GDB_R7]		= regs->r7;
    gdb_regs[GDB_R8]		= regs->r8;
    gdb_regs[GDB_R9]		= regs->r9;
    gdb_regs[GDB_R10]	    = regs->r10;
    gdb_regs[GDB_FP]		= regs->fp;
    gdb_regs[GDB_IP]		= regs->ip;
    gdb_regs[GDB_SPT]	    = regs->sp;
    gdb_regs[GDB_LR]		= regs->lr;
    gdb_regs[GDB_PC]		= regs->pc;
    gdb_regs[GDB_CPSR]  	= regs->cpsr;

};


void gdb_put_register(unsigned long *gdb_regs)
{
    regs->r0    	= gdb_regs[GDB_R0];
    regs->r1    	= gdb_regs[GDB_R1];
    regs->r2    	= gdb_regs[GDB_R2];
    regs->r3    	= gdb_regs[GDB_R3];
    regs->r4    	= gdb_regs[GDB_R4];
    regs->r5    	= gdb_regs[GDB_R5];
    regs->r6    	= gdb_regs[GDB_R6];
    regs->r7    	= gdb_regs[GDB_R7];
    regs->r8    	= gdb_regs[GDB_R8];
    regs->r9    	= gdb_regs[GDB_R9];
    regs->r10   	= gdb_regs[GDB_R10];
    regs->fp        = gdb_regs[GDB_FP];
    regs->ip    	= gdb_regs[GDB_IP];
    regs->sp    	= gdb_regs[GDB_SPT];
    regs->lr	    = gdb_regs[GDB_LR];
    regs->pc        = gdb_regs[GDB_PC];
    regs->cpsr      = gdb_regs[GDB_CPSR];
}


/* It will be called during process_packet */
int gdb_arch_handle_exception(char *remcom_in_buffer,
                              char *remcom_out_buffer)
{
    unsigned long addr;
    char *ptr;

    switch (remcom_in_buffer[0]) {
        case 'D':
        case 'k':
        case 'c':
            /*
             * If this was a compiled breakpoint, we need to move
             * to the next instruction or we will breakpoint
             * over and over again
             */
            ptr = &remcom_in_buffer[1];
            if (gdb_hex2long(&ptr, &addr))
                regs->pc = addr;
            else if (compiled_break == 1)
                regs->pc += 4;

            compiled_break = 0;

            return 0;

        case 's':
            if (compiled_break == 1)
                regs->pc += 4;
            
            compiled_break = 0;

            return 0;
    }

	return -1;

}



