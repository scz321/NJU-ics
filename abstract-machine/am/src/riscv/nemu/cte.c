#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  
  for(int i=0;i<32;i++){
	printf("0x%08x\n",c->gpr[i]);
  }
  if (user_handler) {
    Event ev = {0};
	//根据mcause寄存器的值设置中断原因
    switch (c->mcause) {
      default: ev.event = EVENT_ERROR; break;
    }
	

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry

  //用于设置机器模式的中断向量 mtvec，使其指向 __am_asm_trap 函数
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  //按照讲义，我们应该使用内联汇编，设置mstatus寄存器为0x1800
  asm volatile("csrw mstatus, %0" : : "r"(0x1800));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
  asm volatile("li a7, -1; ecall");
}


bool ienabled() {
  return false;
}

void iset(bool enable) {

}
