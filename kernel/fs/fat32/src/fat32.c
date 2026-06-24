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
static char open_file_path[256] = {0};

uint32_t fat32_find_file(FAT32_VOLUME* vol, const char* filename, uint32_t* out_size);
uint32_t fat32_read_file(FAT32_VOLUME* vol, uint32_t start_cluster, uint32_t file_size, void* buffer, uint32_t offset);

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
        int i = 0; 
        while(path[i] && i < 255) { open_file_path[i] = path[i]; i++; } 
        open_file_path[i] = '\0';
        return 0; // Success
    }
    return -1; // Not found
}

static int fat32_read(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    if (open_file_cluster == 0) return -1;

    if (offset >= open_file_size) return 0;
    uint32_t read_size = size;
    if (offset + size > open_file_size) {
        read_size = open_file_size - (uint32_t)offset;
    }
    
    uint32_t bytes_read = fat32_read_file(vol, open_file_cluster, read_size, buffer, (uint32_t)offset);
    return (int)bytes_read;
}

static int fat32_close(VFS_Node* node) {
    (void)node;
    open_file_cluster = 0;
    open_file_size = 0;
    return 0;
}

static int fat32_readdir(VFS_Node* node, const char* path, int index, vfs_dirent_t* out_entry);
static int fat32_write(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer);
static int fat32_mkdir(VFS_Node* parent, const char* name);
static int fat32_create(VFS_Node* parent, const char* name);
static int fat32_rename(VFS_Node* node, const char* old_path, const char* new_name);
static int fat32_delete(VFS_Node* node, const char* path);

FilesystemDriver fat32_fs_driver = {
    .name = "fat32",
    .mount = fat32_mount,
    .open = fat32_open,
    .read = fat32_read,
    .write = fat32_write,
    .close = fat32_close,
    .readdir = fat32_readdir,
    .mkdir = fat32_mkdir,
    .create = fat32_create,
    .rename = fat32_rename,
    .delete = fat32_delete
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
    uint8_t found_attr;
} FAT32_SearchCtx;

static void format_fat_name(const char* filename, char* out_name) {
    int i = 0, j = 0;
    // Fill with spaces
    for (int k = 0; k < 11; k++) out_name[k] = ' ';
    
    if (filename[0] == '.' && filename[1] == '\0') {
        out_name[0] = '.'; return;
    }
    if (filename[0] == '.' && filename[1] == '.' && filename[2] == '\0') {
        out_name[0] = '.'; out_name[1] = '.'; return;
    }

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
            // Handle cluster 0 -> root cluster
            if (search_ctx->found_cluster == 0) search_ctx->found_cluster = vol->root_cluster;
            search_ctx->found_size = entries[i].file_size;
            search_ctx->found_attr = entries[i].attr;
            kfree(buffer);
            return false; // Abort walk, we found it!
        }
    }

    kfree(buffer);
    return true; // Continue walk
}

static uint32_t fat32_resolve_path(FAT32_VOLUME* vol, const char* path, uint32_t* out_size, uint8_t* out_attr) {
    if (!path) return 0;
    
    uint32_t current_cluster = vol->root_cluster;
    
    if (path[0] == '/') path++;
    if (path[0] == '\0') {
        if (out_size) *out_size = 0;
        if (out_attr) *out_attr = FAT_ATTR_DIRECTORY;
        return vol->root_cluster;
    }
    
    char token[128];
    int path_idx = 0;
    
    while (path[path_idx] != '\0') {
        int token_idx = 0;
        while (path[path_idx] != '/' && path[path_idx] != '\0' && token_idx < 127) {
            token[token_idx++] = path[path_idx++];
        }
        token[token_idx] = '\0';
        if (path[path_idx] == '/') path_idx++;
        
        if (token_idx == 0) continue;
        
        FAT32_SearchCtx ctx;
        ctx.vol = vol;
        ctx.found_cluster = 0;
        ctx.found_size = 0;
        ctx.found_attr = 0;
        format_fat_name(token, ctx.target_name);
        
        fat32_walk_cluster_chain(vol, current_cluster, fat32_search_dir_callback, &ctx);
        
        if (ctx.found_cluster == 0) return 0; // Not found
        
        current_cluster = ctx.found_cluster;
        if (out_size) *out_size = ctx.found_size;
        if (out_attr) *out_attr = ctx.found_attr;
        
        if (path[path_idx] != '\0' && !(ctx.found_attr & FAT_ATTR_DIRECTORY)) {
            return 0; // Invalid path
        }
    }
    
    return current_cluster;
}

static uint32_t fat32_resolve_parent(FAT32_VOLUME* vol, const char* path, char* out_filename) {
    if (!path) return 0;
    if (path[0] == '/') path++;
    
    int last_slash = -1;
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') last_slash = i;
    }
    
    if (last_slash == -1) {
        int i = 0;
        while (path[i] && i < 127) {
            out_filename[i] = path[i];
            i++;
        }
        out_filename[i] = '\0';
        return vol->root_cluster;
    }
    
    char parent_path[256];
    for (int i = 0; i < last_slash; i++) parent_path[i] = path[i];
    parent_path[last_slash] = '\0';
    
    int j = 0;
    for (int i = last_slash + 1; path[i] != '\0' && j < 127; i++) out_filename[j++] = path[i];
    out_filename[j] = '\0';
    
    uint8_t attr;
    uint32_t parent_cluster = fat32_resolve_path(vol, parent_path, NULL, &attr);
    if (parent_cluster == 0 || !(attr & FAT_ATTR_DIRECTORY)) return 0;
    
    return parent_cluster;
}

uint32_t fat32_find_file(FAT32_VOLUME* vol, const char* filename, uint32_t* out_size) {
    return fat32_resolve_path(vol, filename, out_size, NULL);
}


// -------------------------------------------------------------
// Sprint 6: File Reading Logic
// -------------------------------------------------------------

typedef struct {
    FAT32_VOLUME* vol;
    uint8_t* buffer;
    uint32_t file_size;
    uint32_t bytes_read;
    uint32_t offset;
    uint32_t current_cluster_idx;
} FAT32_ReadCtx;

static bool fat32_read_file_callback(uint32_t cluster, void* ctx) {
    FAT32_ReadCtx* read_ctx = (FAT32_ReadCtx*)ctx;
    FAT32_VOLUME* vol = read_ctx->vol;
    
    uint32_t clusters_to_skip = read_ctx->offset / vol->bytes_per_cluster;
    
    if (read_ctx->current_cluster_idx < clusters_to_skip) {
        read_ctx->current_cluster_idx++;
        return true; // Skip this cluster entirely
    }
    
    uint32_t cluster_offset = 0;
    if (read_ctx->current_cluster_idx == clusters_to_skip) {
        cluster_offset = read_ctx->offset % vol->bytes_per_cluster;
    }
    
    read_ctx->current_cluster_idx++;

    uint32_t lba = fat32_cluster_to_lba(vol, cluster);
    
    uint8_t* buffer = (uint8_t*)kmalloc(vol->bytes_per_cluster);
    if (!buffer) return false;

    if (!block_device_read(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
        kfree(buffer);
        return false;
    }

    uint32_t remaining = read_ctx->file_size - read_ctx->bytes_read;
    uint32_t available_in_cluster = vol->bytes_per_cluster - cluster_offset;
    uint32_t copy_size = (remaining < available_in_cluster) ? remaining : available_in_cluster;

    uint8_t* dst = read_ctx->buffer + read_ctx->bytes_read;
    uint8_t* src = buffer + cluster_offset;
    for (uint32_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    
    read_ctx->bytes_read += copy_size;
    kfree(buffer);

    return read_ctx->bytes_read < read_ctx->file_size;
}

uint32_t fat32_read_file(FAT32_VOLUME* vol, uint32_t start_cluster, uint32_t file_size, void* buffer, uint32_t offset) {
    FAT32_ReadCtx ctx;
    ctx.vol = vol;
    ctx.buffer = (uint8_t*)buffer;
    ctx.file_size = file_size;
    ctx.bytes_read = 0;
    ctx.offset = offset;
    ctx.current_cluster_idx = 0;

    fat32_walk_cluster_chain(vol, start_cluster, fat32_read_file_callback, &ctx);
    
    return ctx.bytes_read;
}

// -------------------------------------------------------------
// Sprint 7: Directory Reading Logic
// -------------------------------------------------------------

typedef struct {
    FAT32_VOLUME* vol;
    int target_index;
    int current_index;
    vfs_dirent_t* out_entry;
    bool found;
} FAT32_ReaddirCtx;

static bool fat32_readdir_callback(uint32_t cluster, void* ctx) {
    FAT32_ReaddirCtx* readdir_ctx = (FAT32_ReaddirCtx*)ctx;
    FAT32_VOLUME* vol = readdir_ctx->vol;
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
        if (entries[i].name[0] == 0x00) {
            kfree(buffer);
            return false; // End of directory
        }
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        if (entries[i].attr == FAT_ATTR_LFN || entries[i].attr == FAT_ATTR_VOLUME_ID) {
            continue;
        }

        if (readdir_ctx->current_index == readdir_ctx->target_index) {
            int k = 0;
            for (int j = 0; j < 8; j++) {
                if (entries[i].name[j] != ' ') {
                    readdir_ctx->out_entry->name[k++] = entries[i].name[j];
                }
            }
            if (entries[i].name[8] != ' ') {
                readdir_ctx->out_entry->name[k++] = '.';
                for (int j = 8; j < 11; j++) {
                    if (entries[i].name[j] != ' ') {
                        readdir_ctx->out_entry->name[k++] = entries[i].name[j];
                    }
                }
            }
            readdir_ctx->out_entry->name[k] = '\0';
            readdir_ctx->out_entry->size = entries[i].file_size;
            readdir_ctx->out_entry->is_directory = (entries[i].attr & FAT_ATTR_DIRECTORY) ? 1 : 0;
            readdir_ctx->out_entry->cluster = ((uint32_t)entries[i].fst_clus_hi << 16) | entries[i].fst_clus_lo;
            
            readdir_ctx->found = true;
            kfree(buffer);
            return false;
        }
        readdir_ctx->current_index++;
    }

    kfree(buffer);
    return true;
}

static int fat32_readdir(VFS_Node* node, const char* path, int index, vfs_dirent_t* out_entry) {
    if (!node || !node->private_data || !out_entry) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    uint8_t attr;
    uint32_t dir_cluster = fat32_resolve_path(vol, path, NULL, &attr);
    
    if (dir_cluster == 0 || !(attr & FAT_ATTR_DIRECTORY)) {
        return -1; // Not a directory or not found
    }

    FAT32_ReaddirCtx ctx;
    ctx.vol = vol;
    ctx.target_index = index;
    ctx.current_index = 0;
    ctx.out_entry = out_entry;
    ctx.found = false;

    fat32_walk_cluster_chain(vol, dir_cluster, fat32_readdir_callback, &ctx);

    return ctx.found ? 0 : -1;
}

// -------------------------------------------------------------
// Sprint 8: FAT32 Write Foundation (Phase C)
// -------------------------------------------------------------

static bool fat32_write_fat_entry(FAT32_VOLUME* vol, uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t bytes_per_sector = vol->bpb.bytes_per_sector;
    
    for (uint8_t i = 0; i < vol->bpb.fat_count; i++) {
        uint32_t fat_sector = vol->fat_begin + (i * vol->fat_size) + (fat_offset / bytes_per_sector);
        uint32_t ent_offset = fat_offset % bytes_per_sector;

        uint8_t* buffer = (uint8_t*)kmalloc(bytes_per_sector);
        if (!buffer) return false;

        if (!block_device_read(vol->device->id, fat_sector, 1, buffer)) {
            kfree(buffer);
            return false;
        }

        uint32_t* entry = (uint32_t*)&buffer[ent_offset];
        *entry = (*entry & 0xF0000000) | (value & 0x0FFFFFFF);

        if (!block_device_write(vol->device->id, fat_sector, 1, buffer)) {
            kfree(buffer);
            return false;
        }
        kfree(buffer);
    }
    return true;
}

static uint32_t fat32_find_free_cluster(FAT32_VOLUME* vol) {
    for (uint32_t cluster = 2; cluster < vol->total_clusters + 2; cluster++) {
        uint32_t value = fat32_next_cluster(vol, cluster);
        if (value == 0x00000000) {
            return cluster;
        }
    }
    return 0; // Disk full
}

static bool fat32_clear_cluster(FAT32_VOLUME* vol, uint32_t cluster) {
    uint32_t lba = fat32_cluster_to_lba(vol, cluster);
    uint8_t* buffer = (uint8_t*)kmalloc(vol->bytes_per_cluster);
    if (!buffer) return false;
    for (uint32_t i = 0; i < vol->bytes_per_cluster; i++) {
        buffer[i] = 0;
    }
    bool result = block_device_write(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer);
    kfree(buffer);
    return result;
}

static uint32_t fat32_allocate_cluster(FAT32_VOLUME* vol, uint32_t current_tail) {
    uint32_t free_cluster = fat32_find_free_cluster(vol);
    if (free_cluster == 0) return 0;

    display_print("[FAT32] Allocating Cluster: ");
    display_print_dec(free_cluster);
    display_print("\n");

    if (!fat32_write_fat_entry(vol, free_cluster, 0x0FFFFFFF)) return 0;
    if (current_tail >= 2) {
        if (!fat32_write_fat_entry(vol, current_tail, free_cluster)) return 0;
    }
    if (!fat32_clear_cluster(vol, free_cluster)) return 0;

    return free_cluster;
}

typedef struct {
    FAT32_VOLUME* vol;
    FAT32_DIR_ENTRY* new_entry;
    bool written;
} FAT32_AppendDirCtx;

static bool fat32_append_dir_callback(uint32_t cluster, void* ctx) {
    FAT32_AppendDirCtx* append_ctx = (FAT32_AppendDirCtx*)ctx;
    FAT32_VOLUME* vol = append_ctx->vol;
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
        if (entries[i].name[0] == 0x00 || (uint8_t)entries[i].name[0] == 0xE5) {
            for (int k = 0; k < sizeof(FAT32_DIR_ENTRY); k++) {
                ((uint8_t*)&entries[i])[k] = ((uint8_t*)append_ctx->new_entry)[k];
            }
            if (block_device_write(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
                append_ctx->written = true;
            }
            kfree(buffer);
            return false;
        }
    }
    kfree(buffer);
    return true;
}

static bool fat32_append_dir_entry(FAT32_VOLUME* vol, uint32_t dir_cluster, FAT32_DIR_ENTRY* entry) {
    FAT32_AppendDirCtx ctx;
    ctx.vol = vol;
    ctx.new_entry = entry;
    ctx.written = false;

    fat32_walk_cluster_chain(vol, dir_cluster, fat32_append_dir_callback, &ctx);
    if (ctx.written) return true;

    uint32_t current = dir_cluster;
    uint32_t next = fat32_next_cluster(vol, current);
    while (next < 0x0FFFFFF8) {
        current = next;
        next = fat32_next_cluster(vol, current);
    }
    
    uint32_t new_cluster = fat32_allocate_cluster(vol, current);
    if (new_cluster == 0) return false;
    
    fat32_walk_cluster_chain(vol, new_cluster, fat32_append_dir_callback, &ctx);
    return ctx.written;
}

static int fat32_create_object(VFS_Node* parent, const char* path, uint8_t attr) {
    if (!parent || !parent->private_data || !path) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)parent->private_data;

    char name[128];
    uint32_t parent_cluster = fat32_resolve_parent(vol, path, name);
    if (parent_cluster == 0) return -1;

    // Check if exists
    uint8_t existing_attr;
    if (fat32_resolve_path(vol, path, NULL, &existing_attr) != 0) {
        display_print("[FAT32] File/Folder already exists!\n");
        return -1;
    }

    uint32_t new_cluster = fat32_allocate_cluster(vol, 0);
    if (new_cluster == 0) return -1;

    FAT32_DIR_ENTRY entry;
    for(int i=0; i<32; i++) ((uint8_t*)&entry)[i] = 0;

    format_fat_name(name, (char*)entry.name);
    entry.attr = attr;
    entry.fst_clus_hi = (uint16_t)(new_cluster >> 16);
    entry.fst_clus_lo = (uint16_t)(new_cluster & 0xFFFF);
    entry.file_size = 0;

    if (!fat32_append_dir_entry(vol, parent_cluster, &entry)) {
        return -1;
    }

    if (attr & FAT_ATTR_DIRECTORY) {
        FAT32_DIR_ENTRY dot;
        for(int i=0; i<32; i++) ((uint8_t*)&dot)[i] = 0;
        format_fat_name(".", (char*)dot.name);
        dot.attr = FAT_ATTR_DIRECTORY;
        dot.fst_clus_hi = entry.fst_clus_hi;
        dot.fst_clus_lo = entry.fst_clus_lo;
        fat32_append_dir_entry(vol, new_cluster, &dot);

        FAT32_DIR_ENTRY dotdot;
        for(int i=0; i<32; i++) ((uint8_t*)&dotdot)[i] = 0;
        format_fat_name("..", (char*)dotdot.name);
        dotdot.attr = FAT_ATTR_DIRECTORY;
        dotdot.fst_clus_hi = (uint16_t)(parent_cluster >> 16);
        dotdot.fst_clus_lo = (uint16_t)(parent_cluster & 0xFFFF);
        fat32_append_dir_entry(vol, new_cluster, &dotdot);
    }

    display_print("[FAT32] Object '");
    display_print(name);
    display_print("' created!\n");
    return 0;
}

static int fat32_mkdir(VFS_Node* parent, const char* name) {
    return fat32_create_object(parent, name, FAT_ATTR_DIRECTORY);
}

static int fat32_create(VFS_Node* parent, const char* name) {
    return fat32_create_object(parent, name, 0);
}

typedef struct {
    FAT32_VOLUME* vol;
    char target_name[11];
    char new_name[11];
    bool is_delete;
    bool is_size_update;
    uint32_t new_size;
    bool success;
} FAT32_ModifyCtx;

static bool fat32_modify_dir_callback(uint32_t cluster, void* ctx);

uint32_t fat32_write_file(FAT32_VOLUME* vol, uint32_t start_cluster, uint32_t offset, uint32_t size, void* buffer) {
    uint32_t bytes_written = 0;
    uint32_t current_cluster = start_cluster;
    uint8_t* src = (uint8_t*)buffer;
    
    // Walk to the cluster covering 'offset'
    uint32_t clusters_to_skip = offset / vol->bytes_per_cluster;
    uint32_t cluster_offset = offset % vol->bytes_per_cluster;
    
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        uint32_t next = fat32_next_cluster(vol, current_cluster);
        if (next >= 0x0FFFFFF8) {
            next = fat32_allocate_cluster(vol, current_cluster);
            if (next == 0) return bytes_written; // Out of space
        }
        current_cluster = next;
    }
    
    while (bytes_written < size) {
        uint32_t lba = fat32_cluster_to_lba(vol, current_cluster);
        
        uint8_t* sec_buf = (uint8_t*)kmalloc(vol->bytes_per_cluster);
        if (!sec_buf) return bytes_written;
        
        if (cluster_offset > 0 || (size - bytes_written) < vol->bytes_per_cluster) {
            block_device_read(vol->device->id, lba, vol->bpb.sectors_per_cluster, sec_buf);
        }
        
        uint32_t remaining = size - bytes_written;
        uint32_t available = vol->bytes_per_cluster - cluster_offset;
        uint32_t copy_size = (remaining < available) ? remaining : available;
        
        for (uint32_t i = 0; i < copy_size; i++) {
            sec_buf[cluster_offset + i] = src[bytes_written + i];
        }
        
        block_device_write(vol->device->id, lba, vol->bpb.sectors_per_cluster, sec_buf);
        kfree(sec_buf);
        
        bytes_written += copy_size;
        cluster_offset = 0;
        
        if (bytes_written < size) {
            uint32_t next = fat32_next_cluster(vol, current_cluster);
            if (next >= 0x0FFFFFF8) {
                next = fat32_allocate_cluster(vol, current_cluster);
                if (next == 0) return bytes_written; // Out of space
            }
            current_cluster = next;
        }
    }
    return bytes_written;
}

static int fat32_write(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    if (open_file_cluster == 0) return -1;

    uint32_t bytes_written = fat32_write_file(vol, open_file_cluster, (uint32_t)offset, size, buffer);
    
    if (offset + bytes_written > open_file_size) {
        open_file_size = (uint32_t)(offset + bytes_written);
        char target_filename[128];
        uint32_t parent_cluster = fat32_resolve_parent(vol, open_file_path, target_filename);
        if (parent_cluster != 0) {
            FAT32_ModifyCtx ctx;
            ctx.vol = vol;
            ctx.is_delete = false;
            ctx.is_size_update = true;
            ctx.new_size = open_file_size;
            ctx.success = false;
            format_fat_name(target_filename, ctx.target_name);
            fat32_walk_cluster_chain(vol, parent_cluster, fat32_modify_dir_callback, &ctx);
        }
    }
    
    return (int)bytes_written;
}



static bool fat32_modify_dir_callback(uint32_t cluster, void* ctx) {
    FAT32_ModifyCtx* mod_ctx = (FAT32_ModifyCtx*)ctx;
    FAT32_VOLUME* vol = mod_ctx->vol;
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
        if (entries[i].name[0] == 0x00) break;
        if (entries[i].name[0] == 0xE5) continue;
        if (entries[i].attr == FAT_ATTR_LFN) continue;
        
        bool match = true;
        for (int k = 0; k < 11; k++) {
            if (entries[i].name[k] != mod_ctx->target_name[k]) {
                match = false;
                break;
            }
        }

        if (match) {
            uint32_t target_file_cluster = ((uint32_t)entries[i].fst_clus_hi << 16) | entries[i].fst_clus_lo;
            
            if (mod_ctx->is_size_update) {
                entries[i].file_size = mod_ctx->new_size;
            } else if (mod_ctx->is_delete) {
                entries[i].name[0] = 0xE5;
                if (target_file_cluster >= 2) {
                    uint32_t cur = target_file_cluster;
                    while (cur >= 2 && cur < 0x0FFFFFF8) {
                        uint32_t next = fat32_next_cluster(vol, cur);
                        fat32_write_fat_entry(vol, cur, 0);
                        cur = next;
                    }
                }
            } else {
                for (int k = 0; k < 11; k++) entries[i].name[k] = mod_ctx->new_name[k];
            }
            
            if (block_device_write(vol->device->id, lba, vol->bpb.sectors_per_cluster, buffer)) {
                mod_ctx->success = true;
            }
            kfree(buffer);
            return false;
        }
    }

    kfree(buffer);
    return true;
}

static int fat32_rename(VFS_Node* node, const char* old_path, const char* new_name) {
    if (!node || !node->private_data || !old_path || !new_name) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    char target_filename[128];
    uint32_t parent_cluster = fat32_resolve_parent(vol, old_path, target_filename);
    if (parent_cluster == 0) return -1;
    
    FAT32_ModifyCtx ctx;
    ctx.vol = vol;
    ctx.is_delete = false;
    ctx.is_size_update = false;
    ctx.success = false;
    format_fat_name(target_filename, ctx.target_name);
    format_fat_name(new_name, ctx.new_name);

    fat32_walk_cluster_chain(vol, parent_cluster, fat32_modify_dir_callback, &ctx);

    return ctx.success ? 0 : -1;
}

static int fat32_delete(VFS_Node* node, const char* path) {
    if (!node || !node->private_data || !path) return -1;
    FAT32_VOLUME* vol = (FAT32_VOLUME*)node->private_data;

    char target_filename[128];
    uint32_t parent_cluster = fat32_resolve_parent(vol, path, target_filename);
    if (parent_cluster == 0) return -1;

    FAT32_ModifyCtx ctx;
    ctx.vol = vol;
    ctx.is_delete = true;
    ctx.is_size_update = false;
    ctx.success = false;
    format_fat_name(target_filename, ctx.target_name);

    fat32_walk_cluster_chain(vol, parent_cluster, fat32_modify_dir_callback, &ctx);

    return ctx.success ? 0 : -1;
}

void fat32_self_test(void) {
    // Left empty intentionally
}
