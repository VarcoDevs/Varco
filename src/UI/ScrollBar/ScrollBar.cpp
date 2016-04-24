#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>

namespace varco {

  ScrollBar::ScrollBar(UIElement<ui_container_tag>& codeView)
    : UIElement(codeView),
      m_textLineHeight(1), // Height of every line, it depends on the font used (although it's always monospaced)
      m_internalLineCount(1) // How many lines are actually in the document
  {}

  // Emitted when the document changes size, it is the only way to detect the number of lines in the document if wrapping is active
  void ScrollBar::documentSizeChanged(const int width_characters, const int height_lines, const SkScalar lineHeight) {

    m_textLineHeight = lineHeight; // textLine.height();
    

//    auto codeViewHeight = static_cast<UICtrlBase>(m_parentContainer).getRect(absoluteRect).height();
    // Update the maximum number of visible lines in the text control, this might have changed
 //   m_maxViewVisibleLines = std::floor(codeViewHeight / m_textLineHeight);

  }

  // The fundamental equation to repaint the scrollbar is:
  //  slider_length = maximum_slider_height * (how_many_lines_I_can_display_in_the_view / total_number_of_lines_in_the_document)
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

    // Since maximum() is ALWAYS greater than value() (the number of lines in the control is always greater
    // or equal to the line we're scrolled at), the position of the slider is:
    //   slider_position = view_height * (line_where_the_view_is_scrolled / total_number_of_lines_in_the_document)
    // in this case we calculate a "relative" position by using value() and maximum() which are relative to the control (not to the document)
//    float viewRelativePos = float(m_maxViewVisibleLines) * (float(value()) / float(maximum() + (extraBottomLines)));
  }

}