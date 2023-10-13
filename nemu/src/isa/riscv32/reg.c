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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};


//每一行打印的寄存器数量
#define REGISTERS_PER_LINE 4
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

void isa_reg_display() {
  int length = ARRLEN(regs);
  int i = 0;
  printf("=========debug-寄存器信息=========\n");
  for (i = 0; i < length; i+= REGISTERS_PER_LINE){
    for (int j = i; j < MIN(length, i + REGISTERS_PER_LINE); ++j){
      printf("\e[1;36m%3s:\e[0m %#12x | ", regs[j], cpu.gpr[j]);
    }
    printf("\n");
  }
}

word_t isa_reg_str2val(const char *s, bool *success) {

  //难绷，原来你是提供且仅提供了一个接口
  int length = ARRLEN(regs);

  for(int i=0;i<length;i++){
    if(regs[i]==s)
    {
      *success=true;
      return cpu.gpr[i];
    }
  }
   *success=false;
  return 0;
}
