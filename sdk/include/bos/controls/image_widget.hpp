#ifndef BOS_UI_CONTROLS_IMAGE_WIDGET_HPP
#define BOS_UI_CONTROLS_IMAGE_WIDGET_HPP

#include "bos/widget.hpp"
#include "bos/resource.hpp"

namespace bos {

enum class ImageFit {
    Stretch,
    Center,
    Contain,
    Cover
};

class ImageWidget : public Widget {
public:
    ImageWidget();
    explicit ImageWidget(const Image& image, ImageFit fit = ImageFit::Stretch);
    ~ImageWidget() override = default;

    const Image& image() const { return m_image; }
    void set_image(const Image& image);

    ImageFit fit_mode() const { return m_fit; }
    void set_fit_mode(ImageFit fit) { m_fit = fit; invalidate(); }

    uint32_t corner_radius() const { return m_radius; }
    void set_corner_radius(uint32_t r) { m_radius = r; invalidate(); }

    void set_border(Color bdr, uint32_t thick = 1) {
        m_border_color = bdr;
        m_border_thick = thick;
        invalidate();
    }

    void paint(Surface& surface) override;
    Size measure_preferred_size() const override;

private:
    Image    m_image{};
    ImageFit m_fit{ImageFit::Stretch};
    uint32_t m_radius{0};
    Color    m_border_color{Color::Transparent()};
    uint32_t m_border_thick{0};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_IMAGE_WIDGET_HPP
