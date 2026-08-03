#ifndef BOS_NOTES_APP_H
#define BOS_NOTES_APP_H

#include <stdint.h>
#include <stdbool.h>

// Open a specific text file in Notes.BOSX editor
void notes_app_open(const char* filepath);

// Launch Notes.BOSX standalone application
int  notes_app_launch(uint32_t* out_win);

#endif // BOS_NOTES_APP_H
