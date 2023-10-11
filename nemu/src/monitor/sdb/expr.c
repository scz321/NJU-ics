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
  TK_NOTYPE = 256, TK_EQ,DIGIT

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
  {"\\(",'('},
  {"\\)",')'}
  
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

// 这里我略作修改，把static关键字删除以便在sdb.c中调用
bool make_token(char *e)
{

  int position = 0;
  int i;

  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */

    // 这里的re存储编译后的正则表达式结果，包括了含rules在内的诸多信息
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case '+':
          tokens[nr_token].type = '+';
          nr_token++;
          break;
        case '-':
          tokens[nr_token].type = '-';
          nr_token++;
          break;
        case TK_NOTYPE:
          tokens[nr_token].type = TK_NOTYPE;
          nr_token++;
          break;
        case TK_EQ:
          tokens[nr_token].type = TK_EQ;
          nr_token++;
          break;
        case '*':
          tokens[nr_token].type = '*';
          nr_token++;
          break;
        case '/':
          tokens[nr_token].type = '/';
          nr_token++;
          break;
        case DIGIT:
          tokens[nr_token].type = DIGIT;
          int len = substr_len < sizeof(tokens[nr_token].str) - 1 ? substr_len : sizeof(tokens[nr_token].str) - 1;
          strncpy(tokens[nr_token].str, substr_start, len);
          tokens[nr_token].str[len] = '\0';
          nr_token++;
          break;
        // 这里需要注意的是，括号的匹配是在“语法分析”中进行的工作，而非词法分析
        case '(':
          tokens[nr_token].type = '(';
          // 这里目前感觉是不需要进行值的记录的，目前记录DIGIT就够了
          // 楼上有点蠢，你不记录，后续怎么知道括号表达式中有什么呢？要知道，词法分析的终极目标就是
          // 解析出所有符合条件的最小词法单元

          //
          nr_token++;
          break;
        case ')':
          tokens[nr_token++].type = ')';
          break;
        default:
          printf("意外的tokenType！\n");
          break;
        }

        // 这里需要break，因为我们识别出了一个token，自然不需要再遍历其他rule
        break;
      }
    }

    printf("当前字符串：%s\n",e);
    printf("i:%d,NR_REGEX:%d\n",i,NR_REGEX);
    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  
  
  printf("=================\n");
  for (int i = 0; i < nr_token; i++)
  {
    printf("make_token函数执行完成，输出tokesStr:%s\n", tokens[i].str);
  }
  printf("=================\n");
  // while(getchar()!='\n'){
  //   printf("输入回车继续执行");
  // }

  printf("执行成功一次，即将返回true\n");
  return true;
}
bool check_parentheses(int st,int ed);
word_t eval(int p, int q);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    // bool test=make_token(e);
    // printf("make_token的执行结果是:%d\n",test);
    *success = false;
    return 0;
  }
  *success=true;
  //printf("+++++++++test++++++++++++\n");
  return eval(0,nr_token-1);

  /* TODO: Insert codes to evaluate the expression. */
  //下面开始伟大的递归求值!


  
}

//返回0，对应+-的优先级，1对应*/
int opLevel(int type){
  if(type=='+'||type=='-')
    return 0;
  if(type=='*'||type=='/')
    return 1;
  return -1;
}

word_t eval(int p, int q) {
  printf("当st:%d,ed:%d\n",p,q);
  if (p > q) {
    /* Bad expression */
    printf("Bad expression!!\n");
    return -1;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    word_t ret=-1;
    char *endptr; // 用于存储转换后剩余的未处理部分的地址
    ret=(word_t)strtoul(tokens[p].str,&endptr,10);
    printf("  即将返回DIGIT:%d\n",ret);
    return ret;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    //首先需要寻找主运算符,逻辑不难，从左到右找优先级最低的运算符
    //debug发现，这是不合理的！没有考虑括号对mainOp位置的影响！
    //首先，我们知道，当前的else情况下不可能出现一个(expr)形式的表达式，因此可以放心大胆地skip括号内的
    //所有运算符
    int opPosition=-1;//主运算符位置
    int braketCount=0;//记录当前的左括号、右括号出现的数量差距，当且仅当lPunm=RPnum=0时，该位置有可能成为mainOp

    for(int i=p;i<q;i++){
      assert(braketCount>=0);
      int type=tokens[i].type;
      if(type=='(')
        braketCount++;
      if(type==')')
        braketCount--;
      if(braketCount==0&&opLevel(type)==0){
        opPosition=i;
        break;
      }
    }
    //如果不存在+-运算：
    if (opPosition == -1)
    {
      for (int i = p; i < q; i++)
      {
        int type = tokens[i].type;
        assert(opPosition>=0);
        if(type=='(')
        braketCount++;
        if(type==')')
          braketCount--;
        
        if (braketCount==0&&opLevel(type) == 1)
        {
          opPosition = i;
          break;
        }
      }
    }
    //如果*/也没有
    if(opPosition==-1){
      printf("  当前表达式没有运算符，不合理！\n");
      return  -1;
    }
    printf("  当前mainOp位置:%d\n",opPosition);

    struct token mainOp=tokens[opPosition];


    word_t val1 = eval(p, opPosition - 1);
    word_t val2 = eval(opPosition + 1, q);

    int ret=-1;
    switch (mainOp.type) {
      case '+': ret= val1 + val2;break;
      case '-': ret= val1 - val2;break;
      case '*': ret= val1 * val2;break;
      case '/': ret= val1 / val2;break;
      default: assert(0);break;
    }
    printf("  即将返回递归求值结果:%d\n",ret);
    return ret;
  }
}

bool check_parentheses(int st,int ed){
  if(tokens[st].type!='('){
    return false;
  }
  if(tokens[ed].type!=')'){
    return false;
  }
  st++;
 
  while(st!=ed){
    if(tokens[st].type=='('||tokens[st].type==')')
      return false;
    st++;
  }
  return true;
}