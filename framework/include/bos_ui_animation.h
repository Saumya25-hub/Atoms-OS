#ifndef BOS_UI_ANIMATION_H
#define BOS_UI_ANIMATION_H

#include "bos_ui_core.h"

typedef struct {
    BOS_UIElement* target;
    float          start_val;
    float          end_val;
    float          current_val;
    uint32_t       duration_ms;
    uint32_t       elapsed_ms;
    bool           is_active;
    void         (*on_update)(BOS_UIElement* elem, float value);
} BOS_UIAnimation;

void BOS_Animation_Init(void);
void BOS_Animation_Start(BOS_UIAnimation* anim);
void BOS_Animation_Tick(uint32_t delta_ms);

#endif /* BOS_UI_ANIMATION_H */
