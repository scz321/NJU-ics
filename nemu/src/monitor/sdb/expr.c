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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  //默认情况下，后一个枚举值会比前一个枚举值大1，这里设计成256开始也是有讲究的
  TK_NOTYPE = 256, TK_EQ,DIGIT,PARENTHESES

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  //这里可能需要注意先后顺序
  //另一个注意点是注意元字符和语言转义字符的混合使用，不要混淆

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"-",'-'},        
  {"\\*",'*'},
  {"/",'/'},
  {"(0|1|2|3|4|5|6|7|8|9)+",DIGIT},
  {"(.*)",PARENTHESES},
  
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;
  
  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};

static int nr_token __attribute__((used))  = 0;

//这里我略作修改，把static关键字删除以便在sdb.c中调用
bool make_token(char *e) {
  int position = 0;
  int i;

  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */

    //这里的re存储编译后的正则表达式结果，包括了含rules在内的诸多信息
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case '+':
            tokens[nr_token].type='+';
            nr_token++;
            break;
          case '-':
            tokens[nr_token].type='-';
            nr_token++;
            break;
          case TK_NOTYPE:
            tokens[nr_token].type=TK_NOTYPE;
            nr_token++;
            break;
          case TK_EQ:
            tokens[nr_token].type=TK_EQ;
            nr_token++;
            break;
          case '*':
            tokens[nr_token].type='*';
            nr_token++;
            break;
          case '/':
            tokens[nr_token].type='/';
            nr_token++;
            break;
          case DIGIT:
            tokens[nr_token].type = DIGIT;
            int len = substr_len < sizeof(tokens[nr_token].str) - 1 ? substr_len : sizeof(tokens[nr_token].str) - 1;
            strncpy(tokens[nr_token].str, substr_start, len);
            tokens[nr_token].str[len] = '\0';
            nr_token++;
            break;
          case PARENTHESES:
            tokens[nr_token].type=PARENTHESES;
            //这里目前感觉是不需要进行值的记录的，目前记录DIGIT就够了
            nr_token++;
            break;
          default: 
            printf("意外的tokenType！\n");
            break;
        }

        //这里需要break，因为我们识别出了一个token，自然不需要再遍历其他rule
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }

  }
printf("=================\n");
  for(int i=0;i<nr_token;i++){
      printf("make_token函数执行完成，输出tokesType:%d\n",tokens[i].type);
  }
printf("=================\n");
while(getchar()!='\n'){
  printf("输入回车继续执行");
}

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
