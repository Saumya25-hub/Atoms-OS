#include "../libbos/include/bos.h"

// Very simple string length
static size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}

// Very simple string compare
static int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static void bos_print_dec(uint64_t num) {
    if (num == 0) {
        bos_print("0");
        return;
    }
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    while (num > 0) {
        buf[i--] = (num % 10) + '0';
        num /= 10;
    }
    bos_print(&buf[i + 1]);
}

void _start(void) {
  bos_print("======================================\n");
  bos_print("       BOS Interactive Shell          \n");
  bos_print("======================================\n");

  char input_buffer[128];
  int buf_idx = 0;

  while (1) {
    bos_print("\nBOS> ");
    buf_idx = 0;

    // Read line
    while (1) {
      char c = bos_getc();

      // Handle Enter
      if (c == '\n' || c == '\r') {
        bos_print("\n");
        input_buffer[buf_idx] = '\0';
        break;
      }

      // Handle Backspace
      if (c == '\b') {
        if (buf_idx > 0) {
          buf_idx--;
          // Print backspace, space, backspace to visually erase
          bos_print("\b \b");
        }
        continue;
      }

      // Store normal character
      if (buf_idx < sizeof(input_buffer) - 1) {
        input_buffer[buf_idx++] = c;

        // Echo character
        char str[2] = {c, '\0'};
        bos_print(str);
      }
    }

    // Process Command
    if (buf_idx == 0) {
      continue;
    }

    if (strcmp(input_buffer, "help") == 0) {
      bos_print("Available commands:\n");
      bos_print("  help  - Show this message\n");
      bos_print("  dir   - List files in current directory\n");
      bos_print("  cat   - View file contents (e.g. cat info.txt)\n");
      bos_print("  run   - Run an executable (e.g. run test)\n");
      bos_print("  clear - Clear the screen\n");
      bos_print("  exit  - Terminate the shell\n");
    } else if (strcmp(input_buffer, "clear") == 0) {
      for (int i = 0; i < 25; i++)
        bos_print("\n");
    } else if (strcmp(input_buffer, "saumya") == 0) {
      bos_print("\n====================================================\n");
      bos_print("               SIGNATURES OS - V1                   \n");
      bos_print("====================================================\n\n");
      bos_print("           Architect & Creator: SAUMYA              \n\n");
      bos_print("       \"Not just an OS. A digital legacy.\"        \n");
      bos_print("     A Masterpiece of System Design and Passion.    \n\n");
      bos_print("====================================================\n\n");
    } else if (strcmp(input_buffer, "exit") == 0) {
      bos_print("Exiting shell...\n");
      break;
    } else if (strcmp(input_buffer, "dir") == 0 || strcmp(input_buffer, "ls") == 0) {
      bos_print("Directory listing for /\n");
      bos_print("----------------------------------------\n");
      int index = 0;
      bos_dirent_t entry;
      while (bos_readdir("/", index, &entry) == 0) {
          if (entry.is_directory) {
              bos_print("[DIR]  ");
          } else {
              bos_print("[FILE] ");
          }
          bos_print(entry.name);
          bos_print(" \t");
          bos_print_dec(entry.size);
          bos_print(" bytes\n");
          index++;
      }
      bos_print("----------------------------------------\n");
      bos_print("Total ");
      bos_print_dec(index);
      bos_print(" entries.\n");
    } else if (input_buffer[0] == 'c' && input_buffer[1] == 'a' && input_buffer[2] == 't' && input_buffer[3] == ' ') {
        char* filename = &input_buffer[4];
        char exec_path[128];
        exec_path[0] = '/';
        int j = 1;
        for (int k = 0; filename[k] != '\0' && j < 120; k++) {
            char c = filename[k];
            if (c >= 'a' && c <= 'z') c -= 32;
            exec_path[j++] = c;
        }
        exec_path[j] = '\0';
        
        int fd = bos_open(exec_path);
        if (fd < 0) {
            bos_print("cat: File not found\n");
        } else {
            char file_buf[256];
            while (1) {
                int bytes = bos_read(fd, file_buf, 255);
                if (bytes <= 0) break;
                file_buf[bytes] = '\0';
                bos_print(file_buf);
            }
            bos_close(fd);
            bos_print("\n");
        }
    } else if (input_buffer[0] == 'r' && input_buffer[1] == 'u' && input_buffer[2] == 'n' && input_buffer[3] == ' ') {
        char* filename = &input_buffer[4];
        char exec_path[128];
        exec_path[0] = '/';
        int j = 1;
        int has_ext = 0;
        for (int k = 0; filename[k] != '\0' && j < 120; k++) {
            char c = filename[k];
            if (c >= 'a' && c <= 'z') c -= 32; // Uppercase
            if (c == '.') has_ext = 1;
            exec_path[j++] = c;
        }
        
        if (!has_ext) {
            exec_path[j++] = '.';
            exec_path[j++] = 'E';
            exec_path[j++] = 'L';
            exec_path[j++] = 'F';
        }
        exec_path[j] = '\0';

        uint64_t pid = bos_spawn(exec_path);
        if (pid == (uint64_t)-1) {
            bos_print("run: Failed to spawn process\n");
        } else {
            bos_print("Spawned process ");
            bos_print(exec_path);
            bos_print(" with PID ");
            bos_print_dec(pid);
            bos_print("\n");
        }
    } else {
      // Try to spawn it as a process.
      char exec_path[128];
      int j = 0;
      exec_path[j++] = '/';
      
      int has_ext = 0;
      for (int k = 0; input_buffer[k] != '\0' && j < 120; k++) {
          char c = input_buffer[k];
          if (c >= 'a' && c <= 'z') c -= 32; // Uppercase
          if (c == '.') has_ext = 1;
          exec_path[j++] = c;
      }
      
      if (!has_ext) {
          exec_path[j++] = '.';
          exec_path[j++] = 'E';
          exec_path[j++] = 'L';
          exec_path[j++] = 'F';
      }
      exec_path[j++] = '\0';

      uint64_t pid = bos_spawn(exec_path);
      if (pid == (uint64_t)-1) {
          bos_print("Unknown command: ");
          bos_print(input_buffer);
          bos_print("\n");
      } else {
          bos_print("Spawned process ");
          bos_print(exec_path);
          bos_print(" with PID ");
          bos_print_dec(pid);
          bos_print("\n");
      }
    }
  }

  bos_exit();
  while (1) {
    bos_yield();
  }
}
