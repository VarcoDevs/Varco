#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>

namespace varco {

  ScrollBar::ScrollBar(UIContainer& codeView)
    : UICtrlBase(codeView),
      m_textLineHeight(1), // Height of every line, it depends on the font used (although it's always monospaced)
      m_internalLineCount(1) // How many lines are actually in the document
  {}

  // Emitted when the document changes size, it is the only way to detect the number of lines in the document if wrapping is active
  void ScrollBar::documentSizeChanged(const int width, const int height, const SkScalar lineHeight) {

    m_textLineHeight = lineHeight; // textLine.height();
                                   // Update the maximum number of visible lines in the text control, this might have changed
    //m_maxViewVisibleLines = qFloor(qreal(m_parent->viewport()->height()) / m_textLineHeight);

  }

  void ScrollBar::paint() {

    if (!m_dirty)
      return;

    m_dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->m_bitmap);

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the bar (alpha only)
    //////////////////////////////////////////////////////////////////////
    canvas.clear(0x00000000); // ARGB

    
    // >> ---------------------------------------------------------------------
    //    Calculate the position, length and drawing area for the slider
    // --------------------------------------------------------------------  <<

    // extraBottomLines are virtual lines to let the last line of text be scrollable till it is left as the only one above in the view.
    // Thus they correspond to the maximum number of lines that the view can visualize - 1 (the one I want is excluded)
    int extraBottomLines = (m_maxViewVisibleLines - 1);
  }

}