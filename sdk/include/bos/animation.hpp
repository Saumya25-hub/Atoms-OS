#ifndef BOS_UI_ANIMATION_HPP
#define BOS_UI_ANIMATION_HPP

#include <stdint.h>
#include <stddef.h>

namespace bos {

// ============================================================================
// Easing Curves
// ============================================================================
enum class Easing : uint32_t {
    Linear = 0,
    EaseIn,
    EaseOut,
    EaseInOut
};

inline float evaluate_easing(Easing easing, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (easing) {
        case Easing::Linear:
            return t;
        case Easing::EaseIn:
            return t * t;
        case Easing::EaseOut:
            return t * (2.0f - t);
        case Easing::EaseInOut:
            return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t);
        default:
            return t;
    }
}

// ============================================================================
// Animation Descriptor & Callbacks
// ============================================================================
typedef void (*AnimationUpdateFn)(float current_val, void* user_data);
typedef void (*AnimationCompleteFn)(void* user_data);

struct AnimationItem {
    int32_t             id{-1};
    float               start_val{0.0f};
    float               end_val{0.0f};
    uint32_t            duration_ms{0};
    uint32_t            elapsed_ms{0};
    Easing              easing{Easing::EaseOut};
    AnimationUpdateFn   on_update{nullptr};
    AnimationCompleteFn on_complete{nullptr};
    void*               user_data{nullptr};
    bool                active{false};
};

// ============================================================================
// Animator System (Non-blocking, zero heap allocation)
// ============================================================================
class Animator {
public:
    static constexpr size_t MAX_ANIMATIONS = 32;

    static Animator& instance() {
        static Animator s_instance;
        return s_instance;
    }

    int32_t start(float start_val, float end_val, uint32_t duration_ms,
                  Easing easing, AnimationUpdateFn on_update,
                  AnimationCompleteFn on_complete = nullptr, void* user_data = nullptr) {
        for (size_t i = 0; i < MAX_ANIMATIONS; ++i) {
            if (!m_items[i].active) {
                m_items[i].id = m_next_id++;
                m_items[i].start_val = start_val;
                m_items[i].end_val = end_val;
                m_items[i].duration_ms = (duration_ms == 0) ? 1 : duration_ms;
                m_items[i].elapsed_ms = 0;
                m_items[i].easing = easing;
                m_items[i].on_update = on_update;
                m_items[i].on_complete = on_complete;
                m_items[i].user_data = user_data;
                m_items[i].active = true;

                if (on_update) {
                    on_update(start_val, user_data);
                }
                return m_items[i].id;
            }
        }
        return -1; // Pool exhausted
    }

    void cancel(int32_t id) {
        if (id < 0) return;
        for (size_t i = 0; i < MAX_ANIMATIONS; ++i) {
            if (m_items[i].active && m_items[i].id == id) {
                m_items[i].active = false;
                break;
            }
        }
    }

    void update(uint32_t delta_ms) {
        for (size_t i = 0; i < MAX_ANIMATIONS; ++i) {
            if (!m_items[i].active) continue;

            m_items[i].elapsed_ms += delta_ms;
            float t = (float)m_items[i].elapsed_ms / (float)m_items[i].duration_ms;
            if (t >= 1.0f) {
                t = 1.0f;
                m_items[i].active = false;
            }

            float eased = evaluate_easing(m_items[i].easing, t);
            float current = m_items[i].start_val + (m_items[i].end_val - m_items[i].start_val) * eased;

            if (m_items[i].on_update) {
                m_items[i].on_update(current, m_items[i].user_data);
            }

            if (!m_items[i].active && m_items[i].on_complete) {
                m_items[i].on_complete(m_items[i].user_data);
            }
        }
    }

    bool has_active_animations() const {
        for (size_t i = 0; i < MAX_ANIMATIONS; ++i) {
            if (m_items[i].active) return true;
        }
        return false;
    }

private:
    Animator() = default;
    AnimationItem m_items[MAX_ANIMATIONS]{};
    int32_t m_next_id{1};
};

} // namespace bos

#endif // BOS_UI_ANIMATION_HPP
