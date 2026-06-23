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

static int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
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

#define HISTORY_MAX 10
static char history[HISTORY_MAX][128];
static int history_count = 0;

void _start(void) {
  bos_print("======================================\n");
  bos_print("       BOS Interactive Shell          \n");
  bos_print("======================================\n");

  char input_buffer[128];
  int buf_len = 0;
  int cursor_pos = 0;
  int current_history = 0;

  while (1) {
    bos_print("\nBOS> ");
    buf_len = 0;
    cursor_pos = 0;
    input_buffer[0] = '\0';
    current_history = history_count;

    // Read line
    bos_key_event_t evt;
    while (1) {
      bos_get_key_event(&evt);
      if (!evt.pressed) continue;

      if (evt.keycode == BOS_KEY_LEFT) {
          if (cursor_pos > 0) {
              cursor_pos--;
              bos_print("\b");
          }
      } else if (evt.keycode == BOS_KEY_RIGHT) {
          if (cursor_pos < buf_len) {
              char str[2] = {input_buffer[cursor_pos], '\0'};
              bos_print(str);
              cursor_pos++;
          }
      } else if (evt.keycode == BOS_KEY_HOME) {
          bos_print("\n[HOME KEY]\nBOS> ");
          bos_print(input_buffer);
          cursor_pos = 0;
          for (int i = 0; i < buf_len; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_END) {
          bos_print("\n[END KEY]\nBOS> ");
          bos_print(input_buffer);
          cursor_pos = buf_len;
      } else if (evt.keycode == BOS_KEY_PGUP) {
          bos_print("\n[PAGE UP KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_PGDN) {
          bos_print("\n[PAGE DOWN KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode >= BOS_KEY_F1 && evt.keycode <= BOS_KEY_F12) {
          bos_print("\n[F");
          bos_print_dec(evt.keycode - BOS_KEY_F1 + 1);
          bos_print(" KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_INS) {
          bos_print("\n[INSERT KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_NUMLOCK) {
          bos_print("\n[NUM LOCK KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_CTRL) {
          bos_print("\n[CTRL KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_ALT) {
          bos_print("\n[ALT KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_SHIFT) {
          bos_print("\n[SHIFT KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
      } else if (evt.keycode == BOS_KEY_DEL) {
          bos_print("\n[DELETE KEY]\nBOS> ");
          bos_print(input_buffer);
          for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");

          if (cursor_pos < buf_len) {
              // Shift left starting from cursor_pos + 1
              for (int i = cursor_pos + 1; i < buf_len; i++) {
                  input_buffer[i - 1] = input_buffer[i];
              }
              buf_len--;
              input_buffer[buf_len] = '\0';

              // Visually update
              bos_print(&input_buffer[cursor_pos]); // print rest
              bos_print(" "); // overwrite last char
              
              // move cursor back to cursor_pos
              int chars_to_backspace = (buf_len - cursor_pos) + 1;
              for (int i = 0; i < chars_to_backspace; i++) {
                  bos_print("\b");
              }
          }
      } else if (evt.keycode == BOS_KEY_UP || evt.keycode == BOS_KEY_DOWN) {
          int next_history = current_history;
          if (evt.keycode == BOS_KEY_UP && history_count > 0 && current_history > 0) {
              next_history = current_history - 1;
          } else if (evt.keycode == BOS_KEY_DOWN && current_history < history_count) {
              next_history = current_history + 1;
          }

          if (next_history != current_history) {
              current_history = next_history;
              
              // Move cursor to end visually
              while (cursor_pos < buf_len) {
                  char str[2] = {input_buffer[cursor_pos], '\0'};
                  bos_print(str);
                  cursor_pos++;
              }
              // Clear line visually
              for(int i=0; i<buf_len; i++) bos_print("\b \b");
              
              if (current_history == history_count) {
                  buf_len = 0;
                  input_buffer[0] = '\0';
              } else {
                  int hist_idx = current_history % HISTORY_MAX;
                  int j=0;
                  while(history[hist_idx][j] != '\0' && j < 127) {
                      input_buffer[j] = history[hist_idx][j];
                      j++;
                  }
                  input_buffer[j] = '\0';
                  buf_len = j;
                  bos_print(input_buffer);
              }
              cursor_pos = buf_len;
          }
      } else if (evt.ascii != 0) {
          char c = evt.ascii;
          
          if (evt.ctrl || evt.alt) {
              bos_print("\n[");
              if (evt.ctrl) bos_print("CTRL + ");
              if (evt.alt) bos_print("ALT + ");
              char str[2] = {c, '\0'};
              if (str[0] >= 'a' && str[0] <= 'z') str[0] -= 32; // Uppercase for display
              bos_print(str);
              bos_print("]\nBOS> ");
              bos_print(input_buffer);
              for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
              continue;
          }

          // Handle Tab
          if (c == '\t') {
              bos_print("\n[TAB KEY (Reserved for Auto-Complete)]\nBOS> ");
              bos_print(input_buffer);
              for (int i = 0; i < buf_len - cursor_pos; i++) bos_print("\b");
              continue;
          }

          // Handle Enter
          if (c == '\n' || c == '\r') {
            bos_print("\n");
            input_buffer[buf_len] = '\0';
            break;
          }

          // Handle Backspace
          if (c == '\b') {
            if (cursor_pos > 0) {
              // Shift left
              for (int i = cursor_pos; i < buf_len; i++) {
                  input_buffer[i - 1] = input_buffer[i];
              }
              buf_len--;
              cursor_pos--;
              input_buffer[buf_len] = '\0';

              // Visually update
              bos_print("\b"); // move back
              bos_print(&input_buffer[cursor_pos]); // print rest
              bos_print(" "); // overwrite last char
              
              // move cursor back to cursor_pos
              int chars_to_backspace = (buf_len - cursor_pos) + 1;
              for (int i = 0; i < chars_to_backspace; i++) {
                  bos_print("\b");
              }
            }
            continue;
          }

          // Store normal character
          if (buf_len < sizeof(input_buffer) - 1) {
            // Shift right
            for (int i = buf_len; i > cursor_pos; i--) {
                input_buffer[i] = input_buffer[i - 1];
            }
            input_buffer[cursor_pos] = c;
            buf_len++;
            cursor_pos++;
            input_buffer[buf_len] = '\0';

            // Visually update
            char str[2] = {c, '\0'};
            bos_print(str);
            bos_print(&input_buffer[cursor_pos]);
            
            // Move cursor back
            int chars_to_backspace = buf_len - cursor_pos;
            for (int i = 0; i < chars_to_backspace; i++) {
                bos_print("\b");
            }
          }
      }
    }

    // Process Command
    if (buf_len == 0) {
      continue;
    }

    // Add to history if not empty and not same as last
    if (buf_len > 0) {
        int last_idx = (history_count - 1) % HISTORY_MAX;
        if (history_count == 0 || strcmp(history[last_idx], input_buffer) != 0) {
            int new_idx = history_count % HISTORY_MAX;
            for(int i=0; i<=buf_len; i++) {
                history[new_idx][i] = input_buffer[i];
            }
            history_count++;
        }
    }

    if (strcmp(input_buffer, "help") == 0) {
      bos_print("Available commands:\n");
      bos_print("  help  - Show this message\n");
      bos_print("  dir   - List files in current directory\n");
      bos_print("  cat   - View file contents (e.g. cat info.txt)\n");
      bos_print("  run      - Run an executable (e.g. run test)\n");
      bos_print("  clear    - Clear the screen\n");
      bos_print("  heapinfo - Show kernel heap statistics\n");
      bos_print("  heapdump - Dump heap block allocations\n");
      bos_print("  heapvalidate - Validate heap block integrity\n");
      bos_print("  heapwalk - View deep heap block details\n");
      bos_print("  heaptrace- Toggle logging for kmalloc\n");
      bos_print("  memmap   - View physical memory map\n");
      bos_print("  dmesg    - View kernel crash/event log\n");
      bos_print("  stressheap- Run Heap V1 stress test\n");
      bos_print("  ps       - Show running and sleeping tasks\n");
      bos_print("  taskinfo <pid> - Show detailed task info\n");
      bos_print("  exit     - Terminate the shell\n");
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
    } else if (strcmp(input_buffer, "heapinfo") == 0) {
        bos_heapinfo();
    } else if (strcmp(input_buffer, "heapdump") == 0) {
        bos_heapdump();
    } else if (strcmp(input_buffer, "heapvalidate") == 0) {
        bos_heapvalidate();
    } else if (strcmp(input_buffer, "heapwalk") == 0) {
        bos_heapwalk();
    } else if (strcmp(input_buffer, "heaptrace") == 0) {
        bos_heaptrace_toggle();
    } else if (strcmp(input_buffer, "memmap") == 0) {
        bos_memmap();
    } else if (strcmp(input_buffer, "dmesg") == 0) {
        bos_dmesg();
    } else if (strcmp(input_buffer, "stressheap") == 0) {
        bos_stressheap();
    } else if (strcmp(input_buffer, "ps") == 0) {
        bos_ps();
    } else if (strncmp(input_buffer, "taskinfo ", 9) == 0) {
        int pid = 0;
        int i = 9;
        while (input_buffer[i] >= '0' && input_buffer[i] <= '9') {
            pid = pid * 10 + (input_buffer[i] - '0');
            i++;
        }
        bos_taskinfo(pid);
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
