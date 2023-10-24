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

#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#define PMEM_LEFT  ((paddr_t)CONFIG_MBASE)
#define PMEM_RIGHT ((paddr_t)CONFIG_MBASE + CONFIG_MSIZE - 1)
#define RESET_VECTOR (PMEM_LEFT + CONFIG_PC_RESET_OFFSET)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

static inline bool in_pmem(paddr_t addr) {
  return addr - CONFIG_MBASE < CONFIG_MSIZE;
}

word_t paddr_read(paddr_t addr, int len);

void paddr_write(paddr_t addr, int len, word_t data);


//add
//新增一个用来存储访存信息的结构体
typedef struct mRingNode{
	bool read;
	bool write;
	paddr_t addr;
	int len;
	//读/写所需记录的信息相同，记录要写入和要读出的数据对于debug是没什么意义的
}mRingNode;

#define M_RING_ARR_MAX 1200//注意它和maxLen的区别
typedef struct mRingArr
{
	int maxLen;
	int len;
	mRingNode mring_arr[M_RING_ARR_MAX];
	int st;
	int ed;
}mRingArr;

void mRingArrPrint(mRingArr* arr);
void mRingArrAdd(mRingArr* arr,mRingNode newNode);
void mRingArrInit(mRingArr* arr);
#endif
