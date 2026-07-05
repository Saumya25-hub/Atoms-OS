# Audio Architecture

The production audio architecture operates entirely independently of the boot sequence.

Applications interact with `audio_api.h` (`audio_stream_create`, `audio_stream_write`). 
The `audio_mixer` pulls from these streams into a software master buffer.
The `ac97_playback` engine periodically pumps DMA pages from the mixer's master buffer to the hardware.

No hardware-specific code exists in the application layer. The Music Player (`music_app.c`) utilizes the `audio_player_play` wrapper.
