#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}


int vsprintf(char *out, const char *fmt, va_list ap) {
	
	int state=0;//初始化状态机

	int idxFmt=0,idxOut=0;//定义两个索引

	for(;fmt[idxFmt]!='\0';idxFmt++){
		switch(state){
			case 0://未检测到%
			{
				if(fmt[idxFmt]=='%')
				{
					state=1;
					break;
				}
				else{
					out[idxOut++]=fmt[idxFmt];
					break;
				}
			}
			case 1://检测到%
			{
				switch(fmt[idxFmt]){
					case 'd':
						int num=va_arg(ap,int);
						//我们从参数中获取到了这个整数，下一步还需要把它转换成字符串才行
						if(num == 0){
							out[idxOut++] = '0';
							break;
						}
						int len;
						char buf[1024];
						//逆序存储
						for(len=0;num!=0;len++){
							buf[len]=num%10-0+'0';
							num=num/10;
						}
						for(int i=len-1;i>-1;i--){
							out[idxOut++]=buf[i];
						}
						state=0;
						break;
					case 's':
						char* txt = va_arg(ap, char *);
						for (int k = 0; txt[k] != '\0'; ++k)
							out[idxOut++] = txt[k];
						break;
						state = 1;
						break;
					default:
						assert(0);
				}
			}	
				
		}
	}

	out[idxOut]='\0';//别忘了进行完美收官
	return idxOut;

  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
//初始化额外参数
	va_list args;//创建一个va_list类型的对象
	va_start(args,fmt);//调用va_start函数来对args进行初始化：指明其起始位置
	//下面我们要做的是对fmt进行遍历，普通字符直接赋值给out指针，%d这种用后面的替换就行
	//这里可能会涉及到类型转换的问题
	vsprintf(out,fmt,args);//调用vsprint函数进行实际解析
	va_end(args);
	return 0;
  //panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
