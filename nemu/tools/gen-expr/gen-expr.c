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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>



uint32_t choose(uint32_t n) {
    if (n == 0)
        return 0;
    return rand() % n;
}

// this should be enough
#define BUF_LEN 65536
static char buf[BUF_LEN] = {};
static char code_buf[BUF_LEN + 128] = {}; // a little larger than `buf`
//前者存储格式化的表达式，后者存储生成的格式化的c代码

static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int bufPtr=0;
//static bool validFlag=true;
static void gen(char c){
  buf[bufPtr++]=c;
}
static void gen_num(){
  
  buf[bufPtr++]=choose(9)-0+'1';//保证第一个数字存在且非零
  while(1){
    if(choose(2)==0)
      break;
    // int temp=choose(10);
    // if(temp==0&&bufPtr-1>=0&&buf[bufPtr-1]=='/'){
    //   validFlag=false;
    // }
    
    buf[bufPtr++]=choose(10)-0+'0';
  }
}
static void gen_rand_op(){
  switch(choose(4)){
    case 0:
      buf[bufPtr++]='+';break;
    case 1:
      buf[bufPtr++]='-';break;
    case 2:
      buf[bufPtr++]='*';break;
    case 3:
      buf[bufPtr++]='/';break;
    default:
      printf("产生了未知的随机数！\n");break;
  }

}


static void gen_rand_expr() {
  //数组越界问题的简单处理方式hhh
  if(bufPtr>100){
    gen_num();
    return;
  }


  //生成
  switch (choose(6)) {
    case 0: gen_num(); break;
    case 1: gen_num(); break;
    case 2: gen_num(); break;
    case 3: gen('('); gen_rand_expr(); gen(')'); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
  //buf[0] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  //第二个参数将会被识别为循环次数（默认为1）
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    //validFlag=true;
    bufPtr=0;//一开始把这一个忘记了，然后一直以为是
    
    gen_rand_expr(0);
    buf[bufPtr]='\0';
    // if(validFlag==flase){
    //   buf[0]='\0';
    // }
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    //下面这种做法是必要的！这是为了防止编译过程中的输出警告信息混在在我们预期的input文件中
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr -Werror 2> /tmp/.error.txt");
    if (ret != 0) {
      i--;
      continue;
    }
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint32_t result;
    //这里涉及到对除数为0 的处理,ret==-1说明读取结果失败，在本实验背景下仅对应于除数为0
    ret = fscanf(fp, "%u", &result);
    if(ret==-1){
      i--;
      continue;
    }


    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
