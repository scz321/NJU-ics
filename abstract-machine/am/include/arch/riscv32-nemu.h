
#include <am.h>
#ifndef ARCH_H__
#define ARCH_H__

//这里，我们需要对上下文的结构体进行重新组织。

//最好笑的一集

struct Context {
  // TODO: fix the order of these members to match trap.S
  uint32_t gpr[32];
  uint32_t mcause;
  uint32_t mstatus;
  uint32_t mepc;
  //uintptr_t mepc, mcause, gpr[32], mstatus;
  void *pdir;
};

// "a7", "a0", "a1", "a2", "a0"
#define GPR1 gpr[17] // a7
#define GPR2 gpr[10] //a0
#define GPR3 gpr[11] //a1
#define GPR4 gpr[12] //a2
#define GPRx gpr[10] //a0

#endif
