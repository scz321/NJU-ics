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

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"

#define IS_DEBUGGING false


bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{

	for (int i = 0; i < 32; ++i)
	{
		if(IS_DEBUGGING)

		if (ref_r->gpr[i] != cpu.gpr[i]){
			printf("出现了异常寄存器值：\n	cpu.gpr[i]:0x%08x\t,ref_r->gpr[i]:0x%08x\n", cpu.gpr[i], ref_r->gpr[i]);
			return false;

		}
			
	}
	
	if (ref_r->pc == pc)
	{
		return true;
	}
	else
	{
		if(IS_DEBUGGING)
			printf("pc值不同！\n");
		if(IS_DEBUGGING)
		printf("ref_r->pc:0x%08x\t,pc:0x%08x\n", ref_r->pc, pc);
		Log("%x\t%x", ref_r->pc, pc);
		return false;
	}
}

void isa_difftest_attach() {

}
