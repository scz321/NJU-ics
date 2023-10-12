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
  else if(strcmp(args,"w")){
    //watchpoint_display();
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
  printf("expr计算结果：%d\n",expr_ret);
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
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  //测试表达式求值start
  FILE *file = fopen("./input.txt", "r");  // 打开文本文件

  if (file == NULL) {
      perror("Unable to open file");
      return ;
  }
   char line[1000];  // 适当选择行缓冲区大小
    uint32_t uintVal;
    char strVal[100];
    int testNum=0;
    int succesNum=0;
    while (fgets(line, sizeof(line), file) != NULL) {
      testNum++;
        // 解析每行数据
        if (sscanf(line, "%u %s", &uintVal, strVal) == 2) {
            // 处理解析得到的数据
            //printf("Read uint32_t: %u, Read string: %s  ", uintVal, strVal);
            bool t;
            uint32_t ret=expr(strVal,&t);
            if(t==false){
              //printf("expr函数执行失败！\n");
            }
            if(ret==uintVal){
              succesNum++;
              //printf("expr()执行结果：%u,测试成功！\n",ret);
            }
            else{
              printf("=====================================\n");
              printf("Read uint32_t: %u, Read string: %s  ", uintVal, strVal);
              IS_DEBUG_EXPR=true;
              expr(strVal,&t);
              
              printf("expr()执行结果：%u,测试失败！\n", ret);
              printf("=====================================\n");

              // printf("expr()执行结果：%u,测试失败！\n",ret);
            }
        } else {
            printf("Invalid format in line: %s\n", line);
        }
        IS_DEBUG_EXPR=false;//注意需要在合适的位置重置IS_DEBUG_EXPR为false
    }
    printf("测试用例数量：%d\n",testNum);
    printf("测试成功数量：%d\n",succesNum);

  //测试表达式求值end


  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

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

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
