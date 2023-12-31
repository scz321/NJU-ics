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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

//#include <utils.h>

//控制是否debug输出的static全局变量
bool IS_DEBUG_EXPR=false;

//end



static int is_batch_mode = false;




void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args){
  //输入的是字符串，需要先转换成int
  int n=1;
  if(args!=NULL){
      sscanf(args,"%d",&n);
  }
  cpu_exec(n);
  return 0;
}


static int cmd_info(char *args){
  if(args==NULL){
    printf("缺少参数！\n");
  }
  else if(strcmp(args,"r")==0){
    isa_reg_display();
  }
  else if(strcmp(args,"w")==0){
    watchpoint_display();
  }
  else{
    printf("参数有误！\n");
  }
  return 0;
}




static int cmd_p(char *args){
  if(args==NULL){
    printf("缺少表达式！\n");
    return -1;
  }
  bool ret;
  int expr_ret=-1;
  expr_ret=expr(args,&ret);
  if(ret==false){
    printf("make_token执行失败！\n");
  }
  printf("expr计算结果：%u\n",expr_ret);
  return 0;
}



static int cmd_w(char* args)
{
   if(args==NULL){
    printf("缺少表达式！\n");
    return -1;
  }
  //首先，申请一个空的wp
  WP* nWp=new_wp();
  
  //然后，使用args来初始化该wp
  //nWp->condition=args;错误方式
  strcpy(nWp->condition,args);
  bool success;
  nWp->preVal=expr(args,&success);
  return 0;
}

// static int cmd_x(char *args)
// {
//   int numBytes=0;
//   if(args==NULL)
//     numBytes=4;
//   else if()
// }
 extern uint8_t* guest_to_host(paddr_t paddr);
static int cmd_x(char *args){

  if(IS_DEBUG_EXPR)
    printf("=======cmd_x  start=============\n");
  char *arg = strtok(args," ");//我们默认输入一个参数第一个参数为起始地址，第二个参数为字节数量,默认为4
   if(IS_DEBUG_EXPR)
    printf("=======t1=============\n");
  char *bytesNum=strtok(NULL," \n");
   if(IS_DEBUG_EXPR)
     printf("=======t2=============\n");
   int n = 4;
   bool success = true;
   paddr_t base = 0x80000000;
   if (IS_DEBUG_EXPR)
     printf("bytesNum:%s\n", bytesNum);
   if (bytesNum != NULL)
     sscanf(bytesNum, "%d", &n);
   if (IS_DEBUG_EXPR)
   {
     printf("当前的预期输出的字节数为：%d\n", n);
   }
  base = expr(arg, &success);
  if (!success)
  {
    return 0;
  }
  if(IS_DEBUG_EXPR){
      printf("当前的预期的地址为：0x%x\n",base);

  }
  //arg = args + strlen(arg) + 1;
  //sscanf(arg, "%i", &base);

  
  int lines=n/4;
  if(lines==0){
    printf("cmd_至少要求输出4个字节！\n");
  }
  for (int i = 0; i < lines; ++i){
      printf ("\n\e[1;36m%#x: \e[0m\t", base + i * 4);
    for (int j = 0; j < 4; ++j){
      uint8_t* pos = guest_to_host(base + i * 4 + j);
      printf("%.2x ", *pos);
    }
    printf("\t");
  }
  printf("\n");
  return 0;
}



static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {


  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","执行指定数量的指令，缺省为1",cmd_si},
  {"info","打印寄存器/监视点信息",cmd_info},
  {"p","表达式求值，这里的表达式支持寄存器",cmd_p},
  {"w","增加一个watchPoint",cmd_w},
  {"x","以16进制输出指定地址的指定字节数量的内存信息，默认为4bytes",cmd_x},
  
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
#ifdef __ENABLE_B_OPTION
  printf("__ENABLE_B_OPTION is defined!!\n");
  is_batch_mode = true;
#else
  printf("__ENABLE_B_OPTION is not defined!!\n");
#endif
  //is_batch_mode=true;
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }
#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif
    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }
    //这部分用于支持watchpoint的实现：
        //遍历head链表中的所有wp，输出preVal发生变化的wp并且更新之
    //changeDisplay();不应该在这里执行检查
    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
