#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512
#define ESP_START_LBA 2048ULL    // 1MB alignment

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
    uint64_t signature;       // "EFI PART"
    uint32_t revision;        // 0x00010000
    uint32_t header_size;     // 92
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries; // 128
    uint32_t size_partition_entry;  // 128
    uint32_t partition_array_crc32;
    uint8_t  reserved2[420];
} GPT_Header;

typedef struct {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t name[36]; // UTF-16LE
} GPT_Entry;

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

// CRC32 Calculation
static uint32_t crc32_table[256];
static void init_crc32_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
}

static uint32_t calculate_crc32(const void* data, size_t length) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}

static uint64_t cluster_to_lba(uint32_t data_lba_base, uint32_t cluster, uint8_t sectors_per_cluster) {
    return (uint64_t)data_lba_base + (uint64_t)(cluster - 2) * (uint64_t)sectors_per_cluster;
}

// Helper to allocate clusters in the FAT
static uint32_t allocate_clusters(uint32_t* fat, uint32_t start_cluster, uint32_t file_size, uint32_t bytes_per_cluster) {
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

int main(int argc, char** argv) {
    const char* bootx64_path = "build/BOOTX64.EFI";
    const char* kernel_path  = "build/kernel.bin";
    const char* out_img      = "build/atoms_uefi_test.img";

    if (argc >= 2) bootx64_path = argv[1];
    if (argc >= 3) kernel_path  = argv[2];
    if (argc >= 4) out_img      = argv[3];

    init_crc32_table();

    uint64_t total_sectors = 1048576ULL;
    if (argc >= 5) {
        total_sectors = (uint64_t)atoi(argv[4]) * 1024ULL * 1024ULL / SECTOR_SIZE;
    } else if (strstr(out_img, "media") != NULL) {
        total_sectors = 96ULL * 1024ULL * 1024ULL / SECTOR_SIZE; // 96 MB
    }
    uint64_t esp_end_lba = total_sectors - 34ULL;

    printf("[GPT BUILDER] Creating pristine GPT image: %s (%llu sectors, %llu MB)\n",
           out_img, total_sectors, (total_sectors * SECTOR_SIZE) / (1024 * 1024));

    FILE* img = fopen(out_img, "wb+");
    if (!img) {
        printf("[ERROR] Could not create image %s\n", out_img);
        return 1;
    }

    // Allocate 1MB zero buffer for fast image initialization
    static uint8_t zero_chunk[1024 * 1024];
    memset(zero_chunk, 0, sizeof(zero_chunk));
    uint64_t total_bytes = total_sectors * SECTOR_SIZE;
    for (uint64_t b = 0; b < total_bytes; b += sizeof(zero_chunk)) {
        fwrite(zero_chunk, 1, sizeof(zero_chunk), img);
    }

    // ------------------------------------------------------------------------
    // 1. Protective MBR at LBA 0
    // ------------------------------------------------------------------------
    uint8_t mbr[SECTOR_SIZE];
    memset(mbr, 0, SECTOR_SIZE);
    MBR_Entry* part0 = (MBR_Entry*)&mbr[446];
    part0->status = 0x00;
    part0->chs_first[0] = 0x00; part0->chs_first[1] = 0x02; part0->chs_first[2] = 0x00;
    part0->type = 0xEE; // GPT Protective MBR
    part0->chs_last[0] = 0xFF; part0->chs_last[1] = 0xFF; part0->chs_last[2] = 0xFF;
    part0->lba_start = 1;
    part0->lba_count = (uint32_t)(total_sectors - 1);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    fseek(img, 0, SEEK_SET);
    fwrite(mbr, 1, SECTOR_SIZE, img);

    // ------------------------------------------------------------------------
    // 2. GPT Partition Table Entries (128 entries * 128 bytes = 32 sectors)
    // ------------------------------------------------------------------------
    GPT_Entry p_entries[128];
    memset(p_entries, 0, sizeof(p_entries));

    // Entry 0: EFI System Partition (ESP)
    // GUID C12A7328-F81F-11D2-BA4B-00A0C93EC93B
    uint8_t esp_guid[16] = {
        0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
        0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
    };
    uint8_t esp_unique_guid[16] = {
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x78, 0x90,
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
    };
    memcpy(p_entries[0].type_guid, esp_guid, 16);
    memcpy(p_entries[0].unique_guid, esp_unique_guid, 16);
    p_entries[0].starting_lba = ESP_START_LBA;
    p_entries[0].ending_lba = esp_end_lba;
    p_entries[0].attributes = 0;
    
    const wchar_t* name = L"EFI System Partition";
    for (int i = 0; name[i] != 0 && i < 35; i++) {
        p_entries[0].name[i] = (uint16_t)name[i];
    }

    uint32_t partition_array_crc32 = calculate_crc32(p_entries, sizeof(p_entries));

    // Write Primary Partition Array at LBA 2
    fseek(img, 2 * SECTOR_SIZE, SEEK_SET);
    fwrite(p_entries, sizeof(GPT_Entry), 128, img);

    // ------------------------------------------------------------------------
    // 3. Primary GPT Header at LBA 1
    // ------------------------------------------------------------------------
    GPT_Header gpt_hdr;
    memset(&gpt_hdr, 0, sizeof(GPT_Header));
    memcpy(&gpt_hdr.signature, "EFI PART", 8); // Exact 8-byte ASCII string
    gpt_hdr.revision = 0x00010000;
    gpt_hdr.header_size = 92;
    gpt_hdr.header_crc32 = 0; // Temporary for CRC calculation
    gpt_hdr.reserved = 0;
    gpt_hdr.current_lba = 1;
    gpt_hdr.backup_lba = total_sectors - 1;
    gpt_hdr.first_usable_lba = 34;
    gpt_hdr.last_usable_lba = total_sectors - 34;
    uint8_t disk_guid[16] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };
    memcpy(gpt_hdr.disk_guid, disk_guid, 16);
    gpt_hdr.partition_entry_lba = 2;
    gpt_hdr.num_partition_entries = 128;
    gpt_hdr.size_partition_entry = 128;
    gpt_hdr.partition_array_crc32 = partition_array_crc32;

    gpt_hdr.header_crc32 = calculate_crc32(&gpt_hdr, gpt_hdr.header_size);

    fseek(img, 1 * SECTOR_SIZE, SEEK_SET);
    fwrite(&gpt_hdr, 1, SECTOR_SIZE, img);

    // ------------------------------------------------------------------------
    // 4. Backup GPT Header & Partition Array at end of disk
    // ------------------------------------------------------------------------
    // Write Backup Partition Array at LBA (total_sectors - 33)
    fseek(img, (total_sectors - 33ULL) * SECTOR_SIZE, SEEK_SET);
    fwrite(p_entries, sizeof(GPT_Entry), 128, img);

    // Backup GPT Header at LBA (total_sectors - 1)
    GPT_Header backup_gpt_hdr = gpt_hdr;
    backup_gpt_hdr.header_crc32 = 0;
    backup_gpt_hdr.current_lba = total_sectors - 1;
    backup_gpt_hdr.backup_lba = 1;
    backup_gpt_hdr.partition_entry_lba = total_sectors - 33;
    backup_gpt_hdr.header_crc32 = calculate_crc32(&backup_gpt_hdr, backup_gpt_hdr.header_size);

    fseek(img, (total_sectors - 1ULL) * SECTOR_SIZE, SEEK_SET);
    fwrite(&backup_gpt_hdr, 1, SECTOR_SIZE, img);

    printf("[GPT BUILDER] GPT Header & Partition Tables Written Successfully.\n");

    // ------------------------------------------------------------------------
    // 5. Construct FAT32 ESP Partition starting at ESP_START_LBA (LBA 2048)
    // ------------------------------------------------------------------------
    uint32_t vol_lba = ESP_START_LBA;
    uint32_t total_esp_sectors = (uint32_t)(esp_end_lba - ESP_START_LBA + 1);

    FAT32_BPB bpb;
    memset(&bpb, 0, sizeof(FAT32_BPB));
    bpb.jump[0] = 0xEB; bpb.jump[1] = 0x58; bpb.jump[2] = 0x90;
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
    bpb.hidden_sectors = ESP_START_LBA;
    bpb.total_sectors_32 = total_esp_sectors;
    bpb.sectors_per_fat_32 = 1024;
    bpb.flags = 0;
    bpb.fat_version = 0;
    bpb.root_cluster = 2;
    bpb.fs_info_sector = 1;
    bpb.backup_boot_sector = 6;
    bpb.drive_number = 0x80;
    bpb.boot_signature = 0x29;
    bpb.volume_id = 0x12345678;
    memcpy(bpb.volume_label, "EFI SYSTEM ", 11);
    memcpy(bpb.fs_type, "FAT32   ", 8);
    bpb.boot_sector_signature = 0xAA55;

    fseek(img, vol_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(&bpb, sizeof(FAT32_BPB), 1, img);

    // FSInfo
    FAT32_FSInfo fsinfo;
    memset(&fsinfo, 0, sizeof(FAT32_FSInfo));
    fsinfo.lead_signature = 0x41615252;
    fsinfo.struc_signature = 0x61417272;
    fsinfo.free_count = 0xFFFFFFFF;
    fsinfo.next_free = 0xFFFFFFFF;
    fsinfo.trail_signature = 0xAA550000;

    fseek(img, (vol_lba + 1) * SECTOR_SIZE, SEEK_SET);
    fwrite(&fsinfo, sizeof(FAT32_FSInfo), 1, img);

    // FAT #1
    uint32_t fat_lba = vol_lba + bpb.reserved_sectors;
    uint32_t* fat = calloc(bpb.sectors_per_fat_32, SECTOR_SIZE);
    
    fat[0] = 0x0FFFFFF8;
    fat[1] = 0x0FFFFFFF;
    fat[2] = 0x0FFFFFFF; // Root directory cluster (Cluster 2)

    uint32_t bytes_per_cluster = bpb.sectors_per_cluster * SECTOR_SIZE;
    uint32_t next_cluster = 3;

    // Load file sizes
    FILE* f_bootx64 = fopen(bootx64_path, "rb");
    uint32_t bootx64_sz = 0;
    if (f_bootx64) { fseek(f_bootx64, 0, SEEK_END); bootx64_sz = ftell(f_bootx64); fseek(f_bootx64, 0, SEEK_SET); }
    else { printf("[ERROR] Could not open %s\n", bootx64_path); return 1; }

    FILE* f_kernel = fopen(kernel_path, "rb");
    uint32_t kernel_sz = 0;
    if (f_kernel) { fseek(f_kernel, 0, SEEK_END); kernel_sz = ftell(f_kernel); fseek(f_kernel, 0, SEEK_SET); }
    else { printf("[ERROR] Could not open %s\n", kernel_path); return 1; }

    const char* startup_nsh_text = "\\EFI\\BOOT\\BOOTX64.EFI\r\n";
    uint32_t startup_nsh_sz = (uint32_t)strlen(startup_nsh_text);

    FILE* f_mp4 = fopen("build/TEST.MP4", "rb");
    if (!f_mp4) f_mp4 = fopen("TEST-VIDEO/test.mp4", "rb");
    if (!f_mp4) f_mp4 = fopen("build/DOLBY.MP4", "rb");
    if (!f_mp4) f_mp4 = fopen("TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4", "rb");
    uint32_t mp4_sz = 0;
    if (f_mp4) { fseek(f_mp4, 0, SEEK_END); mp4_sz = ftell(f_mp4); fseek(f_mp4, 0, SEEK_SET); }

    FILE* f_mp3 = fopen("TEST1[TEMP]/NCSJanjiHeroesTonight.mp3", "rb");
    if (!f_mp3) f_mp3 = fopen("TEST-AUDIO/test.mp3", "rb");
    if (!f_mp3) f_mp3 = fopen("build/HEROES.MP3", "rb");
    uint32_t mp3_sz = 0;
    if (f_mp3) { fseek(f_mp3, 0, SEEK_END); mp3_sz = ftell(f_mp3); fseek(f_mp3, 0, SEEK_SET); }

    // Allocate clusters for files/directories
    uint32_t efi_dir_clus = next_cluster;
    next_cluster = allocate_clusters(fat, efi_dir_clus, bytes_per_cluster, bytes_per_cluster);

    uint32_t boot_dir_clus = next_cluster;
    next_cluster = allocate_clusters(fat, boot_dir_clus, bytes_per_cluster, bytes_per_cluster);

    uint32_t bootx64_file_clus = next_cluster;
    next_cluster = allocate_clusters(fat, bootx64_file_clus, bootx64_sz, bytes_per_cluster);

    uint32_t kernel_file_clus = next_cluster;
    next_cluster = allocate_clusters(fat, kernel_file_clus, kernel_sz, bytes_per_cluster);

    uint32_t startup_nsh_clus = next_cluster;
    next_cluster = allocate_clusters(fat, startup_nsh_clus, startup_nsh_sz, bytes_per_cluster);

    uint32_t mp4_file_clus = 0;
    if (f_mp4 && mp4_sz > 0) {
        mp4_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, mp4_file_clus, mp4_sz, bytes_per_cluster);
    }

    uint32_t mp3_file_clus = 0;
    if (f_mp3 && mp3_sz > 0) {
        mp3_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, mp3_file_clus, mp3_sz, bytes_per_cluster);
    }

    FILE* f_media = NULL;
    uint32_t media_sz = 0;
    uint32_t media_file_clus = 0;
    if (strstr(out_img, "media") == NULL) {
        f_media = fopen("build/media.img", "rb");
        if (f_media) {
            fseek(f_media, 0, SEEK_END);
            media_sz = (uint32_t)ftell(f_media);
            fseek(f_media, 0, SEEK_SET);
            media_file_clus = next_cluster;
            next_cluster = allocate_clusters(fat, media_file_clus, media_sz, bytes_per_cluster);
        }
    }

    FILE* f_player = fopen("build/media_player.elf", "rb");
    uint32_t player_sz = 0;
    uint32_t player_file_clus = 0;
    if (f_player) {
        fseek(f_player, 0, SEEK_END);
        player_sz = (uint32_t)ftell(f_player);
        fseek(f_player, 0, SEEK_SET);
        player_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, player_file_clus, player_sz, bytes_per_cluster);
    }

    FILE* f_jvm = fopen("build/jvm.elf", "rb");
    uint32_t jvm_sz = 0;
    uint32_t jvm_file_clus = 0;
    if (f_jvm) {
        fseek(f_jvm, 0, SEEK_END);
        jvm_sz = (uint32_t)ftell(f_jvm);
        fseek(f_jvm, 0, SEEK_SET);
        jvm_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, jvm_file_clus, jvm_sz, bytes_per_cluster);
    }

    FILE* f_hello = fopen("build/HelloAtoms.class", "rb");
    uint32_t hello_sz = 0;
    uint32_t hello_file_clus = 0;
    if (f_hello) {
        fseek(f_hello, 0, SEEK_END);
        hello_sz = (uint32_t)ftell(f_hello);
        fseek(f_hello, 0, SEEK_SET);
        hello_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, hello_file_clus, hello_sz, bytes_per_cluster);
    }

    FILE* f_jar = fopen("test_phase4/demo.jar", "rb");
    if (!f_jar) f_jar = fopen("build/demo.jar", "rb");
    uint32_t jar_sz = 0;
    uint32_t jar_file_clus = 0;
    if (f_jar) {
        fseek(f_jar, 0, SEEK_END);
        jar_sz = (uint32_t)ftell(f_jar);
        fseek(f_jar, 0, SEEK_SET);
        jar_file_clus = next_cluster;
        next_cluster = allocate_clusters(fat, jar_file_clus, jar_sz, bytes_per_cluster);
    }

    // Write FAT1 and FAT2

    fseek(img, fat_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(fat, SECTOR_SIZE, bpb.sectors_per_fat_32, img);

    uint32_t fat2_lba = fat_lba + bpb.sectors_per_fat_32;
    fseek(img, fat2_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(fat, SECTOR_SIZE, bpb.sectors_per_fat_32, img);

    // Root Directory Entries (written to Cluster 2)
    uint32_t data_lba_base = fat_lba + (2 * bpb.sectors_per_fat_32);
    FAT32_DirEntry root_dir[32];
    memset(root_dir, 0, sizeof(root_dir));

    // Entry 0: /EFI directory
    memcpy(root_dir[0].name, "EFI        ", 11);
    root_dir[0].attr = 0x10;
    root_dir[0].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    root_dir[0].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    // Entry 1: /kernel.bin
    memcpy(root_dir[1].name, "KERNEL  BIN", 11);
    root_dir[1].attr = 0x20;
    root_dir[1].fst_clus_lo = (uint16_t)(kernel_file_clus & 0xFFFF);
    root_dir[1].fst_clus_hi = (uint16_t)((kernel_file_clus >> 16) & 0xFFFF);
    root_dir[1].file_size = kernel_sz;

    // Entry 2: /STARTUP.NSH
    memcpy(root_dir[2].name, "STARTUP NSH", 11);
    root_dir[2].attr = 0x20;
    root_dir[2].fst_clus_lo = (uint16_t)(startup_nsh_clus & 0xFFFF);
    root_dir[2].fst_clus_hi = (uint16_t)((startup_nsh_clus >> 16) & 0xFFFF);
    root_dir[2].file_size = startup_nsh_sz;

    // Entry 3: /TEST.MP4
    if (f_mp4 && mp4_sz > 0) {
        memcpy(root_dir[3].name, "TEST    MP4", 11);
        root_dir[3].attr = 0x20;
        root_dir[3].fst_clus_lo = (uint16_t)(mp4_file_clus & 0xFFFF);
        root_dir[3].fst_clus_hi = (uint16_t)((mp4_file_clus >> 16) & 0xFFFF);
        root_dir[3].file_size = mp4_sz;

        // Entry 4: /DOLBY.MP4 (Alias to same cluster)
        memcpy(root_dir[4].name, "DOLBY   MP4", 11);
        root_dir[4].attr = 0x20;
        root_dir[4].fst_clus_lo = (uint16_t)(mp4_file_clus & 0xFFFF);
        root_dir[4].fst_clus_hi = (uint16_t)((mp4_file_clus >> 16) & 0xFFFF);
        root_dir[4].file_size = mp4_sz;
    }

    // Entry 5: /HEROES.MP3
    if (f_mp3 && mp3_sz > 0) {
        memcpy(root_dir[5].name, "HEROES  MP3", 11);
        root_dir[5].attr = 0x20;
        root_dir[5].fst_clus_lo = (uint16_t)(mp3_file_clus & 0xFFFF);
        root_dir[5].fst_clus_hi = (uint16_t)((mp3_file_clus >> 16) & 0xFFFF);
        root_dir[5].file_size = mp3_sz;

        // Entry 6: /TEST.MP3
        memcpy(root_dir[6].name, "TEST    MP3", 11);
        root_dir[6].attr = 0x20;
        root_dir[6].fst_clus_lo = (uint16_t)(mp3_file_clus & 0xFFFF);
        root_dir[6].fst_clus_hi = (uint16_t)((mp3_file_clus >> 16) & 0xFFFF);
        root_dir[6].file_size = mp3_sz;
    }

    // Entry 7: /MEDIA.IMG
    if (f_media && media_sz > 0) {
        memcpy(root_dir[7].name, "MEDIA   IMG", 11);
        root_dir[7].attr = 0x20;
        root_dir[7].fst_clus_lo = (uint16_t)(media_file_clus & 0xFFFF);
        root_dir[7].fst_clus_hi = (uint16_t)((media_file_clus >> 16) & 0xFFFF);
        root_dir[7].file_size = media_sz;
    }

    // Entry 8: /MEDIA.ELF
    if (f_player && player_sz > 0) {
        memcpy(root_dir[8].name, "MEDIA   ELF", 11);
        root_dir[8].attr = 0x20;
        root_dir[8].fst_clus_lo = (uint16_t)(player_file_clus & 0xFFFF);
        root_dir[8].fst_clus_hi = (uint16_t)((player_file_clus >> 16) & 0xFFFF);
        root_dir[8].file_size = player_sz;

        // Entry 9: /media_player.elf (8.3 alias: MEDIA_PLELF)
        memcpy(root_dir[9].name, "MEDIA_PLELF", 11);
        root_dir[9].attr = 0x20;
        root_dir[9].fst_clus_lo = (uint16_t)(player_file_clus & 0xFFFF);
        root_dir[9].fst_clus_hi = (uint16_t)((player_file_clus >> 16) & 0xFFFF);
        root_dir[9].file_size = player_sz;
    }

    // Entry 10: /JVM.ELF
    if (f_jvm && jvm_sz > 0) {
        memcpy(root_dir[10].name, "JVM     ELF", 11);
        root_dir[10].attr = 0x20;
        root_dir[10].fst_clus_lo = (uint16_t)(jvm_file_clus & 0xFFFF);
        root_dir[10].fst_clus_hi = (uint16_t)((jvm_file_clus >> 16) & 0xFFFF);
        root_dir[10].file_size = jvm_sz;
    }

    // Entry 11: /HELLO.CLS (HelloAtoms.class)
    if (f_hello && hello_sz > 0) {
        memcpy(root_dir[11].name, "HELLO   CLS", 11);
        root_dir[11].attr = 0x20;
        root_dir[11].fst_clus_lo = (uint16_t)(hello_file_clus & 0xFFFF);
        root_dir[11].fst_clus_hi = (uint16_t)((hello_file_clus >> 16) & 0xFFFF);
        root_dir[11].file_size = hello_sz;
    }

    // Entry 12: /DEMO.JAR
    if (f_jar && jar_sz > 0) {
        memcpy(root_dir[12].name, "DEMO    JAR", 11);
        root_dir[12].attr = 0x20;
        root_dir[12].fst_clus_lo = (uint16_t)(jar_file_clus & 0xFFFF);
        root_dir[12].fst_clus_hi = (uint16_t)((jar_file_clus >> 16) & 0xFFFF);
        root_dir[12].file_size = jar_sz;
    }

    uint64_t root_lba = cluster_to_lba(data_lba_base, 2, bpb.sectors_per_cluster);

    fseek(img, root_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(root_dir, sizeof(root_dir), 1, img);

    // /EFI Directory Cluster
    FAT32_DirEntry efi_dir[16];
    memset(efi_dir, 0, sizeof(efi_dir));

    memcpy(efi_dir[0].name, ".          ", 11);
    efi_dir[0].attr = 0x10;
    efi_dir[0].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    efi_dir[0].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    memcpy(efi_dir[1].name, "..         ", 11);
    efi_dir[1].attr = 0x10;
    efi_dir[1].fst_clus_lo = 0;
    efi_dir[1].fst_clus_hi = 0;

    memcpy(efi_dir[2].name, "BOOT       ", 11);
    efi_dir[2].attr = 0x10;
    efi_dir[2].fst_clus_lo = (uint16_t)(boot_dir_clus & 0xFFFF);
    efi_dir[2].fst_clus_hi = (uint16_t)((boot_dir_clus >> 16) & 0xFFFF);

    uint64_t efi_lba = cluster_to_lba(data_lba_base, efi_dir_clus, bpb.sectors_per_cluster);
    fseek(img, efi_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(efi_dir, sizeof(efi_dir), 1, img);

    // /EFI/BOOT Directory Cluster
    FAT32_DirEntry boot_dir[16];
    memset(boot_dir, 0, sizeof(boot_dir));

    memcpy(boot_dir[0].name, ".          ", 11);
    boot_dir[0].attr = 0x10;
    boot_dir[0].fst_clus_lo = (uint16_t)(boot_dir_clus & 0xFFFF);
    boot_dir[0].fst_clus_hi = (uint16_t)((boot_dir_clus >> 16) & 0xFFFF);

    memcpy(boot_dir[1].name, "..         ", 11);
    boot_dir[1].attr = 0x10;
    boot_dir[1].fst_clus_lo = (uint16_t)(efi_dir_clus & 0xFFFF);
    boot_dir[1].fst_clus_hi = (uint16_t)((efi_dir_clus >> 16) & 0xFFFF);

    memcpy(boot_dir[2].name, "BOOTX64 EFI", 11);
    boot_dir[2].attr = 0x20;
    boot_dir[2].fst_clus_lo = (uint16_t)(bootx64_file_clus & 0xFFFF);
    boot_dir[2].fst_clus_hi = (uint16_t)((bootx64_file_clus >> 16) & 0xFFFF);
    boot_dir[2].file_size = bootx64_sz;

    uint64_t boot_lba = cluster_to_lba(data_lba_base, boot_dir_clus, bpb.sectors_per_cluster);
    fseek(img, boot_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(boot_dir, sizeof(boot_dir), 1, img);

    // Write file contents
    // Write BOOTX64.EFI
    uint8_t* buf = malloc(bootx64_sz);
    fread(buf, 1, bootx64_sz, f_bootx64);
    uint64_t bootx64_lba = cluster_to_lba(data_lba_base, bootx64_file_clus, bpb.sectors_per_cluster);
    fseek(img, bootx64_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(buf, 1, bootx64_sz, img);
    free(buf);
    fclose(f_bootx64);

    // Write kernel.bin
    buf = malloc(kernel_sz);
    fread(buf, 1, kernel_sz, f_kernel);
    uint64_t kernel_lba = cluster_to_lba(data_lba_base, kernel_file_clus, bpb.sectors_per_cluster);
    fseek(img, kernel_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(buf, 1, kernel_sz, img);
    free(buf);
    fclose(f_kernel);

    // Write STARTUP.NSH
    uint64_t startup_nsh_lba = cluster_to_lba(data_lba_base, startup_nsh_clus, bpb.sectors_per_cluster);
    fseek(img, startup_nsh_lba * SECTOR_SIZE, SEEK_SET);
    fwrite(startup_nsh_text, 1, startup_nsh_sz, img);

    // Write TEST.MP4
    if (f_mp4 && mp4_sz > 0) {
        buf = malloc(mp4_sz);
        fread(buf, 1, mp4_sz, f_mp4);
        uint64_t mp4_lba = cluster_to_lba(data_lba_base, mp4_file_clus, bpb.sectors_per_cluster);
        fseek(img, mp4_lba * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, mp4_sz, img);
        free(buf);
        fclose(f_mp4);
        printf("[GPT BUILDER] Wrote TEST.MP4 / DOLBY.MP4 (%u bytes) to FAT32 ESP cluster %u\n", mp4_sz, mp4_file_clus);
    }

    // Write HEROES.MP3
    if (f_mp3 && mp3_sz > 0) {
        buf = malloc(mp3_sz);
        fread(buf, 1, mp3_sz, f_mp3);
        uint64_t mp3_lba = cluster_to_lba(data_lba_base, mp3_file_clus, bpb.sectors_per_cluster);
        fseek(img, mp3_lba * SECTOR_SIZE, SEEK_SET);
        fwrite(buf, 1, mp3_sz, img);
        free(buf);
        fclose(f_mp3);
        printf("[GPT BUILDER] Wrote HEROES.MP3 / TEST.MP3 (%u bytes) to FAT32 ESP cluster %u\n", mp3_sz, mp3_file_clus);
    }

    // Write MEDIA.IMG in 1MB chunks
    if (f_media && media_sz > 0) {
        uint64_t media_lba = cluster_to_lba(data_lba_base, media_file_clus, bpb.sectors_per_cluster);
        fseek(img, media_lba * SECTOR_SIZE, SEEK_SET);
        uint8_t* cbuf = malloc(1024 * 1024);
        if (cbuf) {
            uint32_t rem = media_sz;
            while (rem > 0) {
                uint32_t chunk = (rem > 1024 * 1024) ? (1024 * 1024) : rem;
                fread(cbuf, 1, chunk, f_media);
                fwrite(cbuf, 1, chunk, img);
                rem -= chunk;
            }
            free(cbuf);
        }
        fclose(f_media);
        printf("[GPT BUILDER] Wrote MEDIA.IMG (%u bytes) to FAT32 ESP cluster %u\n", media_sz, media_file_clus);
    }

    // Write MEDIA.ELF (Ring-3 Userspace Media Player)
    if (f_player && player_sz > 0) {
        buf = malloc(player_sz);
        if (buf) {
            fread(buf, 1, player_sz, f_player);
            uint64_t player_lba = cluster_to_lba(data_lba_base, player_file_clus, bpb.sectors_per_cluster);
            fseek(img, player_lba * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, player_sz, img);
            free(buf);
        }
        fclose(f_player);
        printf("[GPT BUILDER] Wrote MEDIA.ELF (%u bytes) to FAT32 ESP cluster %u\n", player_sz, player_file_clus);
    }

    // Write JVM.ELF (Ring-3 Userspace Java Virtual Machine)
    if (f_jvm && jvm_sz > 0) {
        buf = malloc(jvm_sz);
        if (buf) {
            fread(buf, 1, jvm_sz, f_jvm);
            uint64_t jvm_lba = cluster_to_lba(data_lba_base, jvm_file_clus, bpb.sectors_per_cluster);
            fseek(img, jvm_lba * SECTOR_SIZE, SEEK_SET);
            fwrite(buf, 1, jvm_sz, img);
            free(buf);
        }
        fclose(f_jvm);
        printf("[GPT BUILDER] Wrote JVM.ELF (%u bytes) to FAT32 ESP cluster %u\n", jvm_sz, jvm_file_clus);
    }

    // Write HELLO.CLS (HelloAtoms.class)
    if (f_hello) {
        if (hello_sz > 0) {
            buf = malloc(hello_sz);
            if (buf) {
                fread(buf, 1, hello_sz, f_hello);
                uint64_t hello_lba = cluster_to_lba(data_lba_base, hello_file_clus, bpb.sectors_per_cluster);
                fseek(img, hello_lba * SECTOR_SIZE, SEEK_SET);
                fwrite(buf, 1, hello_sz, img);
                free(buf);
            }
        }
        fclose(f_hello);
        printf("[GPT BUILDER] Wrote HELLO.CLS (%u bytes) to FAT32 ESP cluster %u\n", hello_sz, hello_file_clus);
    }

    // Write DEMO.JAR
    if (f_jar) {
        if (jar_sz > 0) {
            buf = malloc(jar_sz);
            if (buf) {
                fread(buf, 1, jar_sz, f_jar);
                uint64_t jar_lba = cluster_to_lba(data_lba_base, jar_file_clus, bpb.sectors_per_cluster);
                fseek(img, jar_lba * SECTOR_SIZE, SEEK_SET);
                fwrite(buf, 1, jar_sz, img);
                free(buf);
            }
        }
        fclose(f_jar);
        printf("[GPT BUILDER] Wrote DEMO.JAR (%u bytes) to FAT32 ESP cluster %u\n", jar_sz, jar_file_clus);
    }

    free(fat);

    fclose(img);

    printf("[GPT BUILDER] SUCCESS! Created pristine UEFI/GPT test image: %s\n", out_img);
    return 0;
}
