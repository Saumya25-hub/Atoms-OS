#include "command.h"
#include "../libbos/include/bos.h"
#include "../libbos/include/bodiskhub.h"
#include "../atoms/include/atoms.h"
#include "../atoms/include/atom_compiler.h"
#include "../atoms/internal/atom_memory.h"

static char current_path[256] = "/";

const char* commands_bodh_get_cwd(void) {
    return current_path;
}

static void print_dec(uint32_t num) {
    if (num == 0) { shell_print("0"); return; }
    char buf[16]; int i = 14; buf[15] = '\0';
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    shell_print(&buf[i + 1]);
}

static void resolve_absolute_path(const char* input, char* out) {
    if (input[0] == '/') {
        int i = 0; while (input[i] && i < 255) { out[i] = input[i]; i++; } out[i] = '\0';
        return;
    }
    int i = 0; while (current_path[i] && i < 255) { out[i] = current_path[i]; i++; }
    if (i > 0 && out[i-1] != '/') out[i++] = '/';
    int j = 0; while (input[j] && i < 255) { out[i++] = input[j++]; }
    out[i] = '\0';
}

static void cmd_pwd(int argc, char** argv) {
    shell_print(current_path); shell_print("\n");
}

static void cmd_open(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: open <directory>\n"); return; }
    char new_path[256];
    resolve_absolute_path(argv[1], new_path);
    bos_dirent_t entry;
    if (bos_readdir(new_path, 0, &entry) == 0) {
        int i = 0; while (new_path[i]) { current_path[i] = new_path[i]; i++; } current_path[i] = '\0';
    } else {
        shell_print("Directory not found or invalid.\n");
    }
}

static void cmd_back(int argc, char** argv) {
    int last_slash = -1;
    for (int i = 0; current_path[i]; i++) if (current_path[i] == '/') last_slash = i;
    if (last_slash <= 0) { current_path[0] = '/'; current_path[1] = '\0'; }
    else current_path[last_slash] = '\0';
}

static void cmd_mkdir(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: mkdir <foldername>\n"); return; }
    char abs_path[256]; resolve_absolute_path(argv[1], abs_path);
    if (bodh_folder_create(abs_path) == 0) shell_print("Folder created successfully!\n");
    else shell_print("Error creating folder.\n");
}

static void cmd_new(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: new <filename>\n"); return; }
    char abs_path[256]; resolve_absolute_path(argv[1], abs_path);
    if (bodh_file_create(abs_path) == 0) shell_print("File created successfully!\n");
    else shell_print("Error creating file.\n");
}

static void cmd_rename(int argc, char** argv) {
    if (argc < 3) { shell_print("Usage: rename <target> <new_name>\n"); return; }
    char abs_target[256]; resolve_absolute_path(argv[1], abs_target);
    if (bodh_object_rename(abs_target, argv[2]) == 0) shell_print("Renamed successfully.\n");
    else shell_print("Rename failed.\n");
}

static void cmd_delete(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: delete <target>\n"); return; }
    char abs_target[256]; resolve_absolute_path(argv[1], abs_target);
    if (bodh_object_delete(abs_target) == 0) shell_print("Deleted successfully.\n");
    else shell_print("Delete failed.\n");
}

static void cmd_ls(int argc, char** argv) {
    char target[256];
    if (argc > 1) resolve_absolute_path(argv[1], target);
    else { int i=0; while(current_path[i]){target[i]=current_path[i]; i++;} target[i]='\0'; }

    shell_print("Directory Listing of: "); shell_print(target); shell_print("\n\n");
    bos_dirent_t entry; int index = 0, count = 0;
    while (bos_readdir(target, index, &entry) == 0) {
        if (entry.is_directory) shell_print("[DIR]  "); else shell_print("[FILE] ");
        shell_print(entry.name);
        int len = 0; while(entry.name[len]) len++;
        for(int p = len; p < 16; p++) shell_print(" ");
        shell_print(" | Size: "); print_dec(entry.size); shell_print(" bytes\n");
        index++; count++;
    }
    if (count == 0) shell_print("Directory is empty.\n");
    else { shell_print("\nTotal objects: "); print_dec(count); shell_print("\n"); }
}

static void cmd_info(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: info <target>\n"); return; }
    
    char target_dir[256];
    char target_name[64];
    
    // Simple logic to extract parent directory and filename from the target path
    char abs_target[256]; resolve_absolute_path(argv[1], abs_target);
    
    int last_slash = -1;
    for (int i = 0; abs_target[i]; i++) if (abs_target[i] == '/') last_slash = i;
    
    if (last_slash <= 0) {
        target_dir[0] = '/'; target_dir[1] = '\0';
    } else {
        int i;
        for (i = 0; i < last_slash; i++) target_dir[i] = abs_target[i];
        target_dir[i] = '\0';
    }
    
    int j = 0;
    for (int i = last_slash + 1; abs_target[i]; i++) target_name[j++] = abs_target[i];
    target_name[j] = '\0';

    bos_dirent_t entry;
    int index = 0;
    int found = 0;
    while (bos_readdir(target_dir, index, &entry) == 0) {
        // Compare names manually
        int match = 1;
        for (int k = 0; target_name[k] || entry.name[k]; k++) {
            if (target_name[k] != entry.name[k]) {
                match = 0;
                break;
            }
        }
        if (match) {
            found = 1;
            shell_print("Name    : "); shell_print(entry.name); shell_print("\n");
            shell_print("Type    : "); shell_print(entry.is_directory ? "DIR" : "FILE"); shell_print("\n");
            shell_print("Size    : "); print_dec(entry.size); shell_print(" bytes\n");
            shell_print("Cluster : "); print_dec(entry.cluster); shell_print("\n");
            break;
        }
        index++;
    }
    
    if (!found) shell_print("Target not found.\n");
}

static void tree_recursive(const char* path, int depth) {
    if (depth > 5) return;
    bos_dirent_t entry; int index = 0;
    while (bos_readdir(path, index, &entry) == 0) {
        index++;
        if (entry.name[0] == '.' && entry.name[1] == '\0') continue;
        if (entry.name[0] == '.' && entry.name[1] == '.' && entry.name[2] == '\0') continue;

        for (int i = 0; i < depth; i++) shell_print("  ");
        if (entry.is_directory) {
            shell_print("[DIR]  "); shell_print(entry.name); shell_print("\n");
            char next_path[256]; int i = 0;
            while(path[i]) { next_path[i] = path[i]; i++; }
            if (i > 0 && next_path[i-1] != '/') next_path[i++] = '/';
            int j = 0; while(entry.name[j]) next_path[i++] = entry.name[j++];
            next_path[i] = '\0';
            tree_recursive(next_path, depth + 1);
        } else {
            shell_print("[FILE] "); shell_print(entry.name); shell_print("\n");
        }
    }
}

static void cmd_tree(int argc, char** argv) {
    char target[256];
    if (argc > 1) resolve_absolute_path(argv[1], target);
    else { int i=0; while(current_path[i]){target[i]=current_path[i]; i++;} target[i]='\0'; }
    shell_print("Tree of "); shell_print(target); shell_print(":\n");
    tree_recursive(target, 0);
}

static void cmd_cat(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: cat <filename>\n"); return; }
    char abs_path[256];
    resolve_absolute_path(argv[1], abs_path);
    int fd = bos_open(abs_path);
    if (fd < 0) { shell_print("Error: Could not open file\n"); return; }
    char buf[512];
    int bytes_read;
    while ((bytes_read = bos_read(fd, buf, 511)) > 0) {
        buf[bytes_read] = '\0';
        shell_print(buf);
    }
    shell_print("\n");
    bos_close(fd);
}

static void cmd_run(int argc, char** argv) {
    if (argc < 2) { shell_print("Usage: run <filename.bosl>\n"); return; }
    char abs_path[256];
    resolve_absolute_path(argv[1], abs_path);
    int fd = bos_open(abs_path);
    if (fd < 0) { shell_print("Error: Could not open file\n"); return; }
    
    // Allocate buffer for source code (max 8KB for now)
    char* source_buffer = (char*)atom_alloc(8192);
    if (!source_buffer) { shell_print("Error: Out of memory for script buffer\n"); bos_close(fd); return; }
    
    int total_bytes = 0;
    int bytes_read;
    while ((bytes_read = bos_read(fd, source_buffer + total_bytes, 8191 - total_bytes)) > 0) {
        total_bytes += bytes_read;
        if (total_bytes >= 8191) break;
    }
    source_buffer[total_bytes] = '\0';
    bos_close(fd);
    
    if (total_bytes == 0) { shell_print("Error: File is empty\n"); return; }
    
    AtomFunction* main_fn = atom_function_create(atom_string_create("main").as.string, 0);
    if (atom_compiler_compile(source_buffer, main_fn->chunk)) {
        AtomVM* vm = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm);
        
        if (!atom_vm_execute(vm, main_fn)) {
            shell_print("BOSL Runtime Error\n");
        }
        
        atom_vm_free(vm);
    } else {
        shell_print("BOSL Compiler Error\n");
    }
    atom_function_destroy(main_fn);
    
    // In our bump allocator, memory will be freed automatically if we had GC, 
    // but right now it stays in the pool. It's fine for simple scripts.
}

void commands_bodh_init(void) {
    command_register("mkdir", cmd_mkdir, "Create a folder", "Folder");
    command_register("new", cmd_new, "Create a file", "File");
    command_register("ls", cmd_ls, "List directory contents", "Navigation");
    command_register("pwd", cmd_pwd, "Print working directory", "Navigation");
    command_register("open", cmd_open, "Open directory (cd)", "Navigation");
    command_register("back", cmd_back, "Go back to parent dir (cd ..)", "Navigation");
    command_register("tree", cmd_tree, "Show directory tree", "Navigation");
    command_register("rename", cmd_rename, "Rename file/folder", "File");
    command_register("delete", cmd_delete, "Delete file/folder", "File");
    command_register("info", cmd_info, "Show object info", "File");
    command_register("cat", cmd_cat, "View file content", "File");
    command_register("run", cmd_run, "Execute a .bosl script", "BOSL");
}
