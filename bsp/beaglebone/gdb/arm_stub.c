#include <rtthread.h>
#include <gdb_stub.h>

#include "am33xx.h"
#include "arch_gdb.h"


static int compiled_break = 0;
/**
 * struct gdb_arch - Describe architecture specific values.
 */
struct gdb_arch arch_gdb_ops = {
	.gdb_bpt_instr		= {0xe7, 0xff, 0xde, 0xfe}
};

/* compiled_break */
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

    gdb_regs[_R0]		= regs->r0;
    gdb_regs[_R1]		= regs->r1;
    gdb_regs[_R2]		= regs->r2;
    gdb_regs[_R3]		= regs->r3;
    gdb_regs[_R4]		= regs->r4;
    gdb_regs[_R5]		= regs->r5;
    gdb_regs[_R6]		= regs->r6;
    gdb_regs[_R7]		= regs->r7;
    gdb_regs[_R8]		= regs->r8;
    gdb_regs[_R9]		= regs->r9;
    gdb_regs[_R10]	= regs->r10;
    gdb_regs[_FP]		= regs->fp;
    gdb_regs[_IP]		= regs->ip;
    gdb_regs[_SPT]	= regs->sp;
    gdb_regs[_LR]		= regs->lr;
    gdb_regs[_PC]		= regs->pc;
    gdb_regs[_CPSR]	= regs->cpsr;

};


void gdb_put_register(unsigned long *gdb_regs)
{
    regs->r0  	= gdb_regs[_R0];
    regs->r1  	= gdb_regs[_R1];
    regs->r2  	= gdb_regs[_R2];
    regs->r3  	= gdb_regs[_R3];
    regs->r4  	= gdb_regs[_R4];
    regs->r5  	= gdb_regs[_R5];
    regs->r6  	= gdb_regs[_R6];
    regs->r7  	= gdb_regs[_R7];
    regs->r8  	= gdb_regs[_R8];
    regs->r9  	= gdb_regs[_R9];
    regs->r10 	= gdb_regs[_R10];
    regs->fp	  = gdb_regs[_FP];
    regs->ip	  = gdb_regs[_IP];
    regs->sp	  = gdb_regs[_SPT];
    regs->lr	  = gdb_regs[_LR];
    regs->pc	  = gdb_regs[_PC];
    regs->cpsr  = gdb_regs[_CPSR];
}


/* It will be called after gdb_process_exception */
int gdb_arch_handle_exception()
{
    /*
     * If this was a compiled breakpoint, we need to move
     * to the next instruction or we will just breakpoint
     * over and over again.
     */
    if (compiled_break) {
        compiled_break = 0;
        regs->pc += 4;
        return 1;
    }
    else
        return 0;
}



