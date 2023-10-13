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

#include "sdb.h"

#define NR_WP 32



//变量被声明为static，那么他们在程序的整个声明周期内都可以被访问
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].preVal=0;
    //wp_pool[i].used=false;
  }

  head = NULL;
  free_ = wp_pool;
}

//add 
WP* new_wp(){
  //int i;
  if(free_==NULL){
    printf("wp数量已经达到上限！\n");
    assert(0);
  }
  //头删法找到空闲列表首项
  WP* newWp=free_;
  free_=free_->next;

  //头插法插入到head列表中
  newWp->next=head;
  head=newWp;
  
  //返回新增的头指针
  return newWp;
}

void free_wp(WP *wp){
  //那现在你就能体会到NO这个属性的作用了

  //首先，进行删除
  for (WP* i = head; i!=NULL&&i->next!=NULL; i=i->next)
  {
    if(i->next->NO==wp->NO){//注意这里的判断条件
      i->next=i->next->next;
      break;
    }
    /* code */
  }
  //然后，进行增加,老样子，头插法
   wp->next=free_;
  free_=wp;
}


void watchpoint_display(){
  printf("NO.\tCondition\tcurrentValue\n");
  WP* cur = head;
  while (cur){
    printf("\e[1;36m%d\e[0m\t\e[0;32m%s\e[0m\t\t%d\n", cur->NO, cur->condition,cur->preVal);
    cur = cur->next;
  }
}


void changeDisplay(){
  WP* cur=head;
  while(cur!=NULL){
    bool success;
    word_t ret=expr(cur->condition,&success);
    if(!success)
      printf("expr执行失败\n");
    if(cur->preVal!=ret)
    {
      printf("监视点NO.%d的expr值发生了变化：\n",cur->NO);
      printf("原来是:%d\t新值:%d\t\n",cur->preVal,ret);
      nemu_state.state=NEMU_STOP;
    }
    cur=cur->next;
  }
}
/* TODO: Implement the functionality of watchpoint */

