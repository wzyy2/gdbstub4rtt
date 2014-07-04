/*
 * GDB stub.
 * 
 * Migarte form linux to rt-thread by Wzyy2
 * Original edition : KGDB stub
 * 
 * File      : gdb_stub.h
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
#ifndef __GDB_STUB_H__
#define __GDB_STUB_H__

#include <rtthread.h>
#include <arch_gdb.h>
#include <hal_stub.h>

#define GDB_MAX_BREAKPOINTS 10


/**
 * gdb_connected - Is a host GDB connected to us?
 */
int				gdb_connected;

enum gdb_bptype {
    BP_BREAKPOINT = 0,
    BP_HARDWARE_BREAKPOINT,
    BP_WRITE_WATCHPOINT,
    BP_READ_WATCHPOINT,
    BP_ACCESS_WATCHPOINT,
    BP_POKE_BREAKPOINT,
};

enum gdb_bpstate {
    BP_UNDEFINED = 0,
    BP_REMOVED,
    BP_SET,
    BP_ACTIVE
};

struct gdb_bkpt {
    unsigned long		bpt_addr;
    unsigned char		saved_instr[BREAK_INSTR_SIZE];
    enum gdb_bptype	type;
    enum gdb_bpstate	state;
};

/**
 * struct gdb_arch - Describe architecture specific values.
 * @gdb_bpt_instr: The instruction to trigger a breakpoint.
 * @flags: Flags for the breakpoint, currently just %GDB_HW_BREAKPOINT.
 * @set_hw_breakpoint: Allow an architecture to specify how to set a hardware
 * breakpoint.
 * @remove_hw_breakpoint: Allow an architecture to specify how to remove a
 * hardware breakpoint.
 * @disable_hw_break: Allow an architecture to specify how to disable
 * hardware breakpoints for a single cpu.
 * @remove_all_hw_break: Allow an architecture to specify how to remove all
 * hardware breakpoints.
 * @correct_hw_break: Allow an architecture to specify how to correct the
 * hardware debug registers.
 */
struct gdb_arch {
	unsigned char		gdb_bpt_instr[BREAK_INSTR_SIZE];
	unsigned long		flags;

	int	(*set_hw_breakpoint)(unsigned long, int, enum gdb_bptype);
	int	(*remove_hw_breakpoint)(unsigned long, int, enum gdb_bptype);
	void	(*disable_hw_break)();
	void	(*remove_all_hw_break)(void);
	void	(*correct_hw_break)(void);
};
/**
 * struct gdb_io - Describe the interface for an I/O driver to talk with KGDB.
 * @read_char: Pointer to a function that will return one char.
 * @write_char: Pointer to a function that will write one char.
 * @flush: Pointer to a function that will flush any pending writes.
 * @init: Pointer to a function that will initialize the device.
 */
struct gdb_io {
    int			(*read_char) (void);
    void			(*write_char) (char);
    void			(*flush) (void);
    int			(*init) (void);
};


extern struct gdb_io		gdb_io_ops;
extern void gdb_start();
extern void gdb_set_device(const char* device_name);

/* arch */
extern struct gdb_arch		arch_gdb_ops;
extern void gdb_get_register(unsigned long *gdb_regs);
extern void gdb_put_register(unsigned long *gdb_regs);
extern void gdb_set_register(void *hw_regs);
extern int  gdb_arch_handle_exception();

extern void gdb_handle_exception(int signo, void *regs);

#endif /* __GDB_STUB_H__ */
