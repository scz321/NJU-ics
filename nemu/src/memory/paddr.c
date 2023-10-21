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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
//声明为全局变量，生命周期贯穿整个程序
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif


#ifdef CONFIG_MEM_RANDOM
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int) (CONFIG_MSIZE / sizeof(p[0])); i ++) {
    p[i] = rand();
  }
#endif
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

//add
extern mRingNode new_mring_node;
extern mRingArr mring_arr;
#define IS_DEBUG_M_TRACE false
//end


word_t paddr_read(paddr_t addr, int len) {
	new_mring_node.read=true;
	new_mring_node.addr=addr;
	new_mring_node.len=len;
	mRingArrAdd(&mring_arr,new_mring_node);
	if(IS_DEBUG_M_TRACE){
		printf("执行了一次read\n");
		mRingArrPrint(&mring_arr);
	}
	if (likely(in_pmem(addr))) return pmem_read(addr, len);
	IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
	out_of_bound(addr);
	return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
	new_mring_node.write=true;
	new_mring_node.addr=addr;
	new_mring_node.len=len;
	mRingArrAdd(&mring_arr,new_mring_node);
	
	if(IS_DEBUG_M_TRACE){
		printf("执行了一次write\n");
		mRingArrPrint(&mring_arr);
	}
	
	if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
	IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
	out_of_bound(addr);
}



 void mRingArrAdd(mRingArr* arr,mRingNode newNode){
	if(arr->ed+1>M_RING_ARR_MAX) assert(0);
	arr->mring_arr[arr->ed+1]=newNode;
	arr->len++;
	arr->ed++;
	
	if(arr->len>arr->maxLen){
		arr->st++;
		arr->len--;//这就很容易忘了。。。
		//呃呃，还是一个思维的严谨性的问题
	}
}

 void mRingArrPrint(mRingArr* arr){
	printf("当前len:%d\n",arr->len);
	for(int i=arr->st;i<arr->ed+1;i++){
		mRingNode temp=arr->mring_arr[i];
		if(temp.read==1){
			printf("read:\t");
		}
		if(temp.write==1){
			printf("write:\t");
		}
		printf("paddr:0x%08x\tlen:%d\n",temp.addr,temp.len);
	}
	return;
}
 void mRingArrInit(mRingArr* arr){
	arr->ed=0;
	arr->st=0;
	arr->len=0;
	arr->maxLen=12;
}