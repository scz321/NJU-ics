#include <common.h>


// An event of type @event, caused by @cause of pointer @ref
// typedef struct {
//   enum {
//     EVENT_NULL = 0,
//     EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
//     EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
//   } event;
//   uintptr_t cause, ref;
//   const char *msg;
// } Event;


//这个函数会用来初始化am中的user_handler，也即事件分发函数/事件处理回调函数
//
static Context* do_event(Event e, Context* c) {
	printf("=======开始执行handler函数=========\n");
		printf("=======开始执行handler函数=========\n");
	printf("=======开始执行handler函数=========\n");
	printf("=======开始执行handler函数=========\n");
	printf("=======开始执行handler函数=========\n");
	printf("=======开始执行handler函数=========\n");
	printf("=======开始执行handler函数=========\n");


  switch (e.event) {
	case EVENT_NULL:
		printf("当前中断事件类型为EVENT_NULL(%d)\n",EVENT_NULL);
		break;
	case EVENT_YIELD:
		printf("当前中断事件类型为EVENT_YIELD(%d)\n",EVENT_YIELD);
		break;
	case EVENT_SYSCALL:
		printf("当前中断事件类型为EVENT_SYSCALL(%d)\n",EVENT_SYSCALL);
		break;
	case EVENT_PAGEFAULT:
		printf("当前中断事件类型为EVENT_PAGEFAULT(%d)\n",EVENT_PAGEFAULT);
		break;
	case EVENT_ERROR:
		printf("当前中断事件类型为EVENT_ERROR(%d)\n",EVENT_ERROR);
		break;
	case EVENT_IRQ_TIMER:
		printf("当前中断事件类型为EVENT_IRQ_TIMER(%d)\n",EVENT_IRQ_TIMER);
		break;
	case EVENT_IRQ_IODEV:
		printf("当前中断事件类型为EVENT_IRQ_IODEV(%d)\n",EVENT_IRQ_IODEV);
		break;

    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
