#ifndef BOS_UI_CONTROLS_CARD_HPP
#define BOS_UI_CONTROLS_CARD_HPP

#include "bos/widget.hpp"
#include "bos/icon.hpp"
#include "bos/theme.hpp"

namespace bos {

class Card : public Widget {
public:
    Card();
    explicit Card(const char* title, const Icon& icon = Icon());
    ~Card() override = default;

    const char* title() const { return m_title; }
    void set_title(const char* title);

    const char* subtitle() const { return m_subtitle; }
    void set_subtitle(const char* subtitle);

    const Icon& icon() const { return m_icon; }
    void set_icon(const Icon& icon);

    void set_background_color(Color bg) { m_bg_color = bg; invalidate(); }
    void set_border_color(Color bdr) { m_border_color = bdr; invalidate(); }

    void set_9slice_background(const Image& img, const Insets& borders) {
        m_bg_image = img;
        m_slice_borders = borders;
        invalidate();
    }

    void paint(Surface& surface) override;
    Size measure_preferred_size() const override;

private:
    char     m_title[64]{0};
    char     m_subtitle[64]{0};
    Icon     m_icon{};
    Color    m_bg_color{Theme::SurfaceCard()};
    Color    m_border_color{Theme::Border()};
    Image    m_bg_image{};
    Insets   m_slice_borders{0};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_CARD_HPP
