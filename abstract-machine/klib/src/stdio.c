// #include <am.h>
// #include <klib.h>
// #include <klib-macros.h>
// #include <stdarg.h>

// // #if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
// #ifdef __NATIVE_USE_KLIB__
// int printf(const char *fmt, ...) {
// 	//assert(0);
// 	char buffer[1024];
//   va_list arg;
//   va_start (arg, fmt);
  
//   //这里，我一开始的思路是完全按照vsprintf的逻辑进行改写，这说明我还不具备abstraction的思维！
//   int done = vsprintf(buffer, fmt, arg);

//   putstr(buffer);

//   va_end(arg);
//   return done;
// }
                                                                                                            

// int vsprintf(char *out, const char *fmt, va_list ap) {
	
// 	int state=0;//初始化状态机

// 	int idxFmt=0,idxOut=0;//定义两个索引

// 	for(;fmt[idxFmt]!='\0';idxFmt++){
// 		switch(state){
// 			case 0://未检测到%
// 			{
// 				if(fmt[idxFmt]=='%')
// 				{
// 					state=1;
// 					break;
// 				}
// 				else{
// 					out[idxOut++]=fmt[idxFmt];
// 					break;
// 				}
// 			}
// 			case 1://检测到%
// 			{
// 				switch(fmt[idxFmt]){
// 					case 'd':
// 						int num=va_arg(ap,int);
// 						//我们从参数中获取到了这个整数，下一步还需要把它转换成字符串才行
// 						if(num == 0){
// 							out[idxOut++] = '0';
// 							break;
// 						}
// 						int len;
// 						char buf[1024];
// 						//逆序存储
// 						for(len=0;num!=0;len++){
// 							buf[len]=num%10-0+'0';
// 							num=num/10;
// 						}
// 						for(int i=len-1;i>-1;i--){
// 							out[idxOut++]=buf[i];
// 						}
// 						state=0;
// 						break;
// 					case 's':
// 						char* txt = va_arg(ap, char *);
// 						for (int k = 0; txt[k] != '\0'; ++k)
// 							out[idxOut++] = txt[k];
// 						break;
// 						state = 1;
// 						break;
// 					default:
// 						assert(0);
// 				}
// 			}	
				
// 		}
// 	}

// 	out[idxOut]='\0';//别忘了进行完美收官
// 	return idxOut;

//   panic("Not implemented");
// }

// int sprintf(char *out, const char *fmt, ...) {
// //初始化额外参数
// 	va_list args;//创建一个va_list类型的对象
// 	va_start(args,fmt);//调用va_start函数来对args进行初始化：指明其起始位置
// 	//下面我们要做的是对fmt进行遍历，普通字符直接赋值给out指针，%d这种用后面的替换就行
// 	//这里可能会涉及到类型转换的问题
// 	vsprintf(out,fmt,args);//调用vsprint函数进行实际解析
// 	va_end(args);
// 	return 0;
//   //panic("Not implemented");
// }

// int snprintf(char *out, size_t n, const char *fmt, ...) {
//   panic("Not implemented");
// }

// int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
//   panic("Not implemented");
// }

// #endif
#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsprintf(char *out, const char *fmt, va_list ap);
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap);

int printf(const char *fmt, ...) {
  char buffer[2048];
  va_list arg;
  va_start (arg, fmt);
  
  int done = vsprintf(buffer, fmt, arg);

  putstr(buffer);

  va_end(arg);
  return done;
}

static char HEX_CHARACTERS[] = "0123456789ABCDEF";
#define BIT_WIDE_HEX 8

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list valist;
  va_start(valist, fmt);

  int res = vsprintf(out ,fmt, valist);
  va_end(valist);
  return res;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list arg;
  va_start (arg, fmt);

  int done = vsnprintf(out, n, fmt, arg);

  va_end(arg);
  return done;
}

#define append(x) {out[j++]=x; if (j >= n) {break;}}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  char buffer[128];
  char *txt, cha;
  int num, len;
  unsigned int unum;
  uint32_t pointer;
  
  
  int state = 0, i, j;//模仿一个状态机
  for (i = 0, j = 0; fmt[i] != '\0'; ++i){
    switch (state)
    {
    case 0:
      if (fmt[i] != '%'){
        append(fmt[i]);
      } else
        state = 1;
      break;
    
    case 1:
      switch (fmt[i])
      {
      case 's':
        txt = va_arg(ap, char*);
        for (int k = 0; txt[k] !='\0'; ++k)
          append(txt[k]);
        break;
      
      case 'd':
        num = va_arg(ap, int);
        if(num == 0){
          append('0');
          break;
        }
        if (num < 0){
          append('-');
          num = 0 - num;
        }
        for (len = 0; num ; num /= 10, ++len)
          //buffer[len] = num % 10 + '0';//逆序的
          buffer[len] = HEX_CHARACTERS[num % 10];//逆序的
        for (int k = len - 1; k >= 0; --k)
          append(buffer[k]);
        break;
      
      case 'c':
        cha = (char)va_arg(ap, int);
        append(cha);
        break;

      case 'p':
        pointer = va_arg(ap, uint32_t);
        for (len = 0; pointer ; pointer /= 16, ++len)
          buffer[len] = HEX_CHARACTERS[pointer % 16];//逆序的
        for (int k = 0; k < BIT_WIDE_HEX - len; ++k)
          append('0');

        for (int k = len - 1; k >= 0; --k)
          append(buffer[k]);
        break;

      case 'x':
        unum = va_arg(ap, unsigned int);
        if(unum == 0){
          append('0');
          break;
        }
        for (len = 0; unum ; unum >>= 4, ++len)
          buffer[len] = HEX_CHARACTERS[unum & 0xF];//逆序的

        for (int k = len - 1; k >= 0; --k)
          append(buffer[k]);
        break;  

	case 'u':
		unum = va_arg(ap, unsigned int);
		 if(unum == 0){
          append('0');
          break;
        }
		for (len = 0; unum ; unum /= 10, ++len)
          //buffer[len] = num % 10 + '0';//逆序的
          buffer[len] = HEX_CHARACTERS[unum % 10];//逆序的
        for (int k = len - 1; k >= 0; --k)
          append(buffer[k]);
        break;


      default:
        assert(0);
      }
      state = 0;
      break;
      
    }
  }

  out[j] = '\0';
  return j;
}

#endif