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
#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__
#define IringBuf_MAX_LEN 12



#include <isa.h>

typedef struct Decode {
  vaddr_t pc;
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  ISADecodeInfo isa;
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;


typedef struct IringNode{
	//word_t inst;//你确定这里只存inst就够了吗？
	//主打一个自由好吧，哥们只需要pc+inst即可(墨镜可以摘了🤣🤣🤣😎😎😎😭😭😭)
	//先把最基础的实现，后面再一步一步迭代
	word_t pc;
	word_t inst;
	struct IringNode* next;
}IringNode;




//add 指令环形缓冲队列
typedef struct IringBuf{
	// uint16_t len;
	// uint16_t maxLen;
	// IringNode* head;
	// IringNode* tail;//指向最后一个有效的node

	//实力有限，还是老老实实用数组实现吧
	uint16_t len;
	uint16_t maxLen;
	IringNode buf[1024];
	int st;
	int ed;
} IringBuf;
extern IringBuf iring_buf;
inline void IringBufprint(IringBuf iring_buf){
	for(int i=iring_buf.st;i<iring_buf.ed+1;i++){
		printf("%08x:\t%08x\n",iring_buf.buf[i].pc,iring_buf.buf[i].inst);
	}
}

inline bool addNode(IringNode *newNode, IringBuf *iring_buf){
	if(iring_buf->ed+1>1024){
		assert(0);
	}
	iring_buf->buf[iring_buf->ed+1]=*newNode;
	iring_buf->ed++;
	iring_buf->st++;

	return true;
}

inline void initIringBuf(IringBuf* iring_buf){
	iring_buf->st=0;
	iring_buf->ed=0;
	iring_buf->len=0;
	iring_buf->maxLen=1024;

}

// inline void IringBufprint(IringBuf iring_buf){
// printf("当前head指针：%p, 当前tail指针:%p\n", (void*)iring_buf.head, (void*)iring_buf.tail);
// 	IringNode* cur=iring_buf.head->next;//因为第一个是dummyNode，所以pass掉
// 	//这里还是有隐患，当buf满了之后，第一个可就不再是dummy了！
// 	//呃呃，不过这其实不算是隐患，因为它本来就不在预期的len范围之内

// 	for(int i=0;i<iring_buf.len;i++){
// 		if(cur==NULL) assert(0);
// 		printf("%08x:\t%08x\n",cur->pc,cur->inst);
// 		cur=cur->next;
// 	};
// }
// inline bool addNode(IringNode *newNode, IringBuf *iring_buf)
// {
// 			printf("=================================start=====================\n");

// 	printf("执行前：\n");
// 	IringBufprint(*iring_buf);

// 	printf("new nodePc to add:0x%08x\n", newNode->pc);
// 	if (iring_buf->maxLen > iring_buf->len)
// 	{
// 		// 直接加到队列尾部
// 		// IringNode* temp=iring_buf->tail;
// 		iring_buf->tail->next = newNode;
// 		// 更新其他属性
// 		iring_buf->len++;
// 		iring_buf->tail = newNode;
// 	}
// 	else
// 	{
// 		// 先删除head
// 		if (iring_buf->head == NULL)
// 			assert(0);
// 		iring_buf->head = iring_buf->head->next;
// 		iring_buf->len--;

// 		iring_buf->tail->next = newNode;
// 		iring_buf->tail = newNode;
// 		iring_buf->len++;
// 	}
// 	// IringBufprint(*iring_buf);
// 	printf("执行后：\n");
// 	IringBufprint(*iring_buf);

// 		printf("=================================end=====================\n\n\n");

// 	return true;
// }

// inline IringBuf initIringBuf()
// {
// 	IringBuf ret;
// 	ret.maxLen = IringBuf_MAX_LEN;
// 	ret.len = 0;
// 	// IringNode dummyNode;可以的，精准踩坑
// 	IringNode *dummyNode = (IringNode *)malloc(sizeof(IringNode));
// 	ret.head = dummyNode;
// 	ret.tail = ret.head;
// 	return ret;
// };
// 在执行之前，初始化iringbuf

// --- pattern matching mechanism ---
__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}


// --- pattern matching wrappers for decode ---
#define INSTPAT(pattern, ...) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { \
    INSTPAT_MATCH(s, ##__VA_ARGS__); \
    goto *(__instpat_end); \
  } \
} while (0)

#define INSTPAT_START(name) { const void ** __instpat_end = &&concat(__instpat_end_, name);
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; }

#endif


