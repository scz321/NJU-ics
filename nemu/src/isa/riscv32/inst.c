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
#include <elf.h>


//add
#include "../../monitor/ftrace/ftrace.hpp"//可以用相对路径哦
//end



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
    case TYPE_R: src1R(); src2R();         if(IS_DEBUG_DECODE) printf("Rtype\tImm:NULL\t\tsrc1:0x%x\tsrc2:0x%x\t\n",*src1,*src2);break;
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
//辅助函数
void printBinary(uint32_t value) {
    char binary[33];
    binary[32] = '\0'; // Null-terminate the string

    for(int i = 31; i >= 0; i--) {
        binary[i] = (value & 1) ? '1' : '0';
        value >>= 1;
    }

    printf("imm[31-25]\trs2\trs1\tfunct\trd\topcode\t\n");
    int i=0;
    for (; i < 7; i++)
    {
      printf("%c", binary[i]);
    }
    printf("\t\t");
    for (; i < 12; i++)
    {
      printf("%c", binary[i]);
    }
    printf("\t");

    for (; i < 17; i++)
    {
      printf("%c", binary[i]);
    }
    printf("\t");
    for (; i < 20; i++)
      printf("%c", binary[i]);
    printf("\t");

    for (; i < 25; i++)
      printf("%c", binary[i]);
    printf("\t");

    for (; i < 32; i++)
      printf("%c", binary[i]);
    printf("\n");

    //  printf("当前指令: %s\n", binary);
}

//辅助函数，用来实现指令中的编号和具体的控制寄存器之间的映射关系
//返回对应寄存器的指针
uint32_t* get_csr(word_t IMM,word_t* preval){
	if(IMM==0x305)//mtvec的寄存器编号
	{
		printf("当前寄存器编号对应于mtvec\n");
		
		*preval=csr.mtvec;
		return &csr.mtvec;

	}
	else if(IMM==0x342){
		printf("当前寄存器编号对应于mcause\n");
		*preval=csr.mcause;
		return &csr.mcause;
	}
	else if(IMM==0x300){
		printf("当前寄存器编号对应于mstatus\n");
		*preval=csr.mstatus.value;
		//要正确地从联合体中提取 word_t 值，您应该使用 csr.mstatus.value，
		//这样才能访问联合体中的 word_t 成员。
		return &csr.mstatus.value;
	}
	else if(IMM==0x341){
		printf("当前寄存器编号对应于mepc\n");
		*preval=csr.mepc;
		return &csr.mepc;
	}
	else{

		printf("==============ERROR!===========================================\n");
		printf("未检测到与该编号0x%08x:对应的寄存器，请结合反汇编+IMM字段，考虑新增！！\n",IMM);
		printf("==============ERROR!===========================================\n");
		
	}
	return NULL;
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
  if(strcmp("jal",(name))==0||strcmp("jalr",(name))==0||strcmp("jr",(name))==0){ftraceBufAdd(s->dnpc,s->pc,0);}\
  else if(strcmp("ret",(name))==0){ftraceBufAdd(s->dnpc,s->pc,1);}\
}
  INSTPAT_START();
  //值得一提的是，这里同样是按照先后顺序进行遍历的，一旦发生了匹配，就会立刻退出
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", "lui"    , U, R(rd) = imm);

//load家族开始
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", "lw"     , I, R(rd) = Mr(src1 + imm, 4));

  INSTPAT("??????? ????? ????? 000 ????? 00000 11", "lb"     , I, 
    word_t temp = Mr(src1 + imm, 1);
    //进行符号位扩展
    if((temp&0x80)==0x80){
      temp=temp|0xffffff00;
    }
    R(rd)=temp;
    );
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", "lbu"     , I, 
  word_t temp = Mr(src1 + imm, 1);
    //进行0扩展
    temp=temp&0x000000ff;
    R(rd)=temp;
    );

  INSTPAT("??????? ????? ????? 001 ????? 00000 11", "lh"     , I, 
   word_t temp = Mr(src1 + imm, 2);
    //进行符号位扩展
    if((temp&0x8000)==0x8000){
      temp=temp|0xffff0000;
    }
    R(rd)=temp;
    );
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", "lhu"     , I, 
   word_t temp = Mr(src1 + imm, 2);
    //进行0扩展
    temp=temp&0x0000ffff;
    R(rd)=temp;
    );
//load家族结束


//store家族
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", "sw"     , S, Mw(src1 + imm, 4, src2));
  //sh：将x[rs2]的低位2 个字节存入内存地址x[rs1]+sign-extend(offset)。
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", "sh"      ,S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", "sb"      ,S, Mw(src1 + imm, 1, src2));

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
  INSTPAT("0000000 00000 00000 000 00000 11011 11","j"   ,  U,imm=sign_extend_20bit(imm);R(rd)=s->pc+4;s->dnpc=s->pc+imm;);

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
	if(IS_DEBUG_DECODE)
    	printf("扩展后的imm:0x%x\n计算后的dnpc：0x%x\n", imm, s->dnpc);
	);

  //类似于li，这里ret是jalr的一个特例，具体来说，它是jr的一个特例
  INSTPAT("0000000 00000 00001 000 00000 11001 11","ret"    ,I  ,s->dnpc=src1+0;);
  //jr也是jalr的一个特例
	INSTPAT("0000000 00000 ????? 000 00000 11001 11","jr"    ,I  ,s->dnpc=src1+0;);

  //那顺便也把jalr也实现了吧：
  INSTPAT("??????? ????? ????? 000 ????? 11001 11","jalr"    ,I  ,R(rd)=s->snpc;s->dnpc=src1+imm;);

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
       //slt
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11","slt" ,R  ,R(rd) = ((int)src1 < (int)src2) ? 1 : 0;);

  //sltiu扩展指令--seqz
  INSTPAT("0000000 00001 ????? 011 ????? 00100 11","seqz", I, if(src1<imm){R(rd)=1;}else{R(rd)=0;} );
  //sltiu 我们src1和imm定义的就是uint32型，所以这里直接比较即可
  INSTPAT("??????? ????? ????? 011 ????? 00100 11","sltiu", I, 
  //注意这里首先需要进行符号位的相关设置，因为我们默认的immI的宏默认进行符号位扩展

  if(src1<imm){R(rd)=1;}else{R(rd)=0;} 
  
  );
//slti
  INSTPAT("??????? ????? ????? 010 ????? 00100 11","slti" , I, if((int)src1<(int)imm){R(rd)=1;}else{R(rd)=0;});

//bne 001号B型指令申请出战
  INSTPAT("??????? ????? ????? 001 ????? 11000 11","bne", B,  
  if(src1!=src2){
      s->dnpc=s->pc+imm;
    };);

	//最ty的一集
	

//sub
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11","sub"  ,S  ,R(rd)=src1-src2;);
//add
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11","add"  ,S  ,R(rd)=src1+src2;);

//xor
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11","xor",  S, R(rd)=(~src1&src2)|(src1&~src2););
//or
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11","or",  S, R(rd)=src1|src2;);

//srai
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11","srai",  I,
  // 只取imm的低5位用于移动位数
  imm = imm & 0x1F;
  // 执行算术右移，由于src1是32位的，所以我们需要把它先转换为一个带符号的32位整数
  // 然后扩展到64位以保持符号位，然后进行移动
  int32_t src1_signed = (int32_t)src1; // 将源操作数转换为带符号的32位整数
  int64_t src1_extended = (int64_t)src1_signed; // 扩展到64位以保持符号位
  R(rd) = (int32_t)(src1_extended >> imm); // 执行移动并将结果截断回32位
  );
//slli
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11","slli",  I,
  word_t shift=imm;
  R(rd)=(src1<<shift);
  );

//srli
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11","srli",   I,
 word_t shift=(imm & 0x01f);//取低五位，逻辑右移，所以高位使用0填充
 R(rd)=(src1>>shift);
 );

//andi 由于immI这个宏已经对imm进行了符号位扩展，所以就没我们什么事情了hhh
  INSTPAT("??????? ????? ????? 111 ????? 00100 11","andi",   I,
    R(rd)=imm&src1;
  );
//ori
  INSTPAT("??????? ????? ????? 110 ????? 00100 11","ori",   I,R(rd)=imm|src1);
//xori
  INSTPAT("??????? ????? ????? 100 ????? 00100 11","xori",   I,R(rd)=(~imm&src1)|(imm&~src1));

//sll，逻辑左移,<<运算符默认执行逻辑左移
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11","sll",   R,uint16_t shift=src2&0x1f;  R(rd)=src1<<shift);
//and
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11","and",   R,R(rd)=src1&src2);

//bge-B型
  INSTPAT("??????? ????? ????? 101 ????? 11000 11","bge",   B, if((int)src1>=(int)src2){
    s->dnpc=s->pc+imm;
  });
  //bgeu-B型
  INSTPAT("??????? ????? ????? 111 ????? 11000 11","bgeu",   B, if(src1>=src2){
    s->dnpc=s->pc+imm;
  });
//blt
  INSTPAT("??????? ????? ????? 100 ????? 11000 11","blt",   B, if((int)src1<(int)src2){
    s->dnpc=s->pc+imm;
  });
  //bltu
  INSTPAT("??????? ????? ????? 110 ????? 11000 11","bltu",   B, if(src1<src2){
    s->dnpc=s->pc+imm;
  });
  //mul-R
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11","mul",   R,R(rd)=src1*src2;);

//mulhu
	 INSTPAT("0000001 ????? ????? 011 ????? 01100 11","mul",   R,uint64_t temp=(uint64_t)src1*src2;R(rd)=temp>>32);

//rem-R，测试用例你真是什么冷门指令都有啊
INSTPAT("0000001 ????? ????? 110 ????? 01100 11","rem", R, R(rd)=src1%src2;);

//div-R
INSTPAT("0000001 ????? ????? 100 ????? 01100 11","div",R,R(rd)=((int)src1/(int)src2););

//divu-R
INSTPAT("0000001 ????? ????? 101 ????? 01100 11","div",R,R(rd)=src1/src2;);

// //csrrs
// 	INSTPAT("??????? ????? ????? 010 ?????  ")

//mulh 这里需要注意，src1中的bits我们默认按照uint32_t的形式进行解读，需要先进行一步int32_t
INSTPAT("0000001 ????? ????? 001 ????? 01100 11","mulh",   R,int64_t ret = (int64_t)(int32_t)src1 * (int32_t)src2; R(rd) = ret >> 32);
// remu
INSTPAT("0000001 ????? ????? 111 ????? 01100 11", "remu", R,
  // 需要确保src2不为0，因为除数为0是未定义的行为
  if (src2 == 0) {
    R(rd) = src1; // 如果src2为0，则通常余数就是被除数本身
  } else {
    // 计算无符号余数
    R(rd) = (uint32_t)src1 % (uint32_t)src2;
  }
);

//会且仅会一点qwq

 // sra
INSTPAT("0100000 ????? ????? 101 ????? 01100 11","sra",  R,
  uint32_t shift_amount = src2 & 0x1F; // 只需要低5位来确定移位数，因为最大移位数为31
  int32_t signed_val = (int32_t)src1; // 保证我们以有符号数进行移位操作
  R(rd) = signed_val >> shift_amount; // 执行算术右移操作
);
// srl
INSTPAT("0000000 ????? ????? 101 ????? 01100 11","srl", R,
  uint32_t shift_amount = src2 & 0x1F; // 只需要低5位来确定移位数，因为最大移位数为31
  uint32_t value_to_shift = (uint32_t)src1; // 确保我们使用无符号数进行操作
  R(rd) = value_to_shift >> shift_amount; // 执行逻辑右移操作
);




//=====================pa 3 add================
//ecall

//这里后续应该需要改进。主要是NO参数的传递。
INSTPAT("0000000 00000 00000 000 00000 11100 11","ecall",	I,
//这个NO参数，用来赋值给当前的控制状态寄存器,cause，那么它怎么传呢？，显然不是来自ecall指令的某个字段的，
if(R(17)==-1)
	s->dnpc=isa_raise_intr(11,s->pc);
else
	s->dnpc=isa_raise_intr(8,s->pc);
printf("执行了一次ecall，s->dnpc：0x%08x\n",s->dnpc);
);

//csrrw伪指令--csrw
INSTPAT("??????? ????? ????? 001 00000 11100 11","csrw",	I,
	word_t preval=-1;
	uint32_t* temp_ptr=get_csr(imm,&preval);
	*temp_ptr=src1;
	
	printf("目前csr寄存器将会被赋值为：0x%08x\n",src1);
	R(rd)=preval;
);

//csrrw
INSTPAT("??????? ????? ????? 001 ????? 11100 11","csrrw",	I,
	
	word_t preval=-1;
	uint32_t* temp_ptr=get_csr(imm,&preval);
	*temp_ptr=src1;
	
	printf("目前csr寄存器将会被赋值为：0x%08x\n",src1);
	R(rd)=preval;
);

//csrrs伪指令--csrr
//csrrs

INSTPAT("??????? ????? 00000 010 ????? 11100 11","csrr",	I,
	word_t preval=-1;
	uint32_t* temp_ptr=get_csr(imm,&preval);
	*temp_ptr=preval|src1;
	printf("目前csr寄存器将会被赋值为：0x%08x\n",preval|src1);
	R(rd)=preval;
);



//csrrs

INSTPAT("??????? ????? ????? 010 ????? 11100 11","csrrs",	I,
	word_t preval=-1;
	uint32_t* temp_ptr=get_csr(imm,&preval);
	*temp_ptr=preval|src1;
	printf("目前csr寄存器将会被赋值为：0x%08x\n",preval|src1);
	R(rd)=preval;
);

//mret
INSTPAT("0011000 00010 00000 000 00000 11100 11","mret", R,s->dnpc=csr.mepc);



//-------------------------------------分界线-------------------------------

//如果都不匹配，输出当前的指令信息，便于指令系统的扩展
  printf("没有与当前指令匹配的rule，请考虑新增！\n");
   printBinary(s->isa.inst.val);
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", "inv"    , N, INV(s->pc));
 



  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  
  //啊这个时候就别问这个不着边际的问题了

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
