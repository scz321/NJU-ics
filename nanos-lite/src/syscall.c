#include <common.h>
#include "syscall.h"



void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;//为什么说a7寄存器的值存放系统调用的编号

  switch (a[0]) {
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
