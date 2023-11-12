#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>


#define Machine_Software_Interrupt (11)
#define User_Software_Interrupt (8)
#define IRQ_TIMER (0x80000007)

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  //用于PA3测试，预期会和前面的
//   for(int i=0;i<32;i++){
// 	printf("0x%08x\n",c->gpr[i]);
//   }

 if (user_handler) {
    Event ev = {0};
	printf("=====================\n");
    printf("当前c->mcause：0x%08x\n",c->mcause);
		printf("=====================\n");

    switch (c->mcause) {
      case Machine_Software_Interrupt:
      case User_Software_Interrupt:
        // printf("c->GPR1 = %d \n", c->GPR1);
        if (c->GPR1 == -1){ 
          ev.event = EVENT_YIELD;
        }else {
          ev.event = EVENT_SYSCALL;
        }
        c->mepc += 4;
        break;

      case IRQ_TIMER:
        ev.event = EVENT_IRQ_TIMER;
        break;

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
