#ifndef VARCO_SCROLLBAR_HPP
#define VARCO_SCROLLBAR_HPP

#include <UI/UICtrlBase.hpp>

namespace varco {

  class CodeView;

  class ScrollBar : public UICtrlBase {
  public:
    ScrollBar(UIContainer& codeView);

    void paint() override;

    void documentSizeChanged(const int width, const int height, const SkScalar lineHeight);

  private:
    int  m_maxViewVisibleLines; // Lines that the current view of the text control can visualize
    SkScalar m_textLineHeight;
    int   m_internalLineCount; // Real lines of the code control (not multiplied by m_textLineHeight)
  };

}

#endif // VARCO_SCROLLBAR_HPP
