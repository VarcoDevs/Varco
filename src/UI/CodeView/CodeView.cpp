#include <UI/CodeView/CodeView.hpp>
#include <Utils/Utils.hpp>
#include <SkCanvas.h>
#include <SkTypeface.h>
#include <algorithm>

#include <sstream> // DEBUG

namespace varco {

#define VSCROLLBAR_WIDTH 15

  CodeView::CodeView(UIElement<ui_container_tag>& parentContainer)
    : UIElement(parentContainer), m_threadPool(15)
  {
    // Create a monospace typeface
    // An alternative approach here is to ship a standard font for every/each platform
    // e.g. SkTypeface::MakeFromFile("/home/alex/Desktop/UbuntuMono-R.ttf");
    const char *font_family =
#ifdef _WIN32
      "Consolas";
#elif defined __linux__
      "monospace";
#endif
    // Try to match with the supplied font name or family name (or closest match)
    m_typeface = SkTypeface::MakeFromName(font_family, SkFontStyle{SkFontStyle::Weight::kNormal_Weight,
                                                                   SkFontStyle::Width::kNormal_Width,
                                                                   SkFontStyle::Slant::kUpright_Slant});

    // Stores the width of a single character in pixels with the given font (cache this value for
    // every document to use it) and the height of a line in pixels, plus the paint used for each of them
    m_fontPaint.setTextSize(SkIntToScalar(m_textSize));
    m_fontPaint.setAntiAlias(true);
    m_fontPaint.setAutohinted(true);
    m_fontPaint.setLCDRenderText(true);
    m_fontPaint.setTypeface(m_typeface);

    // The following is a conservative approach including kerning, hinting and antialiasing (a bit too much)
    //SkRect bounds;
    //m_fontPaint.measureText("A", 1, &bounds);
    //m_characterWidthPixels = static_cast<int>(bounds.width());
    
    SkScalar widths[1];
    m_fontPaint.getTextWidths("A", 1, widths);
    m_characterWidthPixels = widths[0];

    /*
     * Here is how the font metrics work
     *
     * ----------------------------------------- Top
     * _________________________________ Ascent
     *               C
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Baseline
     * _________________________________ Descent
     *
     * ----------------------------------------- Bottom
     * (leading)
     *
     * We're using Descent - Ascent + Leading given by getFontSpacing() to separate
     * one line from the other (as a cell). But a character should be drawn from a
     * baseline when drawText() is called, therefore we're also storing the Descent
     * in order to subtract it from the line spacing before rendering.
     *
     */
    m_fontPaint.getFontMetrics(&m_fontMetrics);
    m_characterHeightPixels = m_fontPaint.getFontSpacing();

    // Create the vertical scrollbar
    m_verticalScrollBar = std::make_unique<ScrollBar>(*this, [&](SkScalar value) {
      setViewportYOffset(value);
    });
    m_verticalScrollBar->setLineHeightPixels(m_characterHeightPixels);

    // Create the caret's alpha interpolation sequence
    // 1) Caret's alpha goes from 0 to 255 in 3 seconds
    auto phase1 = std::make_unique<LinearInterpolator>(0, 255, 3000);
    // 2) Caret stays the same for 2 seconds
    auto phase2 = std::make_unique<ConstantInterpolator>(255, 3000);
    // 3) Inverse of phase 1
    auto phase3 = std::make_unique<LinearInterpolator>(255, 0, 3000);
    m_caretInterpolatorSequence.addInterpolator(std::move(phase1));
    m_caretInterpolatorSequence.addInterpolator(std::move(phase2));
    m_caretInterpolatorSequence.addInterpolator(std::move(phase3));
    m_caretInterpolatorSequence.start();
  }

  CodeView::~CodeView() {}

  void CodeView::resize(SkRect rect) {
    UIElement::resize(rect);

    // Recalculate and resize scrollbar control
    SkRect scrollBarRect = SkRect::MakeLTRB(m_rect.fRight - (SkScalar)VSCROLLBAR_WIDTH,
                                            0.f, m_rect.fRight, m_rect.height());
    m_verticalScrollBar->resize(scrollBarRect);

    m_codeViewInitialized = true; // From now on we have valid buffer and size

    // Calculate new wrap width (allow space for the vertical scrollbar if present)
    m_wrapWidthInPixels = static_cast<int>(rect.width() - (m_verticalScrollBar ? (VSCROLLBAR_WIDTH * 2) : 0));

    // If we have a document and we need to recalculate the wrapwidth
    if (m_document != nullptr && m_wrapWidthInPixels != m_document->m_wrapWidthPixels) {
      
      m_document->setWrapWidthInPixels(m_wrapWidthInPixels);
      m_document->recalculateDocumentLines();

      // Resize the document bitmap to fit the new render that will take place
      //SkRect newRect = SkRect::MakeLTRB(0, 0, m_wrapWidthInPixels * getCharacterWidthPixels() + 5.f,
      //                                  m_document->m_numberOfEditorLines * getCharacterHeightPixels() + 20.f);
      //m_document->resize(newRect);

      // Emit a documentSizeChanged signal. This will trigger scrollbars 'maxViewableLines' calculations
      m_verticalScrollBar->documentSizeChanged(m_document->m_maximumCharactersLine, 
                                               m_document->m_numberOfEditorLines);
    }
  }

  void CodeView::loadDocument(Document& doc, SkScalar vScrollbarPos) {

    m_document = &doc; // Save this document's address as the current one

    if (isControlReady() == false)
      return; // We can't show anything if the codeview control hasn't been initialized yet    

    // Calculate new wrap width (allow space for the vertical scrollbar if present)
    auto newWrapWidth = static_cast<int>(m_rect.width() - (m_verticalScrollBar ? (VSCROLLBAR_WIDTH * 2) : 0));
    m_document->setWrapWidthInPixels(newWrapWidth);
    // Calculate the new document size
    m_document->recalculateDocumentLines();

    // Resize the document bitmap to fit the new render that will take place
    SkRect newRect = SkRect::MakeLTRB(0, 0, m_wrapWidthInPixels * getCharacterWidthPixels() + 5.f,
                                      m_document->m_numberOfEditorLines * getCharacterHeightPixels() + 20.f);
    m_document->resize(newRect);

    // Emit a documentSizeChanged signal. This will trigger scrollbars 'maxViewableLines' calculations
    m_verticalScrollBar->documentSizeChanged(m_document->m_maximumCharactersLine, 
                                             m_document->m_numberOfEditorLines);

    // If there was a saved vertical scrollbar position, also restore it, otherwise just set it to 0
    setVScrollbarValue(vScrollbarPos);
    setViewportYOffset(vScrollbarPos); // And obviously also set our viewport Y offset

    //// Set a new pixmap for rendering this document ~ caveat: this is NOT the viewport dimension
    //// since everything needs to be rendered, not just the viewport region
    //m_documentPixmap = std::make_unique<QImage>(viewport()->width(), m_document->m_numberOfEditorLines *
    //  fontMetrics().height() + 20 /* Remember to compensate the offset */,
    //  QImage::Format_ARGB32_Premultiplied);
    //m_documentMutex.unlock();

    //m_messageQueueMutex.lock();
    //m_documentUpdateMessages.emplace_back(viewport()->width(), viewport()->width(), m_document->m_numberOfEditorLines *
    //  fontMetrics().height() + 20 /* Remember to compensate the offset */);
    //m_messageQueueMutex.unlock();

    //if (m_renderingThread->isRunning() == false)
    //  m_renderingThread->start();

    //  QSizeF newSize;
    //  newSize.setHeight( m_document->m_numberOfEditorLines );
    //  newSize.setWidth ( m_document->m_maximumCharactersLine );

    //  // Emit a documentSizeChanged signal. This will trigger scrollbars resizing
    //  emit documentSizeChanged( newSize, fontMetrics().height() );

    //  m_verticalScrollBar->setSliderPosition(0);

    m_dirty = true;
    paint(); // Trigger a cache invalidation for the viewport (necessary)
  }

  SkScalar CodeView::getCharacterWidthPixels() const {
    return m_characterWidthPixels;
  }

  SkScalar CodeView::getCharacterHeightPixels() const {
    return m_characterHeightPixels;
  }

  SkPaint::FontMetrics CodeView::getFontMetrics() const {
    return m_fontMetrics;
  }

  bool CodeView::isControlReady() const {
    return m_codeViewInitialized;
  }

  // Checks if tracking is active inside the control or on one of the scrollbars
  bool CodeView::isTrackingActive() const {
    bool active = false;
    if (m_verticalScrollBar && m_verticalScrollBar->isTrackingActive())
      active = true;
    return active;
  }

  void CodeView::onLeftMouseDown(SkScalar x, SkScalar y) {
    
    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    if (m_verticalScrollBar && isPointInsideRect(relativeToParentCtrl.x(), relativeToParentCtrl.y(), m_verticalScrollBar->getRect(relativeToParentRect)))
      m_verticalScrollBar->onLeftMouseDown(relativeToParentCtrl.x(), relativeToParentCtrl.y());
  }

  void CodeView::onMouseWheel(SkScalar x, SkScalar y, int direction) {
    m_verticalScrollBar->onMouseWheel(x, y, direction);
  }

  void CodeView::onMouseMove(SkScalar x, SkScalar y) {
    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    if (m_verticalScrollBar && 
        (m_verticalScrollBar->isTrackingActive() || isPointInsideRect(relativeToParentCtrl.x(), relativeToParentCtrl.y(), 
                                                                      m_verticalScrollBar->getRect(relativeToParentRect))))
      m_verticalScrollBar->onMouseMove(relativeToParentCtrl.x(), relativeToParentCtrl.y());
  }

  void CodeView::onLeftMouseUp(SkScalar x, SkScalar y) {
    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    if (m_verticalScrollBar &&
        (m_verticalScrollBar->isTrackingActive() || isPointInsideRect(relativeToParentCtrl.x(), relativeToParentCtrl.y(),
                                                                      m_verticalScrollBar->getRect(relativeToParentRect))))
      m_verticalScrollBar->onLeftMouseUp(relativeToParentCtrl.x(), relativeToParentCtrl.y());
  }

  // Change the Y offset to the specified one from the beginning of a document (receives an offset in the total document size)
  void CodeView::setViewportYOffset(SkScalar value) {
    m_currentYoffset = value;
    m_dirty = true;
    m_parentContainer.repaint();
  }

  void CodeView::paint() {

    if (!m_dirty)
      return;

    m_dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->m_bitmap);
    SkRect rect = getRect(absoluteRect); // Drawing is performed on the bitmap - absolute rect

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 39, 40, 34));
      canvas.drawRect(rect, background);
    }

    //////////////////////////////////////////////////////////////////////
    // Draw the vertical scrollbar
    //////////////////////////////////////////////////////////////////////
    {
      if (m_verticalScrollBar) {
        m_verticalScrollBar->paint();
        canvas.drawBitmap(m_verticalScrollBar->getBitmap(), m_verticalScrollBar->getRect(relativeToParentRect).fLeft,
                          m_verticalScrollBar->getRect(relativeToParentRect).fTop);
      }
    }

    if (m_document == nullptr)
      return; // Nothing to display

    //////////////////////////////////////////////////////////////////////
    // Render and draw the requested portion of the document
    //////////////////////////////////////////////////////////////////////

    m_document->paint();

    // Only draw things which intersect the current viewport region
    auto documentYoffset = m_currentYoffset * m_document->m_characterHeightPixels;

    // Calculate source and destination rect
    SkRect bitmapPartialRect = SkRect::MakeLTRB(0, documentYoffset, this->getRect(absoluteRect).width(), documentYoffset + this->getRect(absoluteRect).height());
    SkRect myDestRect = SkRect::MakeLTRB(0, 0, this->getRect(absoluteRect).width(), this->getRect(absoluteRect).height());

    canvas.drawBitmapRect(m_document->getBitmap(), bitmapPartialRect, myDestRect, nullptr,
                          SkCanvas::SrcRectConstraint::kFast_SrcRectConstraint);

    //////////////////////////////////////////////////////////////////////
    // Draw the cursor if in sight
    //////////////////////////////////////////////////////////////////////

    auto cursorPos = m_document->m_cursorPos;

    // Is the cursor in sight?
    SkScalar firstViewVisibleLine = this->m_currentYoffset;
    SkScalar lastViewVisibleLine = firstViewVisibleLine + (this->getRect(absoluteRect).height() / m_characterHeightPixels);
    if (cursorPos.y >= firstViewVisibleLine - 1 && cursorPos.y < lastViewVisibleLine + 1) {
      SkPaint caretPaint;
      int val = m_caretInterpolatorSequence.getValue();
      caretPaint.setColor(SkColorSetARGB(255, val, val, val));
      caretPaint.setAntiAlias(true);
      SkScalar viewRelativeTopStart = (cursorPos.y - firstViewVisibleLine /* Line view-relative where the caret is at */) * m_characterHeightPixels;
      canvas.drawLine(cursorPos.x * m_characterWidthPixels + Document::BITMAP_OFFSET_X,
                      viewRelativeTopStart,
                      cursorPos.x * m_characterWidthPixels + Document::BITMAP_OFFSET_X,
                      viewRelativeTopStart + m_characterHeightPixels /* Caret length */,
                      caretPaint);
      m_dirty = true;
    }

    canvas.flush();
    if (m_dirty == true)
      m_parentContainer.repaint(); // Schedule a repaint
  }

  void CodeView::repaint() {
    // Signals the container to repaint
    m_dirty = true;
    m_parentContainer.repaint();
  }

  void CodeView::startMouseCapture() {
    m_parentContainer.startMouseCapture();
  }

  void CodeView::stopMouseCapture() {
    m_parentContainer.stopMouseCapture();
  }

}
