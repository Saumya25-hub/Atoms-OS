#include "music_app.h"
#include "kernel/gui/window/window.h"
#include "kernel/audio/audio_player.h"

void music_app_launch(void) {
    struct BOSWindow* win = window_create("ATOMS Music", 400, 300, 11);
    if (win) {
        window_show(win);
        window_focus(win);
    }
    audio_player_play("/DEMO1.WAV");
}
