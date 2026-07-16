import os
import re

path = 'kernel/kernel.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# We need to inject the boot menu exactly before Desktop_Shell_StartBootExperience or BOF_BeginAtomicFrame()
boot_menu = """
  // ==========================================================
  // ROOT CAUSE INVESTIGATION BOOT MENU
  // ==========================================================
  display_print("\\n==============================\\n");
  display_print("BOS Kernel Diagnostics\\n");
  display_print("1. Continue Boot (Desktop)\\n");
  display_print("2. Audio Test (Kernel Mode)\\n");
  display_print("==============================\\n");
  display_print("Select option: ");

  extern uint8_t io_in8(uint16_t port);
  char choice = 0;
  while(1) {
      if (io_in8(0x64) & 1) {
          uint8_t scancode = io_in8(0x60);
          if (scancode == 0x02) {
              choice = '1';
              display_print("1\\n");
              break;
          }
          if (scancode == 0x03) {
              choice = '2';
              display_print("2\\n");
              break;
          }
      }
  }

  if (choice == '2') {
      extern void audio_diagnostic_mode(void);
      audio_diagnostic_mode(); // Does not return
  }
  // ==========================================================
"""

# Find BOF_BeginAtomicFrame();
if "ROOT CAUSE INVESTIGATION" not in content:
    content = content.replace('extern void BOF_BeginAtomicFrame(void);', boot_menu + '\n  extern void BOF_BeginAtomicFrame(void);')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
