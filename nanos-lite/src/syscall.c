#include <common.h>
#include "syscall.h"


// void sys_write(Context *c){
//   // if (c->GPR2 == 1 || c->GPR2 == 2){
//   //   for (int i = 0; i < c->GPR4; ++i){
//   //     putch(*(((char *)c->GPR3) + i));
//   //   }
//   //   c->GPRx = c->GPR4;
//   // }else {  
//   int ret = fs_write(c->GPR2, (void *)c->GPR3, c->GPR4);
//   c->GPRx = ret;
//   // }
// }



void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;//为什么说a7寄存器的值存放系统调用的编号

  switch (a[0]) {

	 case SYS_write:
      //sys_write(c);
      break;

	
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
