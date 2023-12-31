#include <stdint.h>

#ifdef __ISA_NATIVE__
#error can not support ISA=native
#endif

#define SYS_yield 1
extern int _syscall_(int, uintptr_t, uintptr_t, uintptr_t);

void mulprint(){
	int i, j;

    for (i = 1; i <= 9; i++) {
        for (j = 1; j <= 9; j++) {
            printf("%d x %d = %d\t", i, j, i * j);
        }
        printf("\n");
    }
}
int main() {
	
//   return _syscall_(SYS_yield, 0, 0, 0);
	mulprint();
}
