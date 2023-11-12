/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <elf.h>
//add
#include <memory/paddr.h>
//end
extern void changeDisplay();

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

//add 
IringBuf iring_buf;
mRingArr mring_arr;
mRingNode new_mring_node;
//end

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

#define CONFIG_WATCHPOINT
#ifdef CONFIG_WATCHPOINT//之后当系统启动了相关配置时，才始终执行下面的代码，从而减少不必要的开销
  changeDisplay();

#endif
}
//add
extern void ftraceBufInit();
extern char *getFunName(__uint32_t vaddr);
extern void ftraceBufPrint();
extern bool ftraceBufAdd(uint32_t paddr);
extern bool elfInfoInit(char *fileName);
extern bool get_sh_info();
extern bool initShStrTbl();
extern Elf32_Shdr *findByName(char *name);

extern char* elf_file;
//end

//add
typedef struct SHTbl
{
	int e_shentsize;
	int e_shnum;
	__uint64_t e_shoff;
} SHTbl;
extern SHTbl sh_tbl;
extern Elf32_Shdr *strtbl;
extern Elf32_Shdr *symtbl;
//end

static void exec_once(Decode *s, vaddr_t pc) {
	s->pc = pc;
	s->snpc = pc;
	
//pa3 测试  add
// if(s->pc==0x800006b8){//其实就是csr.mtvec，也就是__am_asm_trap的地址
// 	printf("用于PA3测试，s->pc==0x800006b8\n");
// 	isa_reg_display();
// }
// end
	//add
#ifdef CONFIG_ITRACE
	IringNode newNode;
	newNode.inst=s->isa.inst.val;
	newNode.pc=s->pc;
# endif
	//end

	
	//add
	new_mring_node.read=false;
	new_mring_node.write=false;
	//end	

	isa_exec_once(s);
	cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;//如果发生了跳转，这里的ilen就比较大了
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p,' ', space_len);
  p += space_len;
#define IS_DEBUG_IRING false
//add

	addNode(&newNode,&iring_buf);
	if(IS_DEBUG_IRING)
	{
		printf("已经执行了一次addNode，当前的iringbuf：\n");
		IringBufprint(iring_buf);
	}

//end
#define CONFIG_IRING_TRACE
#ifdef CONFIG_IRING_TRACE

#endif


#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}
extern char* elf_file;
static void execute(uint64_t n) {
  Decode s;

  
  //add--初始化ftrace所需要的所有相关的全局变量
	//下面的初始化的执行前提是启用了-e选项，这可以使用elf_file是否为NULL来进行判断
	if (elf_file != NULL)
	{
		ftraceBufInit();
		if (!elfInfoInit(elf_file))
		{
			assert(0);
		}
		if (!get_sh_info(&sh_tbl))
		{
			assert(0);
		}
		if (!initShStrTbl())
		{
			printf("initStrTbl执行出现问题！\n");
			assert(0);
		}
		strtbl = findByName(".strtab");
		symtbl = findByName(".symtab");
	}
	//end


  mRingArrInit(&mring_arr);
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
	// printf("===========\n");
	// printf("cpu.pc:0x%08x\n",cpu.pc);
	// printf("s.pc:0x%08x\n",s.pc);
	// printf("===========\n\n");

    trace_and_difftest(&s, cpu.pc);
	
    if (nemu_state.state != NEMU_RUNNING) {
		printf("出现了运行中断/执行结束\ninstruction tracer info:\n");
		IringBufprint(iring_buf);
		
		if(new_mring_node.read==true||new_mring_node.write==true){
			printf("memory tracer info:\n");
			mRingArrPrint(&mring_arr);
		}
		//当elf_file不为NULL时，说明当前已经启用了-e选项
		if(elf_file!=NULL){
			printf("函数调用 tracer info:\n");
			ftraceBufPrint();
		}
		break;
	}
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();//乐，原来你也要用这个api，怪不得给我声明好了
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();


	initIringBuf(&iring_buf);

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
