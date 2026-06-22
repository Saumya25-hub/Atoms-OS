#include "kernel/fs/fat32/include/fat32.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/display/display.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/string.h"

typedef struct {
    FAT32_BPB bpb;
    BlockDevice* device;
    uint32_t fat_begin;
    uint32_t fat_size;
    uint32_t first_data_sector;
    uint32_t root_cluster;
    uint32_t bytes_per_cluster;
    uint32_t total_clusters;
} FAT32_VOLUME;

static VFS_Node* fat32_mount(BlockDevice* device) {
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    if (!buffer) return NULL;

    display_print("\n[FAT32]\nReading Boot Sector...\n");

    // Read LBA 0 of the partition (Block Device API maps this to absolute physical LBA)
    if (!block_device_read(device->id, 0, 1, buffer)) {
        display_print("Mount Failed\n");
        kfree(buffer);
        return NULL;
    }

    FAT32_BPB* bpb = (FAT32_BPB*)buffer;

    // Verify signatures and BPB parameters
    if (bpb->bytes_per_sector != 512 ||
        bpb->sectors_per_cluster == 0 ||
        bpb->reserved_sectors == 0 ||
        bpb->fat_count != 2 ||
        bpb->root_cluster < 2 ||
        bpb->sectors_per_fat_32 == 0 ||
        bpb->boot_sector_signature != 0xAA55 ||
        (bpb->boot_signature != 0x29 && bpb->boot_signature != 0x28)) {
        
        display_print("Mount Failed\n");
        kfree(buffer);
        return NULL;
    }

    uint32_t cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    uint32_t fat_begin = bpb->reserved_sectors;
    uint32_t fat_size = bpb->sectors_per_fat_32;
    uint32_t first_data_sector = bpb->reserved_sectors + (bpb->fat_count * fat_size);
    uint32_t total_sectors = (bpb->total_sectors_16 != 0) ? bpb->total_sectors_16 : bpb->total_sectors_32;
    uint32_t data_sectors = total_sectors - first_data_sector;
    uint32_t total_clusters = data_sectors / bpb->sectors_per_cluster;

    // Print values requested by CTO
    display_print("Bytes/Sector      : ");
    display_print_dec(bpb->bytes_per_sector);
    display_print("\nSector/Cluster    : ");
    display_print_dec(bpb->sectors_per_cluster);
    display_print("\nCluster Size      : ");
    display_print_dec(cluster_size);
    display_print("\nReserved Sectors  : ");
    display_print_dec(bpb->reserved_sectors);
    display_print("\nFAT Count         : ");
    display_print_dec(bpb->fat_count);
    display_print("\nFAT Size          : ");
    display_print_dec(fat_size);
    display_print("\nFirst FAT         : ");
    display_print_dec(fat_begin);
    display_print("\nFirst Data        : ");
    display_print_dec(first_data_sector);
    display_print("\nRoot Cluster      : ");
    display_print_dec(bpb->root_cluster);
    
    char vol_label[12];
    for(int i = 0; i < 12; i++) vol_label[i] = 0;
    for(int i = 0; i < 11; i++) vol_label[i] = bpb->volume_label[i];
    display_print("\nVolume Label      : ");
    display_print(vol_label);

    char oem_name[9];
    for(int i = 0; i < 9; i++) oem_name[i] = 0;
    for(int i = 0; i < 8; i++) oem_name[i] = bpb->oem_name[i];
    display_print("\nOEM Name          : ");
    display_print(oem_name);
    display_print("\nPASS\n");

    // Create a dummy VFS root node for now (Sprint 1)
    VFS_Node* root = (VFS_Node*)kmalloc(sizeof(VFS_Node));
    if (!root) {
        kfree(buffer);
        return NULL;
    }
    
    FAT32_VOLUME* volume = (FAT32_VOLUME*)kmalloc(sizeof(FAT32_VOLUME));
    if (!volume) {
        kfree(root);
        kfree(buffer);
        return NULL;
    }

    uint8_t* dst = (uint8_t*)&volume->bpb;
    uint8_t* src = (uint8_t*)bpb;
    for (uint32_t i = 0; i < sizeof(FAT32_BPB); i++) dst[i] = src[i];
    
    volume->device = device;
    volume->fat_begin = fat_begin;
    volume->fat_size = fat_size;
    volume->first_data_sector = first_data_sector;
    volume->root_cluster = bpb->root_cluster;
    volume->bytes_per_cluster = cluster_size;
    volume->total_clusters = total_clusters;

    strcpy(root->name, "/");
    root->type = VFS_MOUNTPOINT;
    root->size = 0;
    root->parent = NULL;
    root->private_data = volume;
    
    extern FilesystemDriver fat32_fs_driver;
    root->fs_driver = &fat32_fs_driver;

    kfree(buffer);
    return root;
}

static uint32_t open_file_cluster = 0;
static uint32_t open_file_size = 0;

uint32_t fat32_find_file(FAT32_VOLUME* vol, const char* filename, uint32_t* out_size);
uint32_t fat32_read_file(FAT32_VOLUME* vol, uint32_t start_cluster, uint32_t file_size, void* buffer);

static int fat32_open(VFS_Node* node, const char* path) {
    if (!node || !node->private_data || !path) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    // Remove leading slash if present
    if (path[0] == '/') path++;

    uint32_t file_size = 0;
    uint32_t cluster = fat32_find_file(vol, path, &file_size);
    
    if (cluster != 0) {
        open_file_cluster = cluster;
        open_file_size = file_size;
        return 0; // Success
    }
    return -1; // Not found
}

static int fat32_read(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    if (open_file_cluster == 0) return -1;

    // Simplified for Sprint 7: ignore offset, just read the whole file (or up to size)
    uint32_t read_size = (size < open_file_size) ? size : open_file_size;
    
    uint32_t bytes_read = fat32_read_file(vol, open_file_cluster, read_size, buffer);
    return (int)bytes_read;
}

static int fat32_close(VFS_Node* node) {
    (void)node;
    open_file_cluster = 0;
    open_file_size = 0;
    return 0;
}

FilesystemDriver fat32_fs_driver = {
    .name = "fat32",
    .mount = fat32_mount,
    .open = fat32_open,
    .read = fat32_read,
    .close = fat32_close
};

void fat32_init(void) {
    vfs_register_fs(&fat32_fs_driver);
}

uint32_t fat32_next_cluster(FAT32_VOLUME* vol, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t bytes_per_sector = vol->bpb.bytes_per_sector;
    uint32_t fat_sector = vol->fat_begin + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    uint8_t* buffer = (uint8_t*)kmalloc(bytes_per_sector);
    if (!buffer) return 0x0FFFFFFF; // Assume EOF if OOM

    if (!block_device_read(vol->device->id, fat_sector, 1, buffer)) {
        kfree(buffer);
        return 0x0FFFFFFF;
    }

    uint32_t entry = *((uint32_t*)&buffer[ent_offset]);
    kfree(buffer);

    return entry & 0x0FFFFFFF; // Mask the top 4 bits for FAT32
}

bool fat32_walk_cluster_chain(
    FAT32_VOLUME* volume,
    uint32_t start_cluster,
    bool (*callback)(uint32_t cluster, void* ctx),
    void* ctx) 
{
    uint32_t current_cluster = start_cluster;
    uint32_t max_iterations = volume->total_clusters;
    uint32_t iterations = 0;

    while (true) {
        // Callback for the current cluster
        if (!callback(current_cluster, ctx)) {
            return false; // Callback aborted the walk
        }

        // Rule 141: Use fat32_next_cluster only
        uint32_t next = fat32_next_cluster(volume, current_cluster);

        // Rule 142: EOF Detection
        if (next >= 0x0FFFFFF8) {
            display_print("EOF\n");
            return true;
        }

        // Rule 143: Bad Cluster
        if (next == 0x0FFFFFF7) {
            display_print("[FAT32] Bad Cluster Detected\n");
            return false;
        }

        // Rule 144: Free Cluster
        if (next == 0x00000000) {
            display_print("[FAT32] Free Cluster in Chain\n");
            return false;
        }

        // Rule 145: Loop Protection
        iterations++;
        if (iterations > max_iterations) {
            display_print("[FAT32] Cluster Loop Detected\n");
            return false;
        }

        current_cluster = next;
    }
}

static uint32_t fat32_cluster_to_lba(FAT32_VOLUME* vol, uint32_t cluster) {
    return vol->first_data_sector + ((cluster - 2) * vol->bpb.sectors_per_cluster);
}

static bool fat32_parse_dir_cluster(uint32_t cluster, void* ctx) {
    FAT32_VOLUME* vol = (FAT32_VOLUME*)ctx;
    uint32_t lba = fat32_cluster_to_lba(vol, cluster);
    
    uint8_t* buffer = (uint8_t*)kmalloc(vol->bytes_per_cluster);
    if (!buffer) return false;

    // Read the entire cluster
    if (!block_device_read(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
        kfree(buffer);
        return false;
    }

    FAT32_DIR_ENTRY* entries = (FAT32_DIR_ENTRY*)buffer;
    uint32_t num_entries = vol->bytes_per_cluster / sizeof(FAT32_DIR_ENTRY);

    for (uint32_t i = 0; i < num_entries; i++) {
        if (entries[i].name[0] == 0x00) {
            break; // End of directory
        }
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        if (entries[i].attr == FAT_ATTR_LFN) {
            continue; // Skip Long File Name entries for now
        }

        // Print basic file info
        char name[12];
        for(int j=0; j<11; j++) name[j] = entries[i].name[j];
        name[11] = '\0';

        uint32_t file_cluster = ((uint32_t)entries[i].fst_clus_hi << 16) | entries[i].fst_clus_lo;

        if (entries[i].attr & FAT_ATTR_DIRECTORY) {
            display_print("[DIR]  ");
        } else {
            display_print("[FILE] ");
        }
        
        display_print(name);
        display_print(" | Size: ");
        display_print_dec(entries[i].file_size);
        display_print(" bytes | Cluster: ");
        display_print_dec(file_cluster);
        display_print("\n");
    }

    kfree(buffer);
    return true; // Continue walk
}

// -------------------------------------------------------------
// Sprint 5: File Search Logic
// -------------------------------------------------------------

typedef struct {
    FAT32_VOLUME* vol;
    char target_name[11];
    uint32_t found_cluster;
    uint32_t found_size;
} FAT32_SearchCtx;

static void format_fat_name(const char* filename, char* out_name) {
    int i = 0, j = 0;
    // Fill with spaces
    for (int k = 0; k < 11; k++) out_name[k] = ' ';
    
    // Parse name (up to 8 chars)
    while (filename[i] != '.' && filename[i] != '\0' && j < 8) {
        char c = filename[i];
        if (c >= 'a' && c <= 'z') c -= 32; // To uppercase
        out_name[j++] = c;
        i++;
    }
    
    // Skip to extension
    while (filename[i] != '.' && filename[i] != '\0') i++;
    if (filename[i] == '.') i++;
    
    // Parse extension (up to 3 chars)
    j = 8;
    while (filename[i] != '\0' && j < 11) {
        char c = filename[i];
        if (c >= 'a' && c <= 'z') c -= 32; // To uppercase
        out_name[j++] = c;
        i++;
    }
}

static bool fat32_search_dir_callback(uint32_t cluster, void* ctx) {
    FAT32_SearchCtx* search_ctx = (FAT32_SearchCtx*)ctx;
    FAT32_VOLUME* vol = search_ctx->vol;
    uint32_t lba = fat32_cluster_to_lba(vol, cluster);
    
    uint8_t* buffer = (uint8_t*)kmalloc(vol->bytes_per_cluster);
    if (!buffer) return false;

    if (!block_device_read(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
        kfree(buffer);
        return false;
    }

    FAT32_DIR_ENTRY* entries = (FAT32_DIR_ENTRY*)buffer;
    uint32_t num_entries = vol->bytes_per_cluster / sizeof(FAT32_DIR_ENTRY);

    for (uint32_t i = 0; i < num_entries; i++) {
        if (entries[i].name[0] == 0x00) break; // End of directory
        if (entries[i].name[0] == 0xE5) continue; // Deleted
        if (entries[i].attr == FAT_ATTR_LFN) continue; // Skip LFN
        
        bool match = true;
        for (int k = 0; k < 11; k++) {
            if (entries[i].name[k] != search_ctx->target_name[k]) {
                match = false;
                break;
            }
        }

        if (match) {
            search_ctx->found_cluster = ((uint32_t)entries[i].fst_clus_hi << 16) | entries[i].fst_clus_lo;
            search_ctx->found_size = entries[i].file_size;
            kfree(buffer);
            return false; // Abort walk, we found it!
        }
    }

    kfree(buffer);
    return true; // Continue walk
}

uint32_t fat32_find_file(FAT32_VOLUME* vol, const char* filename, uint32_t* out_size) {
    FAT32_SearchCtx ctx;
    ctx.vol = vol;
    ctx.found_cluster = 0;
    ctx.found_size = 0;
    format_fat_name(filename, ctx.target_name);

    fat32_walk_cluster_chain(vol, vol->root_cluster, fat32_search_dir_callback, &ctx);
    
    if (out_size) *out_size = ctx.found_size;
    return ctx.found_cluster;
}

// -------------------------------------------------------------
// Sprint 6: File Reading Logic
// -------------------------------------------------------------

typedef struct {
    FAT32_VOLUME* vol;
    uint8_t* buffer;
    uint32_t file_size;
    uint32_t bytes_read;
} FAT32_ReadCtx;

static bool fat32_read_file_callback(uint32_t cluster, void* ctx) {
    FAT32_ReadCtx* read_ctx = (FAT32_ReadCtx*)ctx;
    FAT32_VOLUME* vol = read_ctx->vol;
    uint32_t lba = fat32_cluster_to_lba(vol, cluster);
    
    uint8_t* buffer = (uint8_t*)kmalloc(vol->bytes_per_cluster);
    if (!buffer) return false;

    if (!block_device_read(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
        kfree(buffer);
        return false;
    }

    uint32_t remaining = read_ctx->file_size - read_ctx->bytes_read;
    uint32_t copy_size = (remaining < vol->bytes_per_cluster) ? remaining : vol->bytes_per_cluster;

    // Use our custom memcpy loop or if we have a lib function
    uint8_t* dst = read_ctx->buffer + read_ctx->bytes_read;
    for (uint32_t i = 0; i < copy_size; i++) {
        dst[i] = buffer[i];
    }
    
    read_ctx->bytes_read += copy_size;
    kfree(buffer);

    // Continue walking if we haven't read the whole file yet
    return read_ctx->bytes_read < read_ctx->file_size;
}

uint32_t fat32_read_file(FAT32_VOLUME* vol, uint32_t start_cluster, uint32_t file_size, void* buffer) {
    FAT32_ReadCtx ctx;
    ctx.vol = vol;
    ctx.buffer = (uint8_t*)buffer;
    ctx.file_size = file_size;
    ctx.bytes_read = 0;

    fat32_walk_cluster_chain(vol, start_cluster, fat32_read_file_callback, &ctx);
    
    return ctx.bytes_read;
}

void fat32_self_test(void) {
    // Left empty intentionally, self test functionality moved to generic VFS testing
}
