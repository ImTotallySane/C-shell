#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
    int initial_count, final_count;
    char buf[100];
    int fd;

    initial_count = getreadcount();
    printf(1, "Initial read count: %d\n", initial_count);

    if((fd = open("dummyfile.txt", 0x200 | 0x01)) < 0){
        printf(2, "Error: failed to create dummyfile.txt\n");
        exit();
    }
    write(fd, "This is a dummy file with exactly 100 bytes of content to ensure the read operation is successful....", 100);
    close(fd);

    if((fd = open("dummyfile.txt", 0x00)) < 0){
        printf(2, "Error: failed to open dummyfile.txt for reading\n");
        exit();
    }
    int n = read(fd, buf, sizeof(buf));
    if(n < 100){
        printf(1, "Warning: read fewer than 100 bytes (%d)\n", n);
    }
    close(fd);

    final_count = getreadcount();
    printf(1, "Final read count: %d\n", final_count);
    printf(1, "Bytes read by this program: %d\n", final_count - initial_count);

    if ((final_count - initial_count) == 100) {
        printf(1, "Verification successful: Count increased by 100.\n");
    } else {
        printf(2, "Verification FAILED.\n");
    }

    exit();
}