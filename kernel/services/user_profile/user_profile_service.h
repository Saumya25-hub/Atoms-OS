#ifndef USER_PROFILE_SERVICE_H
#define USER_PROFILE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 👤 ATOMS OS User Profile Service
 * Manages user account identity metadata and avatar placeholder rendering.
 */

void        user_profile_service_init(void);
const char* user_profile_service_get_name(void);
void        user_profile_service_render_avatar(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride, int cx, int cy, int radius, uint8_t alpha);

#endif /* USER_PROFILE_SERVICE_H */
