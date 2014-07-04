#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

#include "hal_stub.h"
#include "gdb_stub.h"


static rt_device_t gdb_dev = RT_NULL;
static struct rt_serial_device *gdb_serial;

void gdb_uart_putc(char c);
int gdb_uart_getc();


/**
 * if you want to use something instead of the serial,change it 
 */
struct gdb_io	gdb_io_ops = {
    gdb_uart_putc,
    gdb_uart_getc
};



/**
 * @ingroup gdb_stub
 * This function will get GDB stubs started, with a proper environment
 */
void gdb_start()
{
    if (gdb_dev == RT_NULL)
        rt_kprintf("gdb: no gdb_dev found,please set it first\n");
    else
        gdb_breakpoint();
}


/**
 * @ingroup gdb_stub
 *
 * This function sets the input device of gdb_stub.
 *
 * @param device_name the name of new input device.
 */
void gdb_set_device(const char* device_name)
{
    rt_device_t dev = RT_NULL;
    dev = rt_device_find(device_name);
    if(dev == RT_NULL){
        rt_kprintf("gdb: can not find device: %s\n", device_name);
        return;
    }

    /* open this device and set the new device  */
    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK)
    {
        gdb_dev = dev;   
        gdb_serial = (struct rt_serial_device *)gdb_dev;
    }
}


void gdb_uart_putc(char c)
{ 
#ifdef RT_USING_GDB_DEBUG
    rt_kprintf("%c",c);
#endif
    rt_device_write(gdb_dev, 0, &c, 1);
}

/*  polling  */
int gdb_uart_getc()
{
    int ch;

    /*ch = rt_device_read(gdb_dev, 0, &ch, 1);*/
    ch = -1;
    do {
        ch = gdb_serial->ops->getc(gdb_serial);
    } while (ch == -1);
#ifdef RT_USING_GDB_DEBUG
    rt_kprintf("%c",ch);
#endif

    return ch;
}





