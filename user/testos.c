#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
int main(int argc,char* argv[]){
    uint64 size=mmcheck();
    printf("the size of memory is %lu",size);
    return 0;
}