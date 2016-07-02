#include <UI/ScrollBar/ScrollBar.hpp>
#include <Utils/Utils.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkGradientShader.h>

namespace varco {


  Slider::Slider(ScrollBar& parent)
    : m_parent(parent) {}

  ScrollBar::ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView, std::function<void(SkScalar)> sliderChangeCallback)
    : UIElement(static_cast<UIElement<ui_container_tag>&>(codeView)),
      m_parentControlContainer(codeView),
      m_slider(*this),
      m_sliderChangeCallback(std::move(sliderChangeCallback)),
      m_lineHeightPixels(1), // Height of every line, it depends on the font used (although it's always monospaced)
      m_internalLineCount(1) // How many lines are actually in the document
  {}

  void ScrollBar::setLineHeightPixels(SkScalar height) {
    m_lineHeightPixels = height;
  }

  // Emitted when the document changes size, it is the only way to detect the number of lines in the document if wrapping is active
  void ScrollBar::documentSizeChanged(const int width_in_characters, const int height_in_lines) {
    
    auto codeViewHeight = m_parentControlContainer.getRect(absoluteRect).height();
    // Update the maximum number of visible lines in the text control, this might have changed
    m_maxViewVisibleLines = static_cast<int>(codeViewHeight / m_lineHeightPixels);

    // Store the real number of lines in the document
    m_internalLineCount = height_in_lines;

    // Also update the maximum allowed to let the last line to be scrolled till the beginning of the view
    m_maximum = m_internalLineCount - 1;
    m_value = clamp(m_value, 0.f, static_cast<SkScalar>(m_maximum));

    m_dirty = true; // Control needs to be redrawn and slider position to be recalculated
  }

  bool ScrollBar::isTrackingActive() const {
    return m_sliderIsBeingDragged;
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

    SkRect scrollbarRect = this->getRect(absoluteRect);

    // extraBottomLines are virtual lines to let the last line of text be scrollable till it is left as the only one above in the view.
    // Thus they correspond to the maximum number of lines that the view can visualize - 1 (the one I want is excluded)
    int extraBottomLines = (m_maxViewVisibleLines - 1);

    // Since maximum() is ALWAYS greater than value() (the number of lines in the control is always greater
    // or equal to the line we're scrolled at), the position of the slider is:
    //   slider_position = view_height * (line_where_the_view_is_scrolled / total_number_of_lines_in_the_document)
    // in this case we calculate a "relative" position by using value() and maximum() which are relative to the control (not to the document)
    float viewRelativePos = float(m_maxViewVisibleLines) * (m_value / float(m_maximum + 1 + extraBottomLines));

    // now find the absolute position in the control's rect, the proportion is:
    //  rect().height() : x = m_maxViewVisibleLines : viewRelativePos
    float rectAbsPos = (scrollbarRect.height() * viewRelativePos) / float(m_maxViewVisibleLines);

    // Calculate the length of the slider's rect including extraBottomLines
    m_slider.m_length = scrollbarRect.height() * (float(m_maxViewVisibleLines) / float(m_internalLineCount + extraBottomLines));

    // Set a mimumim length for the slider (when there are LOTS of lines)
    if (m_slider.m_length < 15.f)
      m_slider.m_length = 15.f;

    // This is finally the drawing area for the slider (store it for hit tests)
    m_slider.m_rect = SkRect::MakeLTRB(0.f, scrollbarRect.fTop + rectAbsPos, scrollbarRect.width() - 1.f, 
                                       scrollbarRect.fTop + rectAbsPos + m_slider.m_length);

    // A separation line of 1 px
    {
      SkPaint line;
      line.setColor(SkColorSetARGB(255, 29, 29, 29));
      line.setStrokeWidth(1.0);      
      canvas.drawLine(scrollbarRect.fLeft + 1.f, scrollbarRect.fTop, scrollbarRect.fLeft + 1.f,
                      scrollbarRect.fBottom, line);
    }

    // Soft background gradient from sx to dx
    {
      SkPaint backgroundGradient;
      SkPoint leftToRightPoints[2] = {
        SkPoint::Make(scrollbarRect.fLeft, scrollbarRect.fTop),
        SkPoint::Make(scrollbarRect.fRight, scrollbarRect.fTop),
      };
      SkColor colors[2] = { SkColorSetARGB(255, 33, 33, 33), SkColorSetARGB(255, 50, 50, 50) };
      auto shader = SkGradientShader::MakeLinear(leftToRightPoints, colors, NULL, 2, SkShader::kClamp_TileMode, 0, NULL);
      backgroundGradient.setShader(shader);
      backgroundGradient.setStyle(SkPaint::kFill_Style); // Fill with the background color
      SkRect rect = scrollbarRect;
      rect.fLeft += 1.f;
      canvas.drawRect(rect, backgroundGradient);
    }

    // >> ------------------------
    //       Slider drawing
    // -----------------------  <<

    // Draws the slider with a rounded rectangle
    // m_rect is the hitbox, but to draw it we only take a width subsection
    {
      SkRect rcSliderSubsection(m_slider.m_rect);
      rcSliderSubsection.fLeft += 3.f;
      rcSliderSubsection.fRight -= 2.f;
      m_slider.m_path.reset();
      m_slider.m_path.addRoundRect(rcSliderSubsection, 4.f, 4.f);
    }

    // Select a gradient brush to fill the slider
    SkPaint fillGradient;
    {
      SkPoint leftToRightPoints[2] = {
        SkPoint::Make(m_slider.m_rect.fLeft, m_slider.m_rect.fTop),
        SkPoint::Make(m_slider.m_rect.fRight, m_slider.m_rect.fTop),
      };
      SkColor colors[2] = { SkColorSetARGB(255, 88, 88, 88), SkColorSetARGB(255, 64, 64, 64) };
      auto fillGrad = SkGradientShader::MakeLinear(leftToRightPoints, colors, NULL, 2, SkShader::kClamp_TileMode, 0, NULL);
      fillGradient.setShader(fillGrad);
      fillGradient.setAntiAlias(true);
      fillGradient.setStyle(SkPaint::kFill_Style); // Fill with the background color
    }

    canvas.drawPath(m_slider.m_path, fillGradient);

    canvas.flush();
  }

  void ScrollBar::onLeftMouseDown(SkScalar x, SkScalar y) {

    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    // Detect if the click was on the slider
    if (m_sliderIsBeingDragged == false && 
        m_slider.m_path.contains(relativeToParentCtrl.x(), relativeToParentCtrl.y()) == true) 
    {
      m_sliderIsBeingDragged = true;
      m_mouseTrackingStartPoint = relativeToParentCtrl;
      m_mouseTrackingStartValue = m_value;
      m_parentContainer.startMouseCapture();
    } else {
      // A click was detected but NOT on the slider. Move the slider at the click position
      SkScalar y = relativeToParentCtrl.y() * m_internalLineCount / getRect().height();
      m_value = clamp(y, 0.f, static_cast<SkScalar>(m_maximum));
      m_sliderChangeCallback((SkScalar(m_value) / SkScalar(m_maximum)) * (m_internalLineCount - 1)); // Signal to the parent that slider has changed
    }
    m_dirty = true;
    m_parentContainer.repaint();
  }

  // The logic here is the following: if we're tracking the slider, calculate the delta from the tracking start
  // point and scale it not according to the scrollbar area rectangle, but to the number of lines of the document
  // (at rect.y == 0 we're at line 0, at rect.y == max we're at line lastLine). Finally add this value to the
  // slider position that was stored before the tracking (so that we're not modifying its position)
  void ScrollBar::onLeftMouseMove(SkScalar x, SkScalar y) {    
    
    if (m_sliderIsBeingDragged == true) {
      SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);      

      // It is important to note that even if the mouse did a complete document tracking (i.e. from line 0 to the last
      // one), its path's length would NOT be equal to the control rect().height() since the slider's length would have
      // to be subtracted. This can be easily visualized on a piece of paper. The right proportion is therefore:
      // x : m_internalLineCount = delta : (this->rect().height() - sliderLength)
      auto delta = (relativeToParentCtrl.y() - m_mouseTrackingStartPoint.y());
      int y = static_cast<int>(delta * m_internalLineCount / (getRect(absoluteRect).height() - m_slider.m_length));
      m_value = clamp(m_mouseTrackingStartValue + y, 0.f, static_cast<SkScalar>(m_maximum));
      m_sliderChangeCallback((m_value / m_maximum) * m_internalLineCount); // Signal to the parent that slider has changed
    }

    m_dirty = true;
    m_parentContainer.repaint();
  }

  void ScrollBar::onLeftMouseUp(SkScalar x, SkScalar y) {
    if (m_sliderIsBeingDragged == true) {
      m_sliderIsBeingDragged = false;
      m_parentContainer.stopMouseCapture();
    }
  }

  void ScrollBar::onMouseWheel(SkScalar x, SkScalar y, int direction) {
    if (direction == 0) return;

    if (direction == 1) // up
      m_value = clamp(m_value + 5, 0.f, static_cast<SkScalar>(m_maximum));
    else if (direction == -1) // down
      m_value = clamp(m_value - 5, 0.f, static_cast<SkScalar>(m_maximum));

    m_sliderChangeCallback((m_value / static_cast<SkScalar>(m_maximum)) * (m_internalLineCount - 1)); // Signal to the parent that slider has changed

    m_dirty = true;
    m_parentContainer.repaint();
  }

}
