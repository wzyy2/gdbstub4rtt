/*
 * GDB stub.
 * 
 * Migarte form linux to rt-thread by Wzyy2
 * Original edition : KGDB stub
 *
 * File      : gdb_stub.c
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
#include <string.h>
#include "gdb_stub.h"


struct gdb_state {
    int	            ex_vector;
    int	            signo;
    int	            err_code;
    int	            cpu;
    int	            pass_exception;
    unsigned long	thr_query;
    unsigned long	threadid;
    long	        gdb_usethreadid;
}gs;



/*
 * Holds information about breakpoints in a kernel. These breakpoints are
 * added and removed by gdb.
 */
static struct gdb_bkpt		gdb_break[GDB_MAX_BREAKPOINTS] = {
    [0 ... GDB_MAX_BREAKPOINTS-1] = { .state = BP_UNDEFINED }
};
/* Storage for the registers, in GDB format. */
static unsigned long		gdb_regs[(NUMREGBYTES +
					sizeof(unsigned long) - 1) /
					sizeof(unsigned long)];

char remcom_in_buffer[BUFMAX];
char remcom_out_buffer[BUFMAX];


static const char hexchars[] = "0123456789abcdef";
/*
 * GDB remote protocol parser:
 */
static int hex(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    if ((ch >= '0') && (ch <= '9'))
        return ch - '0';
    if ((ch >= 'A') && (ch <= 'F'))
        return ch - 'A' + 10;
    return -1;
}

static char tohex(int c)
{
    return hexchars[c & 15];
}

/*
 * Copy the binary array pointed to by buf into mem.  Fix $, #, and
 * 0x7d escaped with 0x7d.  Return a pointer to the character after
 * the last byte written.
 */
static int ebin2mem(char *buf, char *mem, int count)
{
    int err = 0;
    char c;

    while (count-- > 0) {
        c = *buf++;
        if (c == 0x7d)
            c = *buf++ ^ 0x20;

        *mem = c;
        /*err = probe_kernel_write(mem, &c, 1);*/
        /*if (err)*/
        /*break;*/

        mem++;
    }

    return err;
}

/*
 * Convert the hex array pointed to by buf into binary to be placed in mem.
 * Return a pointer to the character AFTER the last byte written.
 * May return an error.
 */
static int hex2mem(char *buf, char *mem, int count)
{
    char *tmp_raw;
    char *tmp_hex;
    char tmp_cnt;

    tmp_cnt = count * 2;
    tmp_raw = mem;
    tmp_hex = buf;

    while (tmp_cnt) {
        tmp_raw++;
        *tmp_raw = hex(*tmp_hex++);
        *tmp_raw |= hex(*tmp_hex++) << 4;
        tmp_cnt -= 2;
    }

    return 0;
}
/*
 * Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null). May return an error.
 */
static int mem2hex(char *mem, char *buf, int count)
{
    char *tmp = mem;
    char ch,i;

    while (count > 0) {
        ch = *(tmp++);
        *(buf++) = tohex((ch >> 4) & 0xf);
        *(buf++) = tohex(ch & 0xf);

        count--;
    }
    *buf = 0;

    return 0;
}

/*
 * While we find nice hex chars, build a long_val.
 * Return number of chars processed.
 */
static int hex2long(char **ptr, unsigned long *long_val)
{
    int hex_val;
    int num = 0;
    int negate = 0;

    *long_val = 0;

    if (**ptr == '-') {
        negate = 1;
        (*ptr)++;
    }
    while (**ptr) {
        hex_val = hex(**ptr);
        if (hex_val < 0)
            break;

        *long_val = (*long_val << 4) | hex_val;
        num++;
        (*ptr)++;
    }

    if (negate)
        *long_val = -*long_val;

    return num;
}

/* Write memory due to an 'M' or 'X' packet. */
static int write_mem_msg(int binary)
{
    char *ptr = &remcom_in_buffer[1];
    unsigned long addr;
    unsigned long length;
    int err;

    if (hex2long(&ptr, &addr) > 0 && *(ptr++) == ',' &&
            hex2long(&ptr, &length) > 0 && *(ptr++) == ':') {
        if (binary)
            err = ebin2mem(ptr, (char *)addr, length);
        else
            err = hex2mem(ptr, (char *)addr, length);
        if (err)
            return err;
        return 0;
    }

    return -1;
}

/*
 * Send the packet in buffer.
 * Check for gdb connection if asked for.
 */
static void put_packet(char *buffer)
{
    unsigned char checksum;
    int count;
    char ch;

    /*
     * $<packet info>#<checksum>.
     */
    while (1) {
        gdb_io_ops.write_char('$');
        checksum = 0;
        count = 0;

        while ((ch = buffer[count])) {
            gdb_io_ops.write_char(ch);
            checksum += ch;
            count++;
        }

        gdb_io_ops.write_char('#');
        gdb_io_ops.write_char(tohex((checksum >> 4) & 0xf));
        gdb_io_ops.write_char(tohex(checksum & 0xf));


        /* Now see what we get in reply. */
        ch = gdb_io_ops.read_char();

        /* If we get an ACK, we are done. */
        if (ch == '+')
            return;

        /*
         * If we get the start of another packet, this means
         * that GDB is attempting to reconnect.  We will NAK
         * the packet being sent, and stop trying to send this
         * packet.
         */
        if (ch == '$') {
            gdb_io_ops.write_char('-');
            return;
        }
    }
}

/* scan for the sequence $<data>#<checksum> */
static void get_packet(char *buffer)
{
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    do {
        /*
         * Spin and wait around for the start character, ignore all
         * other characters:
         */
        while ((ch = (gdb_io_ops.read_char())) != '$')
            /* nothing */;

        checksum = 0;
        xmitcsum = -1;

        count = 0;

        /*
         * now, read until a # or end of buffer is found:
         */
        while (count < (BUFMAX - 1)) {
            ch = gdb_io_ops.read_char();
            if (ch == '#')
                break;
            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }
        buffer[count] = 0;

        if (ch == '#') {
            xmitcsum = hex(gdb_io_ops.read_char()) << 4;
            xmitcsum += hex(gdb_io_ops.read_char());

            if (checksum != xmitcsum)
                /* failed checksum */
                gdb_io_ops.write_char('-');
            else
                /* successful transfer */
                gdb_io_ops.write_char('+');
            /* if (flush)*/
            /*flush();*/
        }
    } while (checksum != xmitcsum);
}

static void error_packet(char *pkt, int error)
{
    error = -error;
    pkt[0] = 'E';
    pkt[1] = tohex((error / 10));
    pkt[2] = tohex((error % 10));
    pkt[3] = '\0';
}



static int gdb_arch_set_breakpoint(unsigned long addr, char *saved_instr)
{
    rt_memcpy(saved_instr, (char *)addr, BREAK_INSTR_SIZE);


    rt_memcpy((char *)addr, arch_gdb_ops.gdb_bpt_instr,
            BREAK_INSTR_SIZE);

    return 0;
}

static int gdb_arch_remove_breakpoint(unsigned long addr, char *bundle)
{
    rt_memcpy((char *)addr,
            (char *)bundle, BREAK_INSTR_SIZE);

    return 0;
}

/*
 * SW breakpoint management:
 */
static int gdb_activate_sw_breakpoints(void)
{
    unsigned long addr;
    int error = 0;
    int i;

    for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
        if (gdb_break[i].state != BP_SET)
            continue;

        addr = gdb_break[i].bpt_addr;
        error = gdb_arch_set_breakpoint(addr,
                gdb_break[i].saved_instr);
        if (error)
            return error;

        /*gdb_flush_swbreak_addr(addr);*/
        gdb_break[i].state = BP_ACTIVE;
    }
    return 0;
}

static int gdb_set_sw_break(unsigned long addr)
{
    /* int err = gdb_validate_break_address(addr);*/
    int breakno = -1;
    int i;

    /*if (err)*/
    /*return err;*/

    for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
        if ((gdb_break[i].state == BP_SET) &&
                (gdb_break[i].bpt_addr == addr))
            return -1;
    }
    for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
        if (gdb_break[i].state == BP_REMOVED &&
                gdb_break[i].bpt_addr == addr) {
            breakno = i;
            break;
        }
    }

    if (breakno == -1) {
        for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
            if (gdb_break[i].state == BP_UNDEFINED) {
                breakno = i;
                break;
            }
        }
    }

    if (breakno == -1)
        return -1;

    gdb_break[breakno].state = BP_SET;
    gdb_break[breakno].type = BP_BREAKPOINT;
    gdb_break[breakno].bpt_addr = addr;

    return 0;
}

static int gdb_deactivate_sw_breakpoints(void)
{
    unsigned long addr;
    int error = 0;
    int i;

    for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
        if (gdb_break[i].state != BP_ACTIVE)
            continue;
        addr = gdb_break[i].bpt_addr;
        error = gdb_arch_remove_breakpoint(addr,
                gdb_break[i].saved_instr);
        if (error)
            return error;

        /*gdb_flush_swbreak_addr(addr);*/
        gdb_break[i].state = BP_SET;
    }
    return 0;
}

static int gdb_remove_sw_break(unsigned long addr)
{
    int i;

    for (i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
        if ((gdb_break[i].state == BP_SET) &&
                (gdb_break[i].bpt_addr == addr)) {
            gdb_break[i].state = BP_REMOVED;
            return 0;
        }
    }
    return -1;
}



/* Handle the 'm' memory read bytes */
static void gdb_cmd_memread()
{
    char *ptr = &remcom_in_buffer[1];
    unsigned long length;
    unsigned long addr;
    int err;

    if (hex2long(&ptr, &addr) > 0 && *ptr++ == ',' &&
            hex2long(&ptr, &length) > 0) {
        err = mem2hex((char *)addr, remcom_out_buffer, length);
    }
}

/* Handle the 'M' memory write bytes */
static void gdb_cmd_memwrite()
{
    int err = write_mem_msg(0);

    strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'X' memory binary write bytes */
static void gdb_cmd_binwrite()
{
    int err = write_mem_msg(1);

    strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'q' query packets */
static void gdb_cmd_query()
{

}

/* Handle the '?' status packets */
static void gdb_cmd_status()
{
    /*
     * We know that this packet is only sent
     * during initial connect.  So to be safe,
     * we clear out our breakpoints now in case
     * GDB is reconnecting.
     */
    int sigval = 0x05; 
    /*remove_all_break();*/
    remcom_out_buffer[0] = 'S';
    remcom_out_buffer[1] = tohex((sigval >> 4) &0xf); 
    remcom_out_buffer[2] = tohex(sigval & 0xf); 
    remcom_out_buffer[3] = 0;
}

/* Handle the 'g' or 'p' get registers request */
static void gdb_cmd_getregs()
{
    char len = sizeof(long);

    gdb_get_register((unsigned long *)gdb_regs); 

    /*get one registers*/
    if (remcom_in_buffer[0] == 'p'){
        char *p = &remcom_in_buffer[1];
        unsigned long regno = 0;

        if (hex2long(&p, &regno)){
            mem2hex(gdb_regs + regno * len, remcom_out_buffer, len);
            return;
        } else {
            strcpy(remcom_out_buffer, "INVALID");
            return;
        }
    }

    mem2hex(gdb_regs, remcom_out_buffer, NUMREGBYTES);
}

/* Handle the 'G' or 'P' set registers request */
static void gdb_cmd_setregs()
{
    char len = sizeof(long);


    /*set one registers*/
    if (remcom_in_buffer[0] == 'P'){       
        char *p = &remcom_in_buffer[1];
        unsigned long regno = 0;

        if (hex2long(&p, &regno) && *p++ == '='){
            gdb_get_register((unsigned long *)gdb_regs); 
            hex2mem(p, (char *)(gdb_regs + regno * len), len);
            gdb_put_register(gdb_regs);
            strcpy(remcom_out_buffer, "OK");
            return;
        } else {
            strcpy(remcom_out_buffer, "P01");
            return;
        }

    }

    hex2mem(&remcom_in_buffer[1], (char *)gdb_regs, NUMREGBYTES);

    gdb_put_register(gdb_regs);
    strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'D' or 'k', detach or kill packets */
static void gdb_cmd_detachkill()
{
    int error;

    /* The detach case */
    if (remcom_in_buffer[0] == 'D') {
        /*     error = remove_all_break();*/
        /*if (error < 0) {*/
        /*error_packet(remcom_out_buffer, error);*/
        /*} else {*/
        strcpy(remcom_out_buffer, "OK");
        /*}*/
        put_packet(remcom_out_buffer);
    } else {
        /*
         * Assume the kill case, with no exit code checking,
         * trying to force detach the debugger:
         */
        /*remove_all_break();*/
    }
}
/* Handle the 'R' reboot packets */
static int gdb_cmd_reboot()
{
    /* For now, only honor R0 */
    if (strcmp(remcom_in_buffer, "R0") == 0) {
        rt_kprintf("Executing emergency reboot\n");
        strcpy(remcom_out_buffer, "OK");
        put_packet(remcom_out_buffer);

        rt_hw_cpu_reset();

        return 1;
    }
    return 0;
}

/* Handle the 'z' or 'Z' breakpoint remove or set packets */
static void gdb_cmd_break()
{
    /*
     * Since GDB-5.3, it's been drafted that '0' is a software
     * breakpoint, '1' is a hardware breakpoint, so let's do that.
     */
    char *bpt_type = &remcom_in_buffer[1];
    char *ptr = &remcom_in_buffer[2];
    unsigned long addr;
    unsigned long length;
    int error = 0;

    /*    if (arch_kgdb_ops.set_hw_breakpoint && *bpt_type >= '1') {*/
    /*[> Unsupported <]*/
    /*if (*bpt_type > '4')*/
    /*return;*/
    /*} else {*/
    /*if (*bpt_type != '0' && *bpt_type != '1')*/
    /*[> Unsupported. <]*/
    /*return;*/
    /*}*/

    /*
     * Test if this is a hardware breakpoint, and
     * if we support it:
     */
    if (*bpt_type == '1' /*&& !(arch_kgdb_ops.flags & KGDB_HW_BREAKPOINT)*/)
        /* Unsupported. */
        return;

    if (*(ptr++) != ',') {
        error_packet(remcom_out_buffer, -1);
        return;
    }
    if (!hex2long(&ptr, &addr)) {
        error_packet(remcom_out_buffer, -1);
        return;
    }
    if (*(ptr++) != ',' ||
            !hex2long(&ptr, &length)) {
        error_packet(remcom_out_buffer, -1);
        return;
    }

    if (remcom_in_buffer[0] == 'Z' && *bpt_type == '0')
        error = gdb_set_sw_break(addr);
    else if (remcom_in_buffer[0] == 'z' && *bpt_type == '0')
        error = gdb_remove_sw_break(addr);
    /* else if (remcom_in_buffer[0] == 'Z')*/
    /*error = arch_kgdb_ops.set_hw_breakpoint(addr,*/
    /*(int)length, *bpt_type - '0');*/
    /*else if (remcom_in_buffer[0] == 'z')*/
    /*error = arch_kgdb_ops.remove_hw_breakpoint(addr,*/
    /*(int) length, *bpt_type - '0');*/

    if (error == 0)
        strcpy(remcom_out_buffer, "OK");
    else
        error_packet(remcom_out_buffer, error);
}



static int process_packet()
{
    remcom_out_buffer[0] = 0;
    switch (remcom_in_buffer[0]) {
        case '?':
            gdb_cmd_status();/* gdbserial status */
            break;
        case 'q':/* query command */
            gdb_cmd_query();
            break;
        case 'p':       /* return the value of  a single CPU register */
        case 'g':       /* return the value of the CPU registers */
            gdb_cmd_getregs();
            break;  
        case 'P':       /* set the value of a single CPU registers - return OK */
        case 'G':       /* set the value of the CPU registers - return OK */
            gdb_cmd_setregs();
            break;
        case 'm': /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
            gdb_cmd_memread();
            break;
        case 'X':/* XAA..AA,LLLL: Write LLLL escaped binary bytes at address AA.AA*/
            gdb_cmd_binwrite();
            break;
        case 'M':/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
            gdb_cmd_memwrite();
            break;
        case 'D': /* Debugger detach */
        case 'k': /* Debugger detach via kill */
            gdb_cmd_detachkill();  
            return -1;
        case 'R': /* Reset */
            gdb_cmd_reboot();
            break;
        case 'S':
        case 's': 
        case 'C':/* Exception passing */
        case 'c': /* cAA..AA    Continue at address AA..AA (optional) */
            gdb_activate_sw_breakpoints();
            return -1;
        case 'z':/* Break point remove */
            break;
        case 'Z':/* Break point set */
            break;
        case 'H':/* task related */
            break;
        case 'T':/* Query thread status */
            break;
        case 'b': /* bBB...  Set baud rate to BB... */
            break;
    }

    put_packet(remcom_out_buffer);
    return 0;
}




/*
 * This function does all command procesing for interfacing to gdb.
 */
int gdb_process_exception(int sigval)
{
    int status;

    do {
        get_packet(remcom_in_buffer);
        status = process_packet();
    } while (status == 0);

    if (status < 0)
        return 0;
    else
        return 1;
}
void gdb_handle_exception(int signo, void *regs)
{
    int sigval;
    gdb_set_register(regs);

    gdb_deactivate_sw_breakpoints();

    gdb_connected = 1;
    if (gdb_connected) {
        /* Reply to host that an exception has occurred */
        remcom_out_buffer[0] = 'S';
        remcom_out_buffer[1] = tohex((0x05 >> 4) &0xf); 
        remcom_out_buffer[2] = tohex(0x05 & 0xf); 
        remcom_out_buffer[3] = 0;
        put_packet(remcom_out_buffer);
    }

    while (gdb_process_exception(sigval));

    gdb_arch_handle_exception();

}


