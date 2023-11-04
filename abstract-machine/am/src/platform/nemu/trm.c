#include <am.h>
#include <nemu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {
	//OUT系列函数中，第一个参数是目标地址，第二个是数据地址
  outb(SERIAL_PORT, ch);
}

void halt(int code) {
  nemu_trap(code);
  
  // should not reach here
  while (1);
}


void _trm_init() {

  int ret = main(mainargs);
  halt(ret);
}
