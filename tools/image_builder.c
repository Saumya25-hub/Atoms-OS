#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512
#define PARTITION_LBA 8192
#define DISK_SIZE (512 * 1024 * 1024)

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
    part1.type = 0xEF;   // EFI System Partition (ESP) for UEFI Firmware Auto-Mount
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
    bpb.sectors_per_cluster = 8; // 4KB clusters to match 4KB directory table allocation
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
    bpb.sectors_per_fat_32 = 1024; // 1024 sectors for 130,000 FAT32 cluster entries
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
    FAT32_DirEntry dir[64];
    memset(dir, 0, sizeof(dir));

    // Helper lambda-like to read file size
    FILE* f_init = fopen("build/init.elf", "rb");
    uint32_t init_sz = 0;
    if (f_init) { fseek(f_init, 0, SEEK_END); init_sz = ftell(f_init); fseek(f_init, 0, SEEK_SET); }
    
    FILE* f_test = fopen("build/test.elf", "rb");
    uint32_t test_sz = 0;
    if (f_test) { fseek(f_test, 0, SEEK_END); test_sz = ftell(f_test); fseek(f_test, 0, SEEK_SET); }

    FILE* f_shell = fopen("build/shell.elf", "rb");
    uint32_t shell_sz = 0;
    if (f_shell) { fseek(f_shell, 0, SEEK_END); shell_sz = ftell(f_shell); fseek(f_shell, 0, SEEK_SET); }

    FILE* f_fault = fopen("build/fault.elf", "rb");
    uint32_t fault_sz = 0;
    if (f_fault) { fseek(f_fault, 0, SEEK_END); fault_sz = ftell(f_fault); fseek(f_fault, 0, SEEK_SET); }

    FILE* f_calc = fopen("build/sdk_explorer.elf", "rb");
    if (!f_calc) f_calc = fopen("build/calc.elf", "rb");
    uint32_t calc_sz = 0;
    if (f_calc) { fseek(f_calc, 0, SEEK_END); calc_sz = ftell(f_calc); fseek(f_calc, 0, SEEK_SET); }

    FILE* f_boot = fopen("build/boot.raw", "rb");
    uint32_t boot_sz = 0;
    if (f_boot) { fseek(f_boot, 0, SEEK_END); boot_sz = ftell(f_boot); fseek(f_boot, 0, SEEK_SET); }

    FILE* f_demo = fopen("MUSIC/DEMO1.wav", "rb");
    uint32_t demo_sz = 0;
    if (f_demo) { fseek(f_demo, 0, SEEK_END); demo_sz = ftell(f_demo); fseek(f_demo, 0, SEEK_SET); }

    FILE* f_boot1 = fopen("boot_sound/bootsound1.wav", "rb");
    uint32_t boot1_sz = 0;
    if (f_boot1) { fseek(f_boot1, 0, SEEK_END); boot1_sz = ftell(f_boot1); fseek(f_boot1, 0, SEEK_SET); }

    /* DOLBY.avi is a real RIFF/AVI MJPEG file — always use it as primary */
    FILE* f_dolby = fopen("TEST-VIDEO/DOLBY.avi", "rb");
    if (!f_dolby) f_dolby = fopen("TEST-VIDEO/test1.avi", "rb");
    if (!f_dolby) f_dolby = fopen("TEST-VIDEO/test1.mp4", "rb");
    if (!f_dolby) f_dolby = fopen("TEST-VIDEO/DOLBY.mp4", "rb");
    uint32_t dolby_sz = 0;
    if (f_dolby) { fseek(f_dolby, 0, SEEK_END); dolby_sz = ftell(f_dolby); fseek(f_dolby, 0, SEEK_SET); }

    FILE* f_doom_elf = fopen("build/doom.elf", "rb");
    uint32_t doom_elf_sz = 0;
    if (f_doom_elf) { 
        fseek(f_doom_elf, 0, SEEK_END); 
        doom_elf_sz = ftell(f_doom_elf); 
        fseek(f_doom_elf, 0, SEEK_SET); 
        
        if (doom_elf_sz == 0) {
            printf("[FATAL] build/doom.elf is 0 bytes! Build failed.\n");
            exit(1);
        }
        
        uint8_t magic[4];
        fread(magic, 1, 4, f_doom_elf);
        fseek(f_doom_elf, 0, SEEK_SET);
        if (magic[0] != 0x7F || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
            printf("[FATAL] build/doom.elf is not a valid ELF file! Build failed.\n");
            exit(1);
        }
    } else {
        printf("[FATAL] build/doom.elf not found! Build failed.\n");
        exit(1);
    }

    FILE* f_doom_wad = fopen("assets/doom/DOOM1.WAD", "rb");
    uint32_t doom_wad_sz = 0;
    if (f_doom_wad) { fseek(f_doom_wad, 0, SEEK_END); doom_wad_sz = ftell(f_doom_wad); fseek(f_doom_wad, 0, SEEK_SET); }

    FILE* f_w1 = fopen("BOOT-WALLAPPERS/1.png", "rb");
    uint32_t w1_sz = 0;
    if (f_w1) { fseek(f_w1, 0, SEEK_END); w1_sz = ftell(f_w1); fseek(f_w1, 0, SEEK_SET); }
    FILE* f_w2 = fopen("BOOT-WALLAPPERS/2.png", "rb");
    uint32_t w2_sz = 0;
    if (f_w2) { fseek(f_w2, 0, SEEK_END); w2_sz = ftell(f_w2); fseek(f_w2, 0, SEEK_SET); }
    FILE* f_w3 = fopen("BOOT-WALLAPPERS/3.png", "rb");
    uint32_t w3_sz = 0;
    if (f_w3) { fseek(f_w3, 0, SEEK_END); w3_sz = ftell(f_w3); fseek(f_w3, 0, SEEK_SET); }
    FILE* f_w4 = fopen("BOOT-WALLAPPERS/4.png", "rb");
    uint32_t w4_sz = 0;
    if (f_w4) { fseek(f_w4, 0, SEEK_END); w4_sz = ftell(f_w4); fseek(f_w4, 0, SEEK_SET); }
    FILE* f_w5 = fopen("BOOT-WALLAPPERS/1.png", "rb");
    uint32_t w5_sz = 0;
    if (f_w5) { fseek(f_w5, 0, SEEK_END); w5_sz = ftell(f_w5); fseek(f_w5, 0, SEEK_SET); }

    // Application Icons
    FILE* f_ico_exp = fopen("assets/icons/explorer.png", "rb");
    uint32_t ico_exp_sz = 0;
    if (f_ico_exp) { fseek(f_ico_exp, 0, SEEK_END); ico_exp_sz = ftell(f_ico_exp); fseek(f_ico_exp, 0, SEEK_SET); }
    FILE* f_ico_term = fopen("assets/icons/terminal.png", "rb");
    uint32_t ico_term_sz = 0;
    if (f_ico_term) { fseek(f_ico_term, 0, SEEK_END); ico_term_sz = ftell(f_ico_term); fseek(f_ico_term, 0, SEEK_SET); }
    FILE* f_ico_sett = fopen("assets/icons/settings.png", "rb");
    uint32_t ico_sett_sz = 0;
    if (f_ico_sett) { fseek(f_ico_sett, 0, SEEK_END); ico_sett_sz = ftell(f_ico_sett); fseek(f_ico_sett, 0, SEEK_SET); }
    FILE* f_ico_calc = fopen("assets/icons/calculator.png", "rb");
    uint32_t ico_calc_sz = 0;
    if (f_ico_calc) { fseek(f_ico_calc, 0, SEEK_END); ico_calc_sz = ftell(f_ico_calc); fseek(f_ico_calc, 0, SEEK_SET); }
    FILE* f_ico_stress = fopen("assets/icons/stresstest.png", "rb");
    uint32_t ico_stress_sz = 0;
    if (f_ico_stress) { fseek(f_ico_stress, 0, SEEK_END); ico_stress_sz = ftell(f_ico_stress); fseek(f_ico_stress, 0, SEEK_SET); }
    FILE* f_ico_music = fopen("assets/icons/music.png", "rb");
    uint32_t ico_music_sz = 0;
    if (f_ico_music) { fseek(f_ico_music, 0, SEEK_END); ico_music_sz = ftell(f_ico_music); fseek(f_ico_music, 0, SEEK_SET); }
    FILE* f_ico_doom = fopen("assets/icons/doom.png", "rb");
    uint32_t ico_doom_sz = 0;
    if (f_ico_doom) { fseek(f_ico_doom, 0, SEEK_END); ico_doom_sz = ftell(f_ico_doom); fseek(f_ico_doom, 0, SEEK_SET); }
    FILE* f_ico_input = fopen("assets/icons/inputlab.png", "rb");
    uint32_t ico_input_sz = 0;
    if (f_ico_input) { fseek(f_ico_input, 0, SEEK_END); ico_input_sz = ftell(f_ico_input); fseek(f_ico_input, 0, SEEK_SET); }
    FILE* f_ico_atrix = fopen("assets/icons/atrix.png", "rb");
    uint32_t ico_atrix_sz = 0;
    if (f_ico_atrix) { fseek(f_ico_atrix, 0, SEEK_END); ico_atrix_sz = ftell(f_ico_atrix); fseek(f_ico_atrix, 0, SEEK_SET); }
    FILE* f_ico_graph3d = fopen("assets/icons/graph3d.png", "rb");
    uint32_t ico_graph3d_sz = 0;
    if (f_ico_graph3d) { fseek(f_ico_graph3d, 0, SEEK_END); ico_graph3d_sz = ftell(f_ico_graph3d); fseek(f_ico_graph3d, 0, SEEK_SET); }
    FILE* f_ico_tmh = fopen("assets/icons/tmh.png", "rb");
    uint32_t ico_tmh_sz = 0;
    if (f_ico_tmh) { fseek(f_ico_tmh, 0, SEEK_END); ico_tmh_sz = ftell(f_ico_tmh); fseek(f_ico_tmh, 0, SEEK_SET); }

    FILE* f_bootx64 = fopen("build/BOOTX64.EFI", "rb");
    uint32_t bootx64_sz = 0;
    if (f_bootx64) { fseek(f_bootx64, 0, SEEK_END); bootx64_sz = ftell(f_bootx64); fseek(f_bootx64, 0, SEEK_SET); }

    FILE* f_kernel_fat = fopen(kernel_bin, "rb");
    uint32_t kernel_fat_sz = 0;
    if (f_kernel_fat) { fseek(f_kernel_fat, 0, SEEK_END); kernel_fat_sz = ftell(f_kernel_fat); fseek(f_kernel_fat, 0, SEEK_SET); }

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

    // CALC.ELF
    memcpy(dir[6].name, "CALC    ELF", 11);
    dir[6].attr = 0x20;
    dir[6].fst_clus_lo = next_cluster;
    dir[6].file_size = calc_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[6].file_size, bytes_per_cluster);

    // PAINT.ELF
    memcpy(dir[7].name, "PAINT   ELF", 11);
    dir[7].attr = 0x20;
    dir[7].fst_clus_lo = dir[3].fst_clus_lo;
    dir[7].file_size = test_sz;

    // TERM.ELF
    memcpy(dir[8].name, "TERM    ELF", 11);
    dir[8].attr = 0x20;
    dir[8].fst_clus_lo = dir[3].fst_clus_lo;
    dir[8].file_size = test_sz;

    // SETT.ELF
    memcpy(dir[9].name, "SETT    ELF", 11);
    dir[9].attr = 0x20;
    dir[9].fst_clus_lo = dir[3].fst_clus_lo;
    dir[9].file_size = test_sz;

    // BOOT.RAW
    memcpy(dir[10].name, "BOOT    RAW", 11);
    dir[10].attr = 0x20;
    dir[10].fst_clus_lo = next_cluster;
    dir[10].file_size = boot_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[10].file_size, bytes_per_cluster);

    // DEMO1.WAV
    memcpy(dir[11].name, "DEMO1   WAV", 11);
    dir[11].attr = 0x20;
    dir[11].fst_clus_lo = next_cluster;
    dir[11].file_size = demo_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[11].file_size, bytes_per_cluster);

    memcpy(dir[12].name, "W1      PNG", 11);
    dir[12].attr = 0x20;
    dir[12].fst_clus_lo = next_cluster;
    dir[12].file_size = w1_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[12].file_size, bytes_per_cluster);

    memcpy(dir[13].name, "W2      PNG", 11);
    dir[13].attr = 0x20;
    dir[13].fst_clus_lo = next_cluster;
    dir[13].file_size = w2_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[13].file_size, bytes_per_cluster);

    memcpy(dir[14].name, "W3      PNG", 11);
    dir[14].attr = 0x20;
    dir[14].fst_clus_lo = next_cluster;
    dir[14].file_size = w3_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[14].file_size, bytes_per_cluster);

    memcpy(dir[15].name, "W4      PNG", 11);
    dir[15].attr = 0x20;
    dir[15].fst_clus_lo = next_cluster;
    dir[15].file_size = w4_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[15].file_size, bytes_per_cluster);

    memcpy(dir[16].name, "W5      PNG", 11);
    dir[16].attr = 0x20;
    dir[16].fst_clus_lo = next_cluster;
    dir[16].file_size = w5_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[16].file_size, bytes_per_cluster);

    memcpy(dir[17].name, "BOOT1   WAV", 11);
    dir[17].attr = 0x20;
    dir[17].fst_clus_lo = next_cluster;
    dir[17].file_size = boot1_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[17].file_size, bytes_per_cluster);

    memcpy(dir[18].name, "DOOM    ELF", 11);
    dir[18].attr = 0x20;
    dir[18].fst_clus_lo = next_cluster;
    dir[18].file_size = doom_elf_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[18].file_size, bytes_per_cluster);

    memcpy(dir[19].name, "DOOM1   WAD", 11);
    dir[19].attr = 0x20;
    dir[19].fst_clus_lo = next_cluster;
    dir[19].file_size = doom_wad_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[19].file_size, bytes_per_cluster);

    memcpy(dir[20].name, "EXPLORERPNG", 11);
    dir[20].attr = 0x20;
    dir[20].fst_clus_lo = next_cluster;
    dir[20].file_size = ico_exp_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[20].file_size, bytes_per_cluster);

    memcpy(dir[21].name, "TERMINALPNG", 11);
    dir[21].attr = 0x20;
    dir[21].fst_clus_lo = next_cluster;
    dir[21].file_size = ico_term_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[21].file_size, bytes_per_cluster);

    memcpy(dir[22].name, "SETTINGSPNG", 11);
    dir[22].attr = 0x20;
    dir[22].fst_clus_lo = next_cluster;
    dir[22].file_size = ico_sett_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[22].file_size, bytes_per_cluster);

    memcpy(dir[23].name, "CALCULATPNG", 11);
    dir[23].attr = 0x20;
    dir[23].fst_clus_lo = next_cluster;
    dir[23].file_size = ico_calc_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[23].file_size, bytes_per_cluster);

    memcpy(dir[24].name, "STRESST PNG", 11);
    dir[24].attr = 0x20;
    dir[24].fst_clus_lo = next_cluster;
    dir[24].file_size = ico_stress_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[24].file_size, bytes_per_cluster);

    memcpy(dir[25].name, "MUSIC   PNG", 11);
    dir[25].attr = 0x20;
    dir[25].fst_clus_lo = next_cluster;
    dir[25].file_size = ico_music_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[25].file_size, bytes_per_cluster);

    memcpy(dir[26].name, "DOOM    PNG", 11);
    dir[26].attr = 0x20;
    dir[26].fst_clus_lo = next_cluster;
    dir[26].file_size = ico_doom_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[26].file_size, bytes_per_cluster);

    memcpy(dir[27].name, "INPUTLABPNG", 11);
    dir[27].attr = 0x20;
    dir[27].fst_clus_lo = next_cluster;
    dir[27].file_size = ico_input_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[27].file_size, bytes_per_cluster);

    memcpy(dir[28].name, "ATRIX   PNG", 11);
    dir[28].attr = 0x20;
    dir[28].fst_clus_lo = next_cluster;
    dir[28].file_size = ico_atrix_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[28].file_size, bytes_per_cluster);

    memcpy(dir[29].name, "GRAPH3D PNG", 11);
    dir[29].attr = 0x20;
    dir[29].fst_clus_lo = next_cluster;
    dir[29].file_size = ico_graph3d_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[29].file_size, bytes_per_cluster);

    memcpy(dir[30].name, "TMH     PNG", 11);
    dir[30].attr = 0x20;
    dir[30].fst_clus_lo = next_cluster;
    dir[30].file_size = ico_tmh_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[30].file_size, bytes_per_cluster);

    memcpy(dir[31].name, "DOLBY   AVI", 11);
    dir[31].attr = 0x20;
    dir[31].fst_clus_lo = (uint16_t)(next_cluster & 0xFFFF);
    dir[31].fst_clus_hi = (uint16_t)((next_cluster >> 16) & 0xFFFF);
    dir[31].file_size = dolby_sz;
    next_cluster = allocate_clusters(fat, next_cluster, dir[31].file_size, bytes_per_cluster);

    /* 1. Allocate Cluster for /EFI directory */
    uint32_t efi_dir_clus = next_cluster++;
    fat[efi_dir_clus] = 0x0FFFFFFF;

    /* 2. Allocate Cluster for /EFI/BOOT directory */
    uint32_t boot_dir_clus = next_cluster++;
    fat[boot_dir_clus] = 0x0FFFFFFF;

    /* 3. Allocate Clusters for BOOTX64.EFI file */
    uint32_t bootx64_file_clus = next_cluster;
    next_cluster = allocate_clusters(fat, bootx64_file_clus, bootx64_sz, bytes_per_cluster);

    /* 4. Allocate Clusters for KERNEL.BIN file */
    uint32_t kernel_fat_clus = next_cluster;
    next_cluster = allocate_clusters(fat, kernel_fat_clus, kernel_fat_sz, bytes_per_cluster);

    /* Root Dir Entry: /EFI (Directory) */
    memcpy(dir[32].name, "EFI        ", 11);
    dir[32].attr = 0x10;
    dir[32].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    dir[32].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    /* Root Dir Entry: /KERNEL.BIN */
    memcpy(dir[33].name, "KERNEL  BIN", 11);
    dir[33].attr = 0x20;
    dir[33].fst_clus_lo = (uint16_t)(kernel_fat_clus & 0xFFFF);
    dir[33].fst_clus_hi = (uint16_t)((kernel_fat_clus >> 16) & 0xFFFF);
    dir[33].file_size = kernel_fat_sz;

    /* 5. Allocate Cluster for STARTUP.NSH auto-boot script */
    const char* startup_nsh_text = "\\EFI\\BOOT\\BOOTX64.EFI\r\n";
    uint32_t startup_nsh_sz = (uint32_t)strlen(startup_nsh_text);
    uint32_t startup_nsh_clus = next_cluster;
    next_cluster = allocate_clusters(fat, startup_nsh_clus, startup_nsh_sz, bytes_per_cluster);

    /* Root Dir Entry: /STARTUP.NSH */
    memcpy(dir[35].name, "STARTUP NSH", 11);
    dir[35].attr = 0x20;
    dir[35].fst_clus_lo = (uint16_t)(startup_nsh_clus & 0xFFFF);
    dir[35].fst_clus_hi = (uint16_t)((startup_nsh_clus >> 16) & 0xFFFF);
    dir[35].file_size = startup_nsh_sz;

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

    if (f_calc) {
        if (calc_sz > 0) {
            uint8_t* calc_buf = malloc(calc_sz);
            fread(calc_buf, 1, calc_sz, f_calc);
            fseek(img, (data_lba_base + (dir[6].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(calc_buf, 1, calc_sz, img);
            free(calc_buf);
        }
        fclose(f_calc);
    }

    // BOOT.RAW data
    if (f_boot) {
        if (boot_sz > 0) {
            uint8_t* boot_buf = malloc(boot_sz);
            fread(boot_buf, 1, boot_sz, f_boot);
            fseek(img, (data_lba_base + (dir[10].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(boot_buf, 1, boot_sz, img);
            free(boot_buf);
        }
        fclose(f_boot);
    }

    // DEMO1.WAV data
    if (f_demo) {
        if (demo_sz > 0) {
            uint8_t* demo_buf = malloc(demo_sz);
            fread(demo_buf, 1, demo_sz, f_demo);
            fseek(img, (data_lba_base + (dir[11].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(demo_buf, 1, demo_sz, img);
            free(demo_buf);
        }
        fclose(f_demo);
    }
    
    // Wallpapers
    if (f_w1) {
        if (w1_sz > 0) {
            uint8_t* buf = malloc(w1_sz);
            fread(buf, 1, w1_sz, f_w1);
            fseek(img, (data_lba_base + (dir[12].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, w1_sz, img);
            free(buf);
        }
        fclose(f_w1);
    }
    if (f_w2) {
        if (w2_sz > 0) {
            uint8_t* buf = malloc(w2_sz);
            fread(buf, 1, w2_sz, f_w2);
            fseek(img, (data_lba_base + (dir[13].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, w2_sz, img);
            free(buf);
        }
        fclose(f_w2);
    }
    if (f_w3) {
        if (w3_sz > 0) {
            uint8_t* buf = malloc(w3_sz);
            fread(buf, 1, w3_sz, f_w3);
            fseek(img, (data_lba_base + (dir[14].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, w3_sz, img);
            free(buf);
        }
        fclose(f_w3);
    }
    if (f_w4) {
        if (w4_sz > 0) {
            uint8_t* buf = malloc(w4_sz);
            fread(buf, 1, w4_sz, f_w4);
            fseek(img, (data_lba_base + (dir[15].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, w4_sz, img);
            free(buf);
        }
        fclose(f_w4);
    }
    if (f_w5) {
        if (w5_sz > 0) {
            uint8_t* buf = malloc(w5_sz);
            fread(buf, 1, w5_sz, f_w5);
            fseek(img, (data_lba_base + (dir[16].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, w5_sz, img);
            free(buf);
        }
        fclose(f_w5);
    }
    
    // BOOT1.WAV
    if (f_boot1) {
        if (boot1_sz > 0) {
            uint8_t* boot1_buf = malloc(boot1_sz);
            fread(boot1_buf, 1, boot1_sz, f_boot1);
            fseek(img, (data_lba_base + (dir[17].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(boot1_buf, 1, boot1_sz, img);
            free(boot1_buf);
        }
        fclose(f_boot1);
    }

    // DOOM.ELF
    if (f_doom_elf) {
        if (doom_elf_sz > 0) {
            uint8_t* buf = malloc(doom_elf_sz);
            fread(buf, 1, doom_elf_sz, f_doom_elf);
            fseek(img, (data_lba_base + (dir[18].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, doom_elf_sz, img);
            free(buf);
        }
        fclose(f_doom_elf);
    }

    // DOOM1.WAD
    if (f_doom_wad) {
        if (doom_wad_sz > 0) {
            uint8_t* buf = malloc(doom_wad_sz);
            fread(buf, 1, doom_wad_sz, f_doom_wad);
            fseek(img, (data_lba_base + (dir[19].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, doom_wad_sz, img);
            free(buf);
        }
        fclose(f_doom_wad);
    }

    // Application Icons Data
    if (f_ico_exp && ico_exp_sz > 0) {
        uint8_t* buf = malloc(ico_exp_sz);
        fread(buf, 1, ico_exp_sz, f_ico_exp);
        fseek(img, (data_lba_base + (dir[20].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_exp_sz, img);
        free(buf);
        fclose(f_ico_exp);
    }
    if (f_ico_term && ico_term_sz > 0) {
        uint8_t* buf = malloc(ico_term_sz);
        fread(buf, 1, ico_term_sz, f_ico_term);
        fseek(img, (data_lba_base + (dir[21].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_term_sz, img);
        free(buf);
        fclose(f_ico_term);
    }
    if (f_ico_sett && ico_sett_sz > 0) {
        uint8_t* buf = malloc(ico_sett_sz);
        fread(buf, 1, ico_sett_sz, f_ico_sett);
        fseek(img, (data_lba_base + (dir[22].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_sett_sz, img);
        free(buf);
        fclose(f_ico_sett);
    }
    if (f_ico_calc && ico_calc_sz > 0) {
        uint8_t* buf = malloc(ico_calc_sz);
        fread(buf, 1, ico_calc_sz, f_ico_calc);
        fseek(img, (data_lba_base + (dir[23].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_calc_sz, img);
        free(buf);
        fclose(f_ico_calc);
    }
    if (f_ico_stress && ico_stress_sz > 0) {
        uint8_t* buf = malloc(ico_stress_sz);
        fread(buf, 1, ico_stress_sz, f_ico_stress);
        fseek(img, (data_lba_base + (dir[24].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_stress_sz, img);
        free(buf);
        fclose(f_ico_stress);
    }
    if (f_ico_music && ico_music_sz > 0) {
        uint8_t* buf = malloc(ico_music_sz);
        fread(buf, 1, ico_music_sz, f_ico_music);
        fseek(img, (data_lba_base + (dir[25].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_music_sz, img);
        free(buf);
        fclose(f_ico_music);
    }
    if (f_ico_doom && ico_doom_sz > 0) {
        uint8_t* buf = malloc(ico_doom_sz);
        fread(buf, 1, ico_doom_sz, f_ico_doom);
        fseek(img, (data_lba_base + (dir[26].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_doom_sz, img);
        free(buf);
        fclose(f_ico_doom);
    }
    if (f_ico_input && ico_input_sz > 0) {
        uint8_t* buf = malloc(ico_input_sz);
        fread(buf, 1, ico_input_sz, f_ico_input);
        fseek(img, (data_lba_base + (dir[27].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_input_sz, img);
        free(buf);
        fclose(f_ico_input);
    }
    if (f_ico_atrix && ico_atrix_sz > 0) {
        uint8_t* buf = malloc(ico_atrix_sz);
        fread(buf, 1, ico_atrix_sz, f_ico_atrix);
        fseek(img, (data_lba_base + (dir[28].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_atrix_sz, img);
        free(buf);
        fclose(f_ico_atrix);
    }
    if (f_ico_graph3d && ico_graph3d_sz > 0) {
        uint8_t* buf = malloc(ico_graph3d_sz);
        fread(buf, 1, ico_graph3d_sz, f_ico_graph3d);
        fseek(img, (data_lba_base + (dir[29].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_graph3d_sz, img);
        free(buf);
        fclose(f_ico_graph3d);
    }
    if (f_ico_tmh && ico_tmh_sz > 0) {
        uint8_t* buf = malloc(ico_tmh_sz);
        fread(buf, 1, ico_tmh_sz, f_ico_tmh);
        fseek(img, (data_lba_base + (dir[30].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, ico_tmh_sz, img);
        free(buf);
        fclose(f_ico_tmh);
    }
    if (f_dolby && dolby_sz > 0) {
        uint8_t* buf = malloc(dolby_sz);
        fread(buf, 1, dolby_sz, f_dolby);
        uint32_t start_clus = ((uint32_t)dir[31].fst_clus_hi << 16) | dir[31].fst_clus_lo;
        fseek(img, (data_lba_base + (start_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, dolby_sz, img);
        free(buf);
        fclose(f_dolby);
    }
    if (f_bootx64 && bootx64_sz > 0) {
        uint8_t* buf = malloc(bootx64_sz);
        fread(buf, 1, bootx64_sz, f_bootx64);
        fseek(img, (data_lba_base + (bootx64_file_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, bootx64_sz, img);
        free(buf);
        fclose(f_bootx64);
    }
    if (f_kernel_fat && kernel_fat_sz > 0) {
        uint8_t* buf = malloc(kernel_fat_sz);
        fread(buf, 1, kernel_fat_sz, f_kernel_fat);
        fseek(img, (data_lba_base + (kernel_fat_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, kernel_fat_sz, img);
        free(buf);
        fclose(f_kernel_fat);
    }

    /* Write /EFI directory table cluster */
    FAT32_DirEntry efi_dir_entries[16];
    memset(efi_dir_entries, 0, sizeof(efi_dir_entries));
    
    memcpy(efi_dir_entries[0].name, ".          ", 11);
    efi_dir_entries[0].attr = 0x10;
    efi_dir_entries[0].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    efi_dir_entries[0].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    memcpy(efi_dir_entries[1].name, "..         ", 11);
    efi_dir_entries[1].attr = 0x10;
    efi_dir_entries[1].fst_clus_lo = 0;
    efi_dir_entries[1].fst_clus_hi = 0;

    memcpy(efi_dir_entries[2].name, "BOOT       ", 11);
    efi_dir_entries[2].attr = 0x10;
    efi_dir_entries[2].fst_clus_lo = (uint16_t)(boot_dir_clus & 0xFFFF);
    efi_dir_entries[2].fst_clus_hi = (uint16_t)((boot_dir_clus >> 16) & 0xFFFF);

    fseek(img, (data_lba_base + (efi_dir_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
    fwrite(efi_dir_entries, sizeof(efi_dir_entries), 1, img);

    /* Write /EFI/BOOT directory table cluster */
    FAT32_DirEntry boot_dir_entries[16];
    memset(boot_dir_entries, 0, sizeof(boot_dir_entries));

    memcpy(boot_dir_entries[0].name, ".          ", 11);
    boot_dir_entries[0].attr = 0x10;
    boot_dir_entries[0].fst_clus_lo = (uint16_t)(boot_dir_clus & 0xFFFF);
    boot_dir_entries[0].fst_clus_hi = (uint16_t)((boot_dir_clus >> 16) & 0xFFFF);

    memcpy(boot_dir_entries[1].name, "..         ", 11);
    boot_dir_entries[1].attr = 0x10;
    boot_dir_entries[1].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    boot_dir_entries[1].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    memcpy(boot_dir_entries[2].name, "BOOTX64 EFI", 11);
    boot_dir_entries[2].attr = 0x20;
    boot_dir_entries[2].fst_clus_lo = (uint16_t)(bootx64_file_clus & 0xFFFF);
    boot_dir_entries[2].fst_clus_hi = (uint16_t)((bootx64_file_clus >> 16) & 0xFFFF);
    boot_dir_entries[2].file_size = bootx64_sz;

    fseek(img, (data_lba_base + (boot_dir_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
    fwrite(boot_dir_entries, sizeof(boot_dir_entries), 1, img);

    /* Write /STARTUP.NSH file data */
    fseek(img, (data_lba_base + (startup_nsh_clus * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
    fwrite(startup_nsh_text, 1, startup_nsh_sz, img);

    free(fat);
    free(zero_sector);
    fclose(img);
    
    printf("Successfully built OS.img with FAT32 partition!\n");
    return 0;
}
