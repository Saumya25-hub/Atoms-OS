#ifndef BOS_SDK_RUNTIME_H
#define BOS_SDK_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

typedef void* BOS_WindowHandle;
typedef void* BOS_ControlHandle;

BOS_WindowHandle BOS_SDK_CreateWindow(int x, int y, int width, int height, const char* title);
BOS_ControlHandle BOS_SDK_CreateButton(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text);
BOS_ControlHandle BOS_SDK_CreateLabel(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text);
BOS_ControlHandle BOS_SDK_CreateTextBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* placeholder);
BOS_ControlHandle BOS_SDK_CreateCheckBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text, bool is_checked);
BOS_ControlHandle BOS_SDK_CreateRadioButton(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text, bool is_checked);
BOS_ControlHandle BOS_SDK_CreateComboBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text);
BOS_ControlHandle BOS_SDK_CreateListView(BOS_WindowHandle parent, int x, int y, int width, int height);
BOS_ControlHandle BOS_SDK_CreateGridView(BOS_WindowHandle parent, int x, int y, int width, int height);
BOS_ControlHandle BOS_SDK_CreateProgressBar(BOS_WindowHandle parent, int x, int y, int width, int height, int value);
BOS_ControlHandle BOS_SDK_CreateImage(BOS_WindowHandle parent, int x, int y, int width, int height, const char* asset_path);

#endif /* BOS_SDK_RUNTIME_H */
