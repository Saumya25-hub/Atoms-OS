#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include <stddef.h>
#include <stdint.h>

/* Packaged WSE wallpapers are 1.7-2.3 MiB. Keep a finite ceiling while
 * allowing the largest supported asset to reach the decoder. */
#define IMAGE_LOADER_MAX_BYTES (4u * 1024u * 1024u)

extern void display_print(const char *text);

uint8_t *image_loader_read(const char *path, uint32_t *out_size) {
  if (!path || !out_size)
    return NULL;

  int fd = vfs_open(path);
  if (fd < 0) {
    display_print("[WSE] Asset Load Failed (open)\n");
    return NULL;
  }

  // Keep the input buffer bounded because image decoders require additional
  // working storage. AMSSS/VMM can expand the heap for this bounded request.
  uint32_t capacity = IMAGE_LOADER_MAX_BYTES;
  uint8_t *buffer = (uint8_t *)kmalloc(capacity);
  if (!buffer) {
    vfs_close(fd);
    display_print("[WSE] Asset Load Failed (buffer)\n");
    return NULL;
  }

  uint32_t total_read = 0;
  while (1) {
    if (total_read >= capacity) {
      // Reject malformed or unsupported assets beyond the bounded ceiling.
      vfs_close(fd);
      kfree(buffer);
      *out_size = 0;
      display_print("[WSE] Asset Load Failed (exceeds 4MiB)\n");
      return NULL;
    }

    int bytes_read = vfs_read(fd, buffer + total_read, capacity - total_read);
    if (bytes_read <= 0)
      break;
    total_read += bytes_read;
  }

  vfs_close(fd);
  *out_size = total_read;
  display_print("[WSE] Asset Load Success\n");
  return buffer;
}

void image_loader_free(uint8_t *buffer) {
  if (buffer)
    kfree(buffer);
}
