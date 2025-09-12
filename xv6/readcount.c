#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
    int initial_count, final_count;
    char buf[100];
    int fd;

    // 1. Get the initial read count before any operations.
    initial_count = getreadcount();
    printf(1, "Initial read count: %d\n", initial_count);

    // 2. Create a dummy file to read from.
    if((fd = open("dummyfile.txt", 0x200 | 0x01)) < 0){
        printf(2, "Error: failed to create dummyfile.txt\n");
        exit();
    }
    // Write exactly 100 bytes to it.
    write(fd, "This is a dummy file with exactly 100 bytes of content to ensure the read operation is successful....", 100);
    close(fd);

    // 3. Open the file for reading and read the 100 bytes.
    if((fd = open("dummyfile.txt", 0x00)) < 0){
        printf(2, "Error: failed to open dummyfile.txt for reading\n");
        exit();
    }
    int n = read(fd, buf, sizeof(buf));
    if(n < 100){
        printf(1, "Warning: read fewer than 100 bytes (%d)\n", n);
    }
    close(fd);

    // 4. Get the final count and verify the difference.
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