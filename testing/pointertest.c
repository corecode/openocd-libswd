#include <stdlib.h>
#include <stdio.h>

int f2(int *pf21, int *pf22){
 int *stuff;
 stuff=(int*)calloc(1,sizeof(int));
 if (!stuff) exit(-1);
 *stuff=0xDEADBEEF;
 printf("stuff[@%X]=%X\n", stuff, *stuff);
 *pf21=stuff;
 *pf22=*stuff;
 return 0;
}

int f1(int *pf11, int *pf12){
 return f2(pf11, pf12); 
}

int main(){
 int a1=0,a2=0;
 f1(&a1,&a2);
 printf("a1[@%X]=%X\na2[@%X]=%X\n", &a1, a1, &a2, a2);
 return 0;
}
