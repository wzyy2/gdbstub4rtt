#ifndef __HAL_STUB_H__
#define __HAL_STUB_H__


int gdb_putc(char c);
int gdb_getc();
void gdb_start();
void gdb_set_device(const char* device_name);

#endif /* __HAL_STUB_H__ */
