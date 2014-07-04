#ifndef __ARM_GDB_H__
#define __ARM_GDB_H__


#define BREAK_INSTR_SIZE	4
#define HAL_BREAKINST		0xe7ffdefe
#define HAL_COMPILED_BREAK	0xe7ffdeff


#define _GP_REGS		17
#define _FP_REGS		0
#define _EXTRA_REGS		0
#define GDB_MAX_REGS		(_GP_REGS + (_FP_REGS * 3) + _EXTRA_REGS)

#define GDB_MAX_NO_CPUS	1
#define BUFMAX			200
#define NUMREGBYTES		(GDB_MAX_REGS << 2)

#define _R0			0
#define _R1			1
#define _R2			2
#define _R3			3
#define _R4			4
#define _R5			5
#define _R6			6
#define _R7			7
#define _R8			8
#define _R9			9
#define _R10		10
#define _FP			11
#define _IP			12
#define _SPT	  13
#define _LR			14
#define _PC			15
#define _CPSR			(GDB_MAX_REGS - 1)


#endif /* __ARM_GDB_H__ */
