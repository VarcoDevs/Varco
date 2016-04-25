#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkGradientShader.h>

namespace varco {


  Slider::Slider(ScrollBar& parent)
    : m_parent(parent) {}

  ScrollBar::ScrollBar(UIElement<ui_container_tag, ui_control_tag>& codeView)
    : m_parentControlContainer(codeView), UIElement(codeView),
      m_slider(*this),
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

    m_dirty = true; // Control needs to be redrawn and slider position to be recalculated
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
    float viewRelativePos = float(m_maxViewVisibleLines) * (float(m_value) / float(m_maximum + extraBottomLines));

    // now find the absolute position in the control's rect, the proportion is:
    //  rect().height() : x = m_maxViewVisibleLines : viewRelativePos
    float rectAbsPos = (float(m_rect.height()) * viewRelativePos) / float(m_maxViewVisibleLines);

    // Calculate the length of the slider's rect including extraBottomLines
    m_slider.m_lenSlider = m_rect.height() * (float(m_maxViewVisibleLines) / float(m_internalLineCount + extraBottomLines));

    // Set a mimumim length for the slider (when there are LOTS of lines)
    if (m_slider.m_lenSlider < 15.f)
      m_slider.m_lenSlider = 15.f;

    // Prevents the slider to be drawn, due to roundoff errors, outside the scrollbar rectangle
    if (rectAbsPos + m_slider.m_lenSlider > m_rect.height())
      rectAbsPos -= (rectAbsPos + m_slider.m_lenSlider) - m_rect.height();

    // This is finally the drawing area for the slider (store it for hit tests)
    m_slider.m_sliderRect = SkRect::MakeLTRB(0.f, rectAbsPos, m_rect.width() - 1.f, m_slider.m_lenSlider);    

    SkRect scrollbarRect = this->getRect(absoluteRect);

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
    // m_sliderRect is the hitbox, but to draw it we only take a width subsection
    {
      SkRect rcSliderSubsection(m_slider.m_sliderRect);
      rcSliderSubsection.fLeft += 3.f;
      rcSliderSubsection.fRight -= 2.f;
      m_slider.m_path.reset();
      m_slider.m_path.addRoundRect(rcSliderSubsection, 4.f, 4.f);
    }

    // Select a gradient brush to fill the slider
    SkPaint fillGradient;
    {
      SkPoint leftToRightPoints[2] = {
        SkPoint::Make(m_slider.m_sliderRect.fLeft, m_slider.m_sliderRect.fTop),
        SkPoint::Make(m_slider.m_sliderRect.fRight, m_slider.m_sliderRect.fTop),
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
    if (m_slider.m_path.contains(relativeToParentCtrl.x(), relativeToParentCtrl.y()) == true) {
      Beep(200, 200);
    }


    //bool redrawNeeded = false;
    //for (auto i = 0; i < tabs.size(); ++i) {
    //  Tab& tab = tabs[i];

    //  SkPoint relativeToTab = SkPoint::Make(relativeToTabCtrl.x() - tab.getOffset(), relativeToTabCtrl.y());

    //  if (tab.getPath().contains(relativeToTab.x(), relativeToTab.y()) == true) {

    //    if (tab.getMovementOffset() != 0)
    //      return; // No dragging when returning to home position

    //    if (selectedTabIndex != i) { // Left click on the already selected tab won't trigger a "selected check"          
    //      redrawNeeded = true;

    //      if (selectedTabIndex != -1) // Deselect old selected tab
    //        tabs[selectedTabIndex].setSelected(false);

    //      selectedTabIndex = i;
    //      tab.setSelected(true);
    //    }

    //    m_tracking = true;
    //    m_startXTrackingPosition = x;
    //    m_parentContainer.startMouseCapture();
    //  }
    //}

    //if (redrawNeeded) {
    //  this->m_dirty = true;
    //  m_parentContainer.repaint();
    //}
  }

  void ScrollBar::onLeftMouseMove(SkScalar x, SkScalar y) {
    /*if (!m_tracking)
      return;

    tabs[selectedTabIndex].trackingOffset = x - m_startXTrackingPosition;
    tabs[selectedTabIndex].dirty = true;
    m_dirty = true;
    m_parentContainer.repaint();*/
  }

  void ScrollBar::onLeftMouseUp(SkScalar x, SkScalar y) {
    /*stopTracking();*/
  }

}