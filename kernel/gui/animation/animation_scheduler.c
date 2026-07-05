#include "animation_scheduler.h"
#include "animation_engine.h"

// External accessors from animation_engine.c
extern BOFLOW_Animation* _boflow_get_all_animations(void);
extern uint32_t _boflow_get_max_animations(void);

void animation_scheduler_update(uint32_t delta_time_ms) {
    BOFLOW_Animation* anims = _boflow_get_all_animations();
    uint32_t max_anims = _boflow_get_max_animations();
    
    for (uint32_t i = 0; i < max_anims; i++) {
        if (anims[i].is_running) {
            anims[i].elapsed_ms += delta_time_ms;
            
            uint32_t t = 1000;
            if (anims[i].duration_ms > 0) {
                if (anims[i].elapsed_ms < anims[i].duration_ms) {
                    t = (anims[i].elapsed_ms * 1000) / anims[i].duration_ms;
                }
            }
            
            if (t > 1000) t = 1000;
            
            uint32_t eased_t = easing_calculate(anims[i].easing, t);
            
            // start_val and end_val can be anything, but let's assume they are within int32 bounds
            anims[i].current_val = anims[i].start_val + ((anims[i].end_val - anims[i].start_val) * (int32_t)eased_t) / 1000;
            
            if (anims[i].on_update) {
                anims[i].on_update(anims[i].id, anims[i].current_val, anims[i].user_data);
            }
            
            if (t >= 1000) {
                anims[i].is_running = false;
                if (anims[i].on_complete) {
                    anims[i].on_complete(anims[i].id, anims[i].user_data);
                }
            }
        }
    }
}
