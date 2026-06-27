#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512
#define PARTITION_LBA 2048
#define DISK_SIZE (64 * 1024 * 1024)

#pragma pack(push, 1)
typedef struct {
    uint8_t status;
    uint8_t chs_first[3];
    uint8_t type;
    uint8_t chs_last[3];
    uint32_t lba_start;
    uint32_t lba_count;
} MBR_Entry;

typedef struct {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[420];
    uint16_t boot_sector_signature;
} FAT32_BPB;

typedef struct {
    uint32_t lead_signature;
    uint8_t reserved1[480];
    uint32_t struc_signature;
    uint32_t free_count;
    uint32_t next_free;
    uint8_t reserved2[12];
    uint32_t trail_signature;
} FAT32_FSInfo;

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} FAT32_DirEntry;
#pragma pack(pop)

// Helper to allocate clusters in the FAT
uint32_t allocate_clusters(uint32_t* fat, uint32_t start_cluster, uint32_t file_size, uint32_t bytes_per_cluster) {
    if (file_size == 0) {
        fat[start_cluster] = 0x0FFFFFFF; // EOC
        return start_cluster + 1;
    }
    uint32_t clusters = (file_size + bytes_per_cluster - 1) / bytes_per_cluster;
    uint32_t current = start_cluster;
    for (uint32_t i = 1; i < clusters; i++) {
        fat[current] = current + 1;
        current++;
    }
    fat[current] = 0x0FFFFFFF; // EOC
    return current + 1;
}

void copy_file_to_image(FILE* img, const char* path, uint32_t lba) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("Error: Could not open %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* buffer = malloc(size);
    fread(buffer, 1, size, f);
    fclose(f);
    
    fseek(img, lba * SECTOR_SIZE, SEEK_SET);
    fwrite(buffer, 1, size, img);
    free(buffer);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        printf("Usage: image_builder <boot.bin> <stage2.bin> <kernel.bin> <output.img>\n");
        return 1;
    }

    const char* boot_bin = argv[1];
    const char* stage2_bin = argv[2];
    const char* kernel_bin = argv[3];
    const char* out_img = argv[4];

    // 1. Create raw image
    FILE* img = fopen(out_img, "wb+");
    if (!img) {
        printf("Error: Could not create output image %s\n", out_img);
        return 1;
    }

    uint8_t* zero_sector = calloc(1, SECTOR_SIZE);
    for (int i = 0; i < (DISK_SIZE / SECTOR_SIZE); i++) {
        fwrite(zero_sector, 1, SECTOR_SIZE, img);
    }

    // 2. Write bootloader and kernel
    copy_file_to_image(img, boot_bin, 0);
    copy_file_to_image(img, stage2_bin, 1);
    copy_file_to_image(img, kernel_bin, 5);

    // 3. Setup MBR Partition Table at LBA 0, offset 446
    MBR_Entry part1;
    memset(&part1, 0, sizeof(MBR_Entry));
    part1.status = 0x80; // Bootable
    part1.type = 0x0C;   // FAT32 LBA
    part1.lba_start = PARTITION_LBA;
    part1.lba_count = (DISK_SIZE / SECTOR_SIZE) - PARTITION_LBA;
    
    fseek(img, 446, SEEK_SET);
    fwrite(&part1, sizeof(MBR_Entry), 1, img);
    
    // Write MBR Signature just in case
    uint16_t sig = 0xAA55;
    fseek(img, 510, SEEK_SET);
    fwrite(&sig, sizeof(uint16_t), 1, img);

    // 4. Create FAT32 Volume
    uint32_t vol_lba = PARTITION_LBA;
    
    FAT32_BPB bpb;
    memset(&bpb, 0, sizeof(FAT32_BPB));
    bpb.jump[0] = 0xEB; bpb.jump[1] = 0x58; bpb.jump[2] = 0x90; // JMP SHORT
    memcpy(bpb.oem_name, "SIGOS   ", 8);
    bpb.bytes_per_sector = SECTOR_SIZE;
    bpb.sectors_per_cluster = 8; // 4KB clusters
    bpb.reserved_sectors = 32;
    bpb.fat_count = 2;
    bpb.root_dir_entries = 0;
    bpb.total_sectors_16 = 0;
    bpb.media_descriptor = 0xF8;
    bpb.sectors_per_fat_16 = 0;
    bpb.sectors_per_track = 63;
    bpb.heads = 255;
    bpb.hidden_sectors = PARTITION_LBA;
    bpb.total_sectors_32 = part1.lba_count;
    bpb.sectors_per_fat_32 = 128; // Large enough for a 64MB disk
    bpb.flags = 0;
    bpb.fat_version = 0;
    bpb.root_cluster = 2;
    bpb.fs_info_sector = 1;
    bpb.backup_boot_sector = 6;
    bpb.drive_number = 0x80;
    bpb.boot_signature = 0x29;
    bpb.volume_id = 0x12345678;
    memcpy(bpb.volume_label, "SIGNATURES ", 11);
    memcpy(bpb.fs_type, "FAT32   ", 8);
    bpb.boot_sector_signature = 0xAA55;

    fseek(img, vol_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(&bpb, sizeof(FAT32_BPB), 1, img);

    // Write FSInfo
    FAT32_FSInfo fsinfo;
    memset(&fsinfo, 0, sizeof(FAT32_FSInfo));
    fsinfo.lead_signature = 0x41615252;
    fsinfo.struc_signature = 0x61417272;
    fsinfo.free_count = 0xFFFFFFFF;
    fsinfo.next_free = 0xFFFFFFFF;
    fsinfo.trail_signature = 0xAA550000;

    fseek(img, (vol_lba + 1) * SECTOR_SIZE, SEEK_SET);
    fwrite(&fsinfo, sizeof(FAT32_FSInfo), 1, img);

    // 5. Write FAT #1
    uint32_t fat_lba = vol_lba + bpb.reserved_sectors;
    uint32_t* fat = calloc(bpb.sectors_per_fat_32, SECTOR_SIZE);
    
    fat[0] = 0x0FFFFFF8; // Media type
    fat[1] = 0x0FFFFFFF; // EOC
    fat[2] = 0x0FFFFFFF; // Root Directory (EOC)

    // 6. Root Directory
    uint32_t root_dir_lba = fat_lba + (2 * bpb.sectors_per_fat_32);
    FAT32_DirEntry dir[6];
    memset(dir, 0, sizeof(dir));

    // Helper lambda-like to read file size
    FILE* f_init = fopen("build/init.elf", "rb");
    uint32_t init_sz = 0;
    if (f_init) { fseek(f_init, 0, SEEK_END); init_sz = ftell(f_init); fseek(f_init, 0, SEEK_SET); }
    
    FILE* f_test = fopen("build/tests.elf", "rb");
    uint32_t test_sz = 0;
    if (f_test) { fseek(f_test, 0, SEEK_END); test_sz = ftell(f_test); fseek(f_test, 0, SEEK_SET); }

    FILE* f_shell = fopen("build/shell.elf", "rb");
    uint32_t shell_sz = 0;
    if (f_shell) { fseek(f_shell, 0, SEEK_END); shell_sz = ftell(f_shell); fseek(f_shell, 0, SEEK_SET); }

    FILE* f_fault = fopen("build/fault.elf", "rb");
    uint32_t fault_sz = 0;
    if (f_fault) { fseek(f_fault, 0, SEEK_END); fault_sz = ftell(f_fault); fseek(f_fault, 0, SEEK_SET); }

    uint32_t next_cluster = 3;
    uint32_t bytes_per_cluster = SECTOR_SIZE * bpb.sectors_per_cluster;
    
    // BOS_OS.TXT
    memcpy(dir[0].name, "BOS_OS  TXT", 11);
    dir[0].attr = 0x20; // Archive
    dir[0].fst_clus_lo = next_cluster;
    const char* hello_text = 
        "I am BOS. I am fully equipped to read,\n"
        "write, and execute searches. I am\n"
        "operating at peak satisfaction!\n";
    dir[0].file_size = strlen(hello_text);
    next_cluster = allocate_clusters(fat, next_cluster, dir[0].file_size, bytes_per_cluster);
    
    // README.TXT
    memcpy(dir[1].name, "README  TXT", 11);
    dir[1].attr = 0x20; // Archive
    dir[1].fst_clus_lo = next_cluster;
    const char* readme_text = 
        "=== SignaturesOS Phase 12 ===\n"
        "\n"
        "Welcome to the Native Text Viewer!\n"
        "This app demonstrates the new File\n"
        "Association Engine in action.\n"
        "\n"
        "Features:\n"
        "1. Dynamic VFS Integration\n"
        "2. Automatic Line Wrapping\n"
        "3. Multi-instance Window Support\n"
        "4. Interactive Vertical Scrolling\n"
        "\n"
        "Keyboard Controls:\n"
        "- DOWN ARROW or S or J : Scroll Down\n"
        "- UP ARROW or W or K   : Scroll Up\n"
        "- PAGE UP / PAGE DOWN  : Fast Scroll\n"
        "\n"
        "Why scrolling wasn't showing earlier:\n"
        "The scroll offset was updating in memory\n"
        "but the screen redraw compositor was not\n"
        "triggered on key press. Now fixed!\n"
        "\n"
        "SignaturesOS Subsystems:\n"
        "- Stage 1 & 2 Bootloaders (16/32/64-bit)\n"
        "- Paging & Virtual Memory Manager\n"
        "- Preemptive Multitasking Scheduler\n"
        "- PS/2 Mouse & Keyboard Drivers\n"
        "- BOSurface Window Manager Engine\n"
        "- Virtual File System (VFS) & FAT32\n"
        "- File Association & Routing Engine\n"
        "\n"
        "Keep scrolling down to see the end!\n"
        "...\n"
        "Line 35: System stability verified.\n"
        "Line 36: Memory leaks zeroed.\n"
        "Line 37: Rendering pipeline optimized.\n"
        "Line 38: Multi-window cascade active.\n"
        "Line 39: Almost at the end...\n"
        "Line 40: You reached the bottom!\n"
        "=== END OF README ===\n";
    dir[1].file_size = strlen(readme_text);
    next_cluster = allocate_clusters(fat, next_cluster, dir[1].file_size, bytes_per_cluster);

    // INIT.ELF
    memcpy(dir[2].name, "INIT    ELF", 11);
    dir[2].attr = 0x20;
    dir[2].fst_clus_lo = next_cluster;
    dir[2].file_size = init_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[2].file_size, bytes_per_cluster);

    // TESTS.ELF
    memcpy(dir[3].name, "TESTS   ELF", 11);
    dir[3].attr = 0x20;
    dir[3].fst_clus_lo = next_cluster;
    dir[3].file_size = test_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[3].file_size, bytes_per_cluster);

    // SHELL.ELF
    memcpy(dir[4].name, "SHELL   ELF", 11);
    dir[4].attr = 0x20;
    dir[4].fst_clus_lo = next_cluster;
    dir[4].file_size = shell_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[4].file_size, bytes_per_cluster);

    // FAULT.ELF
    memcpy(dir[5].name, "FAULT   ELF", 11);
    dir[5].attr = 0x20;
    dir[5].fst_clus_lo = next_cluster;
    dir[5].file_size = fault_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[5].file_size, bytes_per_cluster);

    fseek(img, fat_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(fat, bpb.sectors_per_fat_32 * SECTOR_SIZE, 1, img);
    
    // Write FAT #2
    fseek(img, (fat_lba + bpb.sectors_per_fat_32) * SECTOR_SIZE, SEEK_SET);
    fwrite(fat, bpb.sectors_per_fat_32 * SECTOR_SIZE, 1, img);

    fseek(img, root_dir_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(dir, sizeof(dir), 1, img);

    // 7. Write File Data
    uint32_t data_lba_base = root_dir_lba - (2 * bpb.sectors_per_cluster); // cluster 2 is at root_dir_lba
    
    // BOS_OS.TXT data
    fseek(img, (data_lba_base + (dir[0].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
    fwrite(hello_text, strlen(hello_text), 1, img);
    
    // README.TXT data
    fseek(img, (data_lba_base + (dir[1].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
    fwrite(readme_text, strlen(readme_text), 1, img);

    // INIT.ELF data
    if (init_sz > 0) {
        uint8_t* init_buf = malloc(init_sz);
        fread(init_buf, 1, init_sz, f_init);
        fseek(img, (data_lba_base + (dir[2].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(init_buf, 1, init_sz, img);
        free(init_buf);
        fclose(f_init);
    }
    
    if (f_test) {
        if (test_sz > 0) {
            uint8_t* test_buf = malloc(test_sz);
            fread(test_buf, 1, test_sz, f_test);
            fseek(img, (data_lba_base + (dir[3].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(test_buf, 1, test_sz, img);
            free(test_buf);
        }
        fclose(f_test);
    }

    if (f_shell) {
        if (shell_sz > 0) {
            uint8_t* shell_buf = malloc(shell_sz);
            fread(shell_buf, 1, shell_sz, f_shell);
            fseek(img, (data_lba_base + (dir[4].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(shell_buf, 1, shell_sz, img);
            free(shell_buf);
        }
        fclose(f_shell);
    }

    if (f_fault) {
        if (fault_sz > 0) {
            uint8_t* fault_buf = malloc(fault_sz);
            fread(fault_buf, 1, fault_sz, f_fault);
            fseek(img, (data_lba_base + (dir[5].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(fault_buf, 1, fault_sz, img);
            free(fault_buf);
        }
        fclose(f_fault);
    }

    free(fat);
    free(zero_sector);
    fclose(img);
    
    printf("Successfully built OS.img with FAT32 partition!\n");
    return 0;
}
