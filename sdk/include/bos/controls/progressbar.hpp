#ifndef BOS_UI_CONTROLS_PROGRESSBAR_HPP
#define BOS_UI_CONTROLS_PROGRESSBAR_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class ProgressBar : public Widget {
public:
    ProgressBar();
    ProgressBar(int32_t min, int32_t max, int32_t val);
    ~ProgressBar() override = default;

    int32_t value() const { return m_val; }
    void set_value(int32_t val);

    int32_t min_value() const { return m_min; }
    int32_t max_value() const { return m_max; }
    void set_range(int32_t min, int32_t max);

    bool shows_percentage() const { return m_show_percentage; }
    void set_show_percentage(bool show) { m_show_percentage = show; invalidate(); }

    void paint(Surface& surface) override;
    Size measure_preferred_size() const override;

private:
    int32_t m_min{0};
    int32_t m_max{100};
    int32_t m_val{0};
    bool    m_show_percentage{true};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_PROGRESSBAR_HPP
