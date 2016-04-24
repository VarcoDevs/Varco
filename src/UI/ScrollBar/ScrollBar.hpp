#ifndef VARCO_SCROLLBAR_HPP
#define VARCO_SCROLLBAR_HPP

#include <UI/UIElement.hpp>

namespace varco {

  class CodeView;

  class ScrollBar : public UIElement<ui_control_tag> {
  public:
    ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView);

    void paint() override;

    void documentSizeChanged(const int width, const int height, const SkScalar lineHeight);

  private:
    UIElement<ui_container_tag, ui_control_tag>& m_parentControlContainer;

    int  m_maxViewVisibleLines; // Lines that the current view of the text control can visualize
    SkScalar m_textLineHeight;
    int   m_internalLineCount; // Real lines of the code control (not multiplied by m_textLineHeight)
  };

}

#endif // VARCO_SCROLLBAR_HPP
