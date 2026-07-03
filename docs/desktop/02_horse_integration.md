# Horse Engine Integration

The Horse Engine acts as the central application registry and launcher.
Rather than allowing the desktop shell to arbitrarily spawn windows,
the shell passes an APP_ID to horse_launch(APP_ID).

## Responsibilities
- Register applications on boot.
- Translate APP_ID into standard launcher callbacks.
- Retrieve running processes for UI.
