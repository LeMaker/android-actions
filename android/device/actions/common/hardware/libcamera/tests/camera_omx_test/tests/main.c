#include <stdio.h>

extern int allocBufTest();
extern int useBufTest();
extern int bufHandleTest();

#define TEST_START(func) do{ \
    int retval=0; \
    printf("\n\nstart of test func=%s!!!\n",#func);   \
    retval = func();                                \
    printf("end of test func=%s ,retval = %d!!!\n\n",#func, retval);     \
}while(0)

int main(int argc, char *argv[])
{
    //TEST_START(allocBufTest);
    //TEST_START(useBufTest);
    TEST_START(bufHandleTest);
    return 0;
}
