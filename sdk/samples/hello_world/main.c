#include "sdk/include/bos/bos.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    BOS_SDK_Init("HelloWorldApp", "1.0.0");
    
    BOS_Window* win = BOS_SDK_CreateWindow(100, 100, 640, 480, "Hello World - BOS OS SDK");
    BOS_SDK_ShowWindow(win);

    return BOS_SDK_Run();
}
