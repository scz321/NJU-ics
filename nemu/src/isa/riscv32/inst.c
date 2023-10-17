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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_B,TYPE_R,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
//新增
#define immB() do { *imm = (SEXT((BITS(i, 31, 31) << 12) | \
                                   (BITS(i, 7, 7) << 11) | \
                                   (BITS(i, 30, 25) << 5) | \
                                   (BITS(i, 11, 8) << 1), 12)); } while(0)
#define IS_DEBUG_DECODE false

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); if(IS_DEBUG_DECODE) printf("Itype\tImm:0x%x\t\tsrc1:0x%x\t\n",*imm,*src1);break;
    case TYPE_U:                   immU(); if(IS_DEBUG_DECODE) printf("Utype\tImm:0x%x\t\t\n",*imm); break;
    case TYPE_S: src1R(); src2R(); immS(); if(IS_DEBUG_DECODE) printf("Stype\tImm:0x%x\t\tsrc1:0x%x\tsrc2:0x%x\t\n",*imm,*src1,*src2);break;
    //新增
    case TYPE_B: src1R(); src2R(); immB(); if(IS_DEBUG_DECODE) printf("Btype\tImm:0x%x\t\tsrc1:0x%x\tsrc2:0x%x\t\n",*imm,*src1,*src2);break;
    case TYPE_R: src1R(); src2R();        if(IS_DEBUG_DECODE) printf("Rtype\tImm:NULL\t\tsrc1:0x%x\tsrc2:0x%x\t\n",*src1,*src2);break;
  }
}

//辅助函数，用于实现符号位扩展
static word_t sign_extend_20bit(word_t imm20) {
    // 检查 imm20 的第 20 位是否为 1
    if (imm20 & (1 << 19)) {
        // 如果第 20 位为 1, 将高位全部设为 1 以完成符号扩展
        imm20 |= 0xFFF00000;
    }
    // 如果第 20 位为 0, 直接使用 imm20
    else{
      imm20 |= 0x00000000;
    }
    return imm20;
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
  if(IS_DEBUG_DECODE)\
  printf("已经正常执行一条汇编：%s\n",(name));\
}
  INSTPAT_START();
  //值得一提的是，这里同样是按照先后顺序进行遍历的，一旦发生了匹配，就会立刻退出
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", "lui"    , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", "lw"     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", "sw"     , S, Mw(src1 + imm, 4, src2));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", "ebreak" , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  
  //新增
//一个小trick：把li写在addi前面，就能优先被遍历到了hhh，其实意义不是很大，不写也可正常执行，这里是为了加深理解
//以及可能debug的输出会用到hhh
  INSTPAT("??????? ????? 00000 000 ????? 00100 11","li"   ,  I,R(rd)=src1+imm);
//mv
  INSTPAT("0000000 00000 ????? 000 ????? 00100 11","mv"   ,  I,R(rd)=src1+imm);

  INSTPAT("??????? ????? ????? 000 ????? 00100 11","addi"   ,  I,R(rd)=src1+imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11","auipc"  ,  U,R(rd)=(imm)+s->pc);


//jal的一个特例：j（无条件跳转），返回值保存到了$0，最终仍然会被覆盖hhh
  INSTPAT("0000000 00000 00000 000 00000 11011 11","j"   ,  U,imm=sign_extend_20bit(imm);R(rd)=s->pc+4;s->pc+=imm;);

  //这里直接借用Utype似乎就可以取代所谓的Jtype
  // INSTPAT("??????? ????? ????? ??? ????? 11011 11","jal"   ,  U,imm=sign_extend_20bit(imm);
  // R(rd)=s->snpc;s->dnpc=s->pc+imm;printf("扩展后的imm:0x%x\n计算后的dnpc：0x%x\n",imm,s->dnpc););
  // 修改INSTPAT中的imm字段处理
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", "jal", U,
    uint32_t inst=INSTPAT_INST(s);
    imm = ((inst & 0x80000000) >> 11) // 获取第31位并移到imm[20]的位置
        | ((inst & 0x7FE00000) >> 20) // 获取21-30位并移到imm[10:1]的位置
        | ((inst & 0x00100000) >> 9)  // 获取20位并移到imm[11]的位置
        | (inst & 0x000FF000);        // 获取12-19位并置于imm[19:12]的位置
    imm = sign_extend_20bit(imm);     // 进行符号扩展
    R(rd) = s->snpc;                   // 保存返回地址
    s->dnpc = s->pc + imm;             // 计算跳转地址
    printf("扩展后的imm:0x%x\n计算后的dnpc：0x%x\n", imm, s->dnpc);

  
);

  //类似于li，这里ret是jalr的一个特例
  INSTPAT("0000000 00000 00001 000 00000 11001 11","ret"    ,I  ,s->dnpc=src1+0;);
  //那顺便也把jalr也实现了吧：
  INSTPAT("??????? ????? ????? 000 ????? 11001 11","jalr"    ,I  ,R(rd)=s->snpc;s->dnpc=src1+imm;);

  //sw
  //INSTPAT("??????? ????? ????? 010 ????? 01000 11",sw    ,S,);

//branch 家族
      //beqz，x2寄存器设置为0寄存器
 INSTPAT("??????? 00000 ????? 000 ????? 11000 11","beqz"  ,B  ,
    if(src1==src2){
      s->dnpc=s->pc+imm;
    };
 );//你总不能在这里面再用if吧hhh，那有点搞了  //呃呃，事实证明确实是要用if哈哈哈
    
  INSTPAT("??????? ????? ????? 000 ????? 11000 11","beq",B  ,
    if(src1==src2){
      s->dnpc=s->pc+imm;
    };);

//R type
        //sltu
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11","sltu" ,R  ,R(rd) = (src1 < src2) ? 1 : 0;);


  //sltiu扩展指令--seqz
  INSTPAT("0000000 00001 ????? 011 ????? 00100 11","seqz", I, if(src1<imm){R(rd)=1;}else{R(rd)=0;} );
  //sltiu 我们src1和imm定义的就是uint32型，所以这里直接比较即可
  INSTPAT("??????? ????? ????? 011 ????? 00100 11","sltiu", I, if(src1<imm){R(rd)=1;}else{R(rd)=0;} );


//bne 001号B型指令申请出战
  INSTPAT("??????? ????? ????? 001 ????? 11000 11","bne", B,  
  if(src1!=src2){
      s->dnpc=s->pc+imm;
    };);
//sub
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11","sub"  ,S  ,R(rd)=src1-src2;);
//add
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11","add"  ,S  ,R(rd)=src1+src2;);

//xor
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11","xor",  S, R(rd)=(~src1&src2)|(src1&~src2););
//or
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11","or",  S, R(rd)=src1|src2;);

//如果都不匹配，输出当前的指令信息，便于指令系统的扩展
  printf("没有与当前指令匹配的rule，请考虑新增！\n");
  printf("当前指令:%x\n",s->isa.inst.val);
  INSTPAT_END();
  
  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
