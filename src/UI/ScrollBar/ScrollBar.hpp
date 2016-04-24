#ifndef VARCO_SCROLLBAR_HPP
#define VARCO_SCROLLBAR_HPP

#include <UI/UIElement.hpp>

namespace varco {

  class CodeView;

  class ScrollBar : public UIElement<ui_control_tag> {
  public:
    ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView);

    void paint() override;

    void setLineHeightPixels(SkScalar height);
    void documentSizeChanged(const int width_in_characters, const int height_in_lines);

  private:
    UIElement<ui_container_tag, ui_control_tag>& m_parentControlContainer;

    int  m_maxViewVisibleLines; // Lines that the current view of the text control can visualize
    int  m_internalLineCount;   // Real lines of the document (not multiplied by m_textLineHeight)

    int  m_maximum; // Last document line index we're allowed to scroll to
    int  m_value = 0;   // Current document line index we're at

    SkScalar  m_lineHeightPixels;

    int m_lenSlider;
  };

}

#endif // VARCO_SCROLLBAR_HPP
