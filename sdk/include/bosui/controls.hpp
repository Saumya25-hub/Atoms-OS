#ifndef BOSUI_CONTROLS_HPP
#define BOSUI_CONTROLS_HPP

#include "framework/include/bos_ui_controls.h"
#include "framework/include/bos_ui_layout.h"

namespace bosui {

class Control {
public:
    virtual ~Control() = default;
    virtual BOS_UIElement* GetNativeElement() const = 0;
};

class Button : public Control {
public:
    Button(const char* text, void (*on_click)(BOS_UIElement*) = nullptr) {
        m_btn = BOS_Button_Create(text, on_click);
    }
    BOS_UIElement* GetNativeElement() const override { return (BOS_UIElement*)m_btn; }
private:
    BOS_Button* m_btn{nullptr};
};

class Label : public Control {
public:
    Label(const char* text) {
        m_lbl = BOS_Label_Create(text);
    }
    BOS_UIElement* GetNativeElement() const override { return (BOS_UIElement*)m_lbl; }
private:
    BOS_Label* m_lbl{nullptr};
};

class TextBox : public Control {
public:
    TextBox(const char* placeholder) {
        m_tb = BOS_TextBox_Create(placeholder);
    }
    BOS_UIElement* GetNativeElement() const override { return (BOS_UIElement*)m_tb; }
private:
    BOS_TextBox* m_tb{nullptr};
};

class Grid : public Control {
public:
    Grid() {
        m_grid = BOS_Grid_Create();
    }
    void AddRow(float val, BOS_GridUnitType unit = BOS_GRID_UNIT_PIXEL) {
        BOS_Grid_AddRow(m_grid, val, unit);
    }
    void AddColumn(float val, BOS_GridUnitType unit = BOS_GRID_UNIT_PIXEL) {
        BOS_Grid_AddColumn(m_grid, val, unit);
    }
    void AddChild(Control* control, uint32_t row, uint32_t col) {
        if (control) {
            BOS_Grid_SetCell(m_grid, control->GetNativeElement(), row, col);
        }
    }
    BOS_UIElement* GetNativeElement() const override { return (BOS_UIElement*)m_grid; }
private:
    BOS_Grid* m_grid{nullptr};
};

} // namespace bosui

#endif /* BOSUI_CONTROLS_HPP */
