#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>
#include <SkPaint.h>

namespace varco {

  ScrollBar::ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView)
    : m_parentControlContainer(codeView), UIElement(codeView),
      m_lineHeightPixels(1), // Height of every line, it depends on the font used (although it's always monospaced)
      m_internalLineCount(1) // How many lines are actually in the document
  {}

  void ScrollBar::setLineHeightPixels(SkScalar height) {
    m_lineHeightPixels = height;
  }

  // Emitted when the document changes size, it is the only way to detect the number of lines in the document if wrapping is active
  void ScrollBar::documentSizeChanged(const int width_in_characters, const int height_in_line) {
    
    auto codeViewHeight = m_parentControlContainer.getRect(absoluteRect).height();
    // Update the maximum number of visible lines in the text control, this might have changed
    m_maxViewVisibleLines = static_cast<int>(codeViewHeight / m_lineHeightPixels);

    // Store the real number of lines in the document
    m_internalLineCount = height_in_line;

    // Also update the maximum allowed to let the last line to be scrolled till the beginning of the view
    m_maximum = m_internalLineCount - 1;
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
    canvas.clear(0xFF000000); // ARGB

    
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
    float viewRelativePos = float(m_maxViewVisibleLines) * (float(m_value) / float(m_maximum + extraBottomLines));

    // now find the absolute position in the control's rect, the proportion is:
    //  rect().height() : x = m_maxViewVisibleLines : viewRelativePos
    float rectAbsPos = (float(m_rect.height()) * viewRelativePos) / float(m_maxViewVisibleLines);

    // Calculate the length of the slider's rect including extraBottomLines
    m_lenSlider = int(m_rect.height() * (float(m_maxViewVisibleLines) / float(m_internalLineCount + extraBottomLines)));

    // Set a mimumim length for the slider (when there are LOTS of lines)
    if (m_lenSlider < 15)
      m_lenSlider = 15;

    // Prevents the slider to be drawn, due to roundoff errors, outside the scrollbar rectangle
    if (rectAbsPos + m_lenSlider > m_rect.height())
      rectAbsPos -= (rectAbsPos + m_lenSlider) - m_rect.height();

    // This is finally the drawing area for the slider
    SkRect rcSlider = SkRect::MakeLTRB(0.f, rectAbsPos, m_rect.width() - 1.f, (SkScalar)m_lenSlider);

    SkPaint simpleRed;
    simpleRed.setColor(SK_ColorRED);
    canvas.drawRect(rcSlider, simpleRed);
    // p.fillRect( rcSlider, QColor( 55, 4, 255, 100 ) );

    canvas.flush();
  }

}