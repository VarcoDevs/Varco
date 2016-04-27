#include <UI/CodeView/CodeView.hpp>
#include <Utils/Utils.hpp>
#include <SkCanvas.h>
#include <SkTypeface.h>
#include <algorithm>

namespace varco {

#define VSCROLLBAR_WIDTH 15

  CodeView::CodeView(UIElement<ui_container_tag>& parentContainer)
    : UIElement(parentContainer)
  {
    // Create a monospace typeface
    m_typeface = SkTypeface::CreateFromName("Consolas", SkTypeface::kNormal); // Or closest match

    // Stores the width of a single character in pixels with the given font (cache this value for
    // every document to use it) and the height of a line in pixels, plus the paint used for each of them
    m_fontPaint.setTextSize(SkIntToScalar(13));
    m_fontPaint.setAntiAlias(true);
    m_fontPaint.setLCDRenderText(true);
    m_fontPaint.setTypeface(m_typeface);

    // The following is a conservative approach including kerning, hinting and antialiasing (a bit too much)
    //SkRect bounds;
    //m_fontPaint.measureText("A", 1, &bounds);
    //m_characterWidthPixels = static_cast<int>(bounds.width());
    
    SkScalar widths[1];
    m_fontPaint.getTextWidths("A", 1, widths);
    m_characterWidthPixels = widths[0];

    m_characterHeightPixels = m_fontPaint.getFontSpacing();

    // Create the vertical scrollbar
    m_verticalScrollBar = std::make_unique<ScrollBar>(*this, [&](SkScalar value) {
      setViewportYOffset(value);
    });
    m_verticalScrollBar->setLineHeightPixels(m_characterHeightPixels);
  }

  void CodeView::resize(SkRect rect) {
    UIElement::resize(rect);

    // Recalculate and resize scrollbar control
    SkRect scrollBarRect = SkRect::MakeLTRB(m_rect.fRight - (SkScalar)VSCROLLBAR_WIDTH,
                                            0.f, m_rect.fRight, m_rect.height());
    m_verticalScrollBar->resize(scrollBarRect);

    m_codeViewInitialized = true; // From now on we have valid buffer and size

    // Calculate new wrap width (allow space for the vertical scrollbar if present)
    m_wrapWidthInCharacters = static_cast<int>(rect.width() - (m_verticalScrollBar ? (VSCROLLBAR_WIDTH * 2) : 0));

    // If we have a document and we need to recalculate the wrapwidth
    if (m_document != nullptr && m_wrapWidthInCharacters != m_document->m_wrapWidth) {
      
      m_document->setWrapWidth(m_wrapWidthInCharacters);
      m_document->recalculateDocumentLines();

      // Resize the document bitmap to fit the new render that will take place
      SkRect newRect = SkRect::MakeLTRB(0, 0, m_wrapWidthInCharacters * getCharacterWidthPixels() + 5.f,
                                        m_document->m_numberOfEditorLines * getCharacterHeightPixels() + 20.f);
      m_document->resize(newRect);

      // Emit a documentSizeChanged signal. This will trigger scrollbars 'maxViewableLines' calculations
      m_verticalScrollBar->documentSizeChanged(m_document->m_maximumCharactersLine, 
                                               m_document->m_numberOfEditorLines);
    }
  }

  void CodeView::loadDocument(Document& doc) {

    m_document = &doc; // Save this document's address as the current one

    if (isControlReady() == false)
      return; // We can 

    // Calculate new wrap width (allow space for the vertical scrollbar if present)
    auto newWrapWidth = static_cast<int>(m_rect.width() - (m_verticalScrollBar ? (VSCROLLBAR_WIDTH * 2) : 0));
    m_document->setWrapWidth(newWrapWidth);
    // Calculate the new document size
    m_document->recalculateDocumentLines();

    // Resize the document bitmap to fit the new render that will take place
    SkRect newRect = SkRect::MakeLTRB(0, 0, m_wrapWidthInCharacters * getCharacterWidthPixels() + 5.f,
                                      m_document->m_numberOfEditorLines * getCharacterHeightPixels() + 20.f);
    m_document->resize(newRect);

    // Emit a documentSizeChanged signal. This will trigger scrollbars 'maxViewableLines' calculations
    m_verticalScrollBar->documentSizeChanged(m_document->m_maximumCharactersLine, 
                                             m_document->m_numberOfEditorLines);

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

  void CodeView::onLeftMouseMove(SkScalar x, SkScalar y) {
    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    if (m_verticalScrollBar && 
        (m_verticalScrollBar->isTrackingActive() || isPointInsideRect(relativeToParentCtrl.x(), relativeToParentCtrl.y(), 
                                                                      m_verticalScrollBar->getRect(relativeToParentRect))))
      m_verticalScrollBar->onLeftMouseMove(relativeToParentCtrl.x(), relativeToParentCtrl.y());
  }

  void CodeView::onLeftMouseUp(SkScalar x, SkScalar y) {
    SkPoint relativeToParentCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);

    if (m_verticalScrollBar &&
        (m_verticalScrollBar->isTrackingActive() || isPointInsideRect(relativeToParentCtrl.x(), relativeToParentCtrl.y(),
                                                                      m_verticalScrollBar->getRect(relativeToParentRect))))
      m_verticalScrollBar->onLeftMouseUp(relativeToParentCtrl.x(), relativeToParentCtrl.y());
  }

  // Change the Y offset to the specified one from the beginning of a document (receives a percentage of the total document size)
  void CodeView::setViewportYOffset(SkScalar percentage) {
    m_currentYPercentage = percentage;
    m_dirty = true;
    m_parentContainer.repaint();
  }

  void CodeView::paint() {

    if (!m_dirty)
      return;

    m_dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->m_bitmap);
    SkRect rect = getRect(absoluteRect); // Drawing is performed on the bitmap - absolute rect

    canvas.save();

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
    SkScalar verticalScrollbarWidth = 0.f;
    if (m_verticalScrollBar)
      verticalScrollbarWidth = m_verticalScrollBar->getRect(absoluteRect).width();

    auto documentYoffset = (m_currentYPercentage ) * m_document->m_characterHeightPixels;

    SkRect viewportRect = SkRect::MakeLTRB(getRect(absoluteRect).fLeft, getRect(absoluteRect).fTop,
                                           getRect(absoluteRect).fRight - verticalScrollbarWidth, getRect(absoluteRect).fBottom);

    SkRect documentViewRect = SkRect::MakeLTRB(0.f, documentYoffset, std::min(viewportRect.width(), m_document->getRect(absoluteRect).width()),
                                               std::min(documentYoffset + viewportRect.height(), m_document->getRect(absoluteRect).fBottom));
    viewportRect.fBottom = std::min(documentViewRect.height(), viewportRect.height());

    // Start bitblitting from the current requested line position
    canvas.drawBitmapRect(m_document->getBitmap(), documentViewRect, viewportRect, nullptr);
    //canvas.drawBitmap(m_document->getBitmap(), m_document->getRect(absoluteRect).fLeft,
    //                  m_verticalScrollBar->getRect(absoluteRect).fTop + documentYoffset);
    

    canvas.flush();

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
