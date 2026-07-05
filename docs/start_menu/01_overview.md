# Start Menu & Application Launcher

Phase 8.4 completes the ATOMS OS Desktop by introducing multitasking via the Start Menu and Application Registry.

## Architecture

- **Application Registry**: A central dictionary of installed software (`app_registry.c`).
- **Start Menu**: A floating UI panel bound to the root desktop surface that lists registered applications.
- **Dummy Apps**: Minimal viable windowed applications to test the compositor and focus engine.

Everything runs on the foundational Surface Engine and interacts through the Interaction Engine without bypassing the event flow.
