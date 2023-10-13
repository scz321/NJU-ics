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
  TK_NOTYPE = 256, TK_EQ,DIGIT,HEX_NUM,REG,DEREF,

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
  //事实证明这里没有必要关注先后顺序
  //另一个注意点是注意元字符和语言转义字符的混合使用，不要混淆

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"-",'-'},        
  {"\\*",'*'},//这里实际识别出*之后的type确定需要进行分类讨论（乘法or解引用），这里默认为解引用
  {"/",'/'},
  {"0(x|X)[0-9A-Fa-f]+",HEX_NUM},//十六进制数,注意这个规则必须放在DIGIT规则的前面
  {"(0|1|2|3|4|5|6|7|8|9)+",DIGIT},
  {"\\(",'('},
  {"\\)",')'},

  //新增几种tokenType
  
  {"\\$[0-9a-z]+",REG},
  
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

static Token tokens[1000] __attribute__((used)) = {};

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

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
         //   i, rules[i].regex, position, substr_len, substr_len, substr_start);

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
          //这里需要区分乘法和解引用
          if(nr_token-1>=0&&tokens[nr_token-1].type==TK_NOTYPE){
            tokens[nr_token].type=DEREF;
            strncpy(tokens[nr_token].str, substr_start+1, substr_len);//注意我这里把*删除了
          }
          else
          {
            tokens[nr_token].type = '*';
          }
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
        case HEX_NUM:
          tokens[nr_token].type=HEX_NUM;
          //tokens[nr_token].str=substr_start+2;赋值会报错，但是下面的复制不会
          strncpy(tokens[nr_token].str, substr_start+2, substr_len);//注意我这里把0x删除了
          nr_token++;
          break;
        case REG:
          tokens[nr_token].type=REG;
          strncpy(tokens[nr_token].str, substr_start+1, substr_len);//注意我这里把$删除了
          nr_token++;
          break;
        default:
          printf("意外的tokenType！\n");
          break;
        }



        // 这里需要break，因为我们识别出了一个token，自然不需要再遍历其他rule
        break;
      }
    }

    //printf("当前字符串：%s\n",e);
   // printf("i:%d,NR_REGEX:%d\n",i,NR_REGEX);
    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  
  
  // printf("=================\n");
  // for (int i = 0; i < nr_token; i++)
  // {
  //   printf("make_token函数执行完成，输出toexprkesStr:%s\n", tokens[i].str);
  // }
  // printf("=================\n");
  // while(getchar()!='\n'){
  //   printf("输入回车继续执行");
  // }

  //printf("执行成功一次，即将返回true\n");
  return true;
} 


bool check_parentheses(int st,int ed);
int eval(int p, int q);

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
void tokensPrint(int p,int q){
  for(int i=p;i<q+1;i++){
    int type=tokens[i].type;
    if(type==DIGIT){
      printf("%s",tokens[i].str);
    }
    else if(type==TK_EQ){
      printf("==");
    }
    else{
      printf("%c",type);
    }
  }
 // printf('\n');
}
extern bool IS_DEBUG_EXPR;

//这里的返回值尤为重要，必须要设计这个函数的返回值为int，
//而不能设计成uint32_t，这是为了和c语言默认的表达式计算行为保持一致
//我们只需要在最外层的返回递归结果之后，显示进行一次类型转换即可
int eval(int p, int q) {
  if(IS_DEBUG_EXPR){
    printf("当前是IS_DEBUG_EXPR为true，进行一次debug输出\n");
  }
  if (IS_DEBUG_EXPR)
    printf("当前st:%d,ed:%d,eval对象为:", p, q);
  if (IS_DEBUG_EXPR)
    tokensPrint(p, q);
  if (IS_DEBUG_EXPR)
    printf("返回情况：");
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
    //现在，这里不仅仅只有num一种case了，需要分情况讨论
    int type=tokens[p].type;
    char *endptr; // 用于存储转换后剩余的未处理部分的地址
    word_t ret=-1;

    switch(type){
      case DIGIT:
        ret=(word_t)strtoul(tokens[p].str,&endptr,10);
        if (IS_DEBUG_EXPR)
          printf("即将返回DIGIT:%d\n", ret);
        return ret;
        break;
      case DEREF:
        word_t* retPtr=NULL;
       
        //注意我们默认解析的地址是10进制格式
        retPtr=(word_t *)strtoul(tokens[p].str,&endptr,10);
        if (IS_DEBUG_EXPR && retPtr != NULL)
        {
          printf("即将返回*retPtr:%u\n", *retPtr);
        }
        else
        {
          printf("retPtr is NULL\n");
        }
        return *retPtr;
        break;
      case HEX_NUM:
        ret=(word_t)strtoul(tokens[p].str,&endptr,16);
         if (IS_DEBUG_EXPR){
          
          printf("即将返回HEX_NUM的处理结果:%u\n", ret);
         }
          
        return ret;
        break;
      case REG:
        bool success=false;
        if(IS_DEBUG_EXPR)
          printf("tokens[p].str:%s",tokens[p].str);
        ret=isa_reg_str2val(tokens[p].str,&success);
        if(!success){
          printf("isa_reg_str2val函数执行失败!\n");
        }
        return ret;
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    if (IS_DEBUG_EXPR)
      printf("判断为标准括号表达式，返回内层\n");
    return eval(p + 1, q - 1);
  }
  else {
    //首先需要寻找主运算符,逻辑不难，从左到右找优先级最低的运算符
    //debug发现，这是不合理的！没有考虑括号对mainOp位置的影响！
    //首先，我们知道，当前的else情况下不可能出现一个(expr)形式的表达式，因此可以放心大胆地skip括号内的
    //所有运算符
    int opPosition=-1;//主运算符位置
    int braketCount=0;//记录当前的左括号、右括号出现的数量差距，当且仅当lPunm=RPnum=0时，该位置有可能成为mainOp


    //由于当前情况下仍然有可能出现整个式子被括号包围的情况，这会导致整个表达式识别不出来mainOp的后果，因此需要作预处理
    //执行这种消除的前提是，第一个括号和最后一个括号是匹配的括号！
    int P=p,Q=q;
    bool flag = false;//一开始，我是不认为你是是被匹配性括号包围的
    if(tokens[P].type=='('&&tokens[Q].type==')'){
      flag=true;//但是目前看来，你是有很大可能被包围的
      for (int i = p + 1; i < q - 1; i++)
      {        
        int type = tokens[i].type;
        if (type == '(')
          braketCount++;
        if (type == ')')
          braketCount--;
        if(braketCount<0){
          flag=false;//但是现在看来，你还是不配
          break;
        }
      }     
    }
     //如果这里发生了匹配性消除，就直接return eval(p-1,q+1)即可
    if (flag)
    {
      if (IS_DEBUG_EXPR)
        printf("发生了匹配性括号消除,执行 return eval(p+1,q-1);\n");
      return eval(p + 1, q - 1);
    }

    braketCount=0;
    for(int i=p;i<=q;i++){
      assert(braketCount>=0);
      int type=tokens[i].type;
      if(type=='(')
        braketCount++;
      if(type==')')
        braketCount--;
      if(braketCount==0&&opLevel(type)==0){
        opPosition=i;
      }
    }


    //如果不存在+-运算：
    if (opPosition == -1)
    {
      for (int i = p; i <= q; i++)
      {
        int type = tokens[i].type;

        assert(braketCount>=0);
        if(type=='(')
        braketCount++;
        if(type==')')
          braketCount--;
        if (braketCount==0&&opLevel(type) == 1)
        {
          opPosition = i;
          //break;八嘎，不能break！最后那一个才是mainOp
        }
      }
    }
    //如果*/也没有
    if(opPosition==-1){
      printf("当前表达式没有运算符，不合理！\n");
      return  -1;
    }
    if(IS_DEBUG_EXPR)
    printf("当前mainOp位置:%d\n",opPosition);

    struct token mainOp=tokens[opPosition];

    int val1 = eval(p, opPosition - 1);
    int val2 = eval(opPosition + 1, q);

    int ret=-1;
    //确实，我目前已经确保了input中不会出现除数为0的情况，但是我似乎没有确保expr里面不会出现这种情况
  
  //if(opPosition>=)
    switch (mainOp.type)
    {
    case '+':
      //printf("即将执行一次运算：%u+%u  ", val1, val2);
      ret = val1 + val2;
      if(IS_DEBUG_EXPR)
      printf("执行了一次运算：%d+%d=%d\n", val1, val2, ret);
      break;
    case '-':
      //printf("即将执行一次运算：%u-%u  ", val1, val2);
      ret = val1 - val2;
      if(IS_DEBUG_EXPR)
      printf("执行了一次运算：%d-%d=%d\n", val1, val2, ret);
      break;
    case '*':
      //printf("即将执行一次运算：%u*%u  ", val1, val2);
      ret = val1 * val2;
      if(IS_DEBUG_EXPR)
      printf("执行了一次运算：%d*%d=%d\n", val1, val2, ret);
      break;
    case '/':
    //printf("当前除法执行情况：");
    if(val2==0)
      printf("!!分母为0！！");
    //else 
      //printf("正常执行！哈哈哈\n");
     // printf("即将执行一次运算：%u/%u  ", val1, val2);
      ret = val1 / val2;
      if(IS_DEBUG_EXPR)
      printf("执行了一次运算：%d/%d=%d\n", val1, val2, ret);
      break;
    default:
      assert(0);
      break;
    }
    if (IS_DEBUG_EXPR)
      printf("即将返回递归求值结果:%d\n", ret);
    return ret;//return即使是int型的ret还是会被转换成word_t
  }
  return -1;
}

bool check_parentheses(int st,int ed){
  if(tokens[st].type!='('){
    return false;
  }
  if(tokens[ed].type!=')'){
    return false;
  }
  st++;
 
 //下面的代码纯属画蛇添足，加了之后反而是错误的
  // while(st!=ed){
  //   if(tokens[st].type=='('||tokens[st].type==')')
  //     return false;
  //   st++;
  // }

  //10.11后记，上面的代码不是画蛇添足，比如对于下面的测试用例：
  //((9285))/(7-((1-(3)+44*2)*254))*99+((7620))，如果仅仅按照上面的两条规则，
  //那么括号的“褪去”将会是不匹配的！
  //之前之所以想删除它，是为什么来着？
    while(st!=ed){
    if(tokens[st].type=='('||tokens[st].type==')')
      return false;
    st++;
  }
  return true;
}