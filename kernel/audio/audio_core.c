#include "audio_core.h"
#include <stddef.h>
#include "../drivers/display/display.h"

static AudioStream* stream_list_head = NULL;
static uint32_t next_stream_id = 1;

void audio_core_init(void) {
    stream_list_head = NULL;
    display_print("[AUDIO] PCM Engine Ready\n");
}

void audio_core_shutdown(void) {
    AudioStream* current = stream_list_head;
    while (current != NULL) {
        AudioStream* next = current->next;
        audio_stream_destroy_obj(current);
        current = next;
    }
    stream_list_head = NULL;
}

AudioStream* audio_core_register_stream(uint32_t process_id) {
    uint32_t stream_id = next_stream_id++;
    AudioStream* new_stream = audio_stream_create_obj(stream_id, process_id);
    if (!new_stream) return NULL;

    // Insert at head
    new_stream->next = stream_list_head;
    stream_list_head = new_stream;

    return new_stream;
}

bool audio_core_destroy_stream(uint32_t stream_id) {
    AudioStream* current = stream_list_head;
    AudioStream* prev = NULL;

    while (current != NULL) {
        if (current->stream_id == stream_id) {
            if (prev == NULL) {
                stream_list_head = current->next;
            } else {
                prev->next = current->next;
            }
            audio_stream_destroy_obj(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false; // Not found
}

AudioStream* audio_core_get_stream(uint32_t stream_id) {
    AudioStream* current = stream_list_head;
    while (current != NULL) {
        if (current->stream_id == stream_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

AudioStream* audio_core_get_active_streams(void) {
    return stream_list_head;
}
