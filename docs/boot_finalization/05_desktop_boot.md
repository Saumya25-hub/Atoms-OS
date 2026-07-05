# Desktop Boot

By removing the blocking `DEMO1.wav` playback loop during AC97 initialization, the boot sequence now seamlessly drops into the Desktop Shell.

The User immediately sees the Rook Engine boot splash, followed by the desktop wallpaper, taskbar, and icons. The CPU is instantly yielded to the event polling loop (`BWE_Compose()`), establishing true cooperative desktop multitasking.
