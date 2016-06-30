#ifndef VARCO_SCROLLBAR_HPP
#define VARCO_SCROLLBAR_HPP

#include <UI/UIElement.hpp>
#include <SkPath.h>
#include <functional>

namespace varco {

  class ScrollBar;

  class Slider {
  private:
    friend class ScrollBar;
    Slider(ScrollBar& parent);

    ScrollBar& m_parent;
    SkPath   m_path; // The path where clicks and inputs are accepted
    SkScalar m_length;
    SkRect   m_rect;
  };

  class CodeView;

  class ScrollBar : public UIElement<ui_control_tag> {
  public:
    ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView, std::function<void(SkScalar)> sliderChangeCallback);

    void paint() override;

    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onLeftMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseUp(SkScalar x, SkScalar y);
    void onMouseWheel(SkScalar x, SkScalar y, int direction);

    void setLineHeightPixels(SkScalar height);
    void documentSizeChanged(const int width_in_characters, const int height_in_lines);

    bool isTrackingActive() const;

  private:
    UIElement<ui_container_tag, ui_control_tag>& m_parentControlContainer;

    Slider m_slider;
    std::function<void(SkScalar)> m_sliderChangeCallback;
    bool m_sliderIsBeingDragged = false;
    SkPoint m_mouseTrackingStartPoint;
    SkScalar m_mouseTrackingStartValue;

    int  m_maxViewVisibleLines; // Lines that the current view of the text control can visualize
    int  m_internalLineCount;   // Real lines of the document (not multiplied by m_textLineHeight)

    int  m_maximum; // Last document line index we're allowed to scroll to
    SkScalar  m_value = 0.f;   // Current document offset we're at (or where the slider is from the start)

    SkScalar  m_lineHeightPixels;
  };

}

#endif // VARCO_SCROLLBAR_HPP
