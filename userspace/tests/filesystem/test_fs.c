#include "../test_framework.h"

// Note: File operations are expensive on FAT32 under Bochs/QEMU, 
// so we keep the iteration count reasonable but enough to test cluster allocation.

void test_filesystem_suite(void) {
    TEST_SUITE_START("Filesystem & FAT Integrity");

    // 1. Create and Write
    bool write_ok = true;
    for (int i = 0; i < 50; i++) {
        // Generate filename
        char name[32] = "TEST_";
        char num[4];
        num[0] = (i / 10) + '0';
        num[1] = (i % 10) + '0';
        num[2] = '\0';
        
        // Append num to name (simple strcat)
        int idx = 5;
        for (int j = 0; j < 2; j++) name[idx++] = num[j];
        name[idx] = '\0';

        int fd = bos_open(name);
        if (fd < 0) { write_ok = false; break; }
        
        const char* data = "STRESS TEST DATA\n";
        bos_write(fd, data, 17);
        bos_close(fd);
    }
    ASSERT(write_ok, "Mass File Creation & Write");

    // 2. Read and Verify
    bool read_ok = true;
    for (int i = 0; i < 50; i++) {
        char name[32] = "TEST_";
        char num[4];
        num[0] = (i / 10) + '0';
        num[1] = (i % 10) + '0';
        num[2] = '\0';
        
        int idx = 5;
        for (int j = 0; j < 2; j++) name[idx++] = num[j];
        name[idx] = '\0';

        int fd = bos_open(name);
        if (fd < 0) { read_ok = false; break; }
        
        char buf[32];
        int br = bos_read(fd, buf, 17);
        bos_close(fd);
        
        if (br != 17) read_ok = false;
        // Check data
        for (int k = 0; k < 17; k++) {
            if (buf[k] != "STRESS TEST DATA\n"[k]) read_ok = false;
        }
    }
    ASSERT(read_ok, "Mass File Read & Verify");

    // 3. Delete
    bool del_ok = true;
    for (int i = 0; i < 50; i++) {
        char name[32] = "TEST_";
        char num[4];
        num[0] = (i / 10) + '0';
        num[1] = (i % 10) + '0';
        num[2] = '\0';
        
        int idx = 5;
        for (int j = 0; j < 2; j++) name[idx++] = num[j];
        name[idx] = '\0';

        int res = bos_delete(name);
        if (res < 0) del_ok = false;
    }
    ASSERT(del_ok, "Mass File Deletion");

    // 4. Directory creation
    int dir_res = bos_mkdir("TESTDIR");
    ASSERT(dir_res == 0, "Directory Creation");
    
    // Clean up
    bos_delete("TESTDIR");
}
