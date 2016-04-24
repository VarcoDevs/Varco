#include <UI/CodeView/CodeView.hpp>
#include <SkCanvas.h>
#include <SkTypeface.h>

namespace varco {

#define VSCROLLBAR_WIDTH 20

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
    m_verticalScrollBar = std::make_unique<ScrollBar>(*this);
    m_verticalScrollBar->setLineHeightPixels(m_characterHeightPixels);
  }

  void CodeView::resize(SkRect rect) {
    UIElement::resize(rect);

    // Recalculate and resize scrollbar control
    SkRect scrollBarRect = SkRect::MakeLTRB((SkScalar)(m_rect.fRight - VSCROLLBAR_WIDTH),
                                            0, (SkScalar)m_rect.fRight, m_rect.fBottom);
    m_verticalScrollBar->resize(scrollBarRect);

    m_codeViewInitialized = true; // From now on we have valid buffer and size

    // Calculate new wrap width (allow space for the vertical scrollbar if present)
    auto newWrapWidth = static_cast<int>(rect.width() - (m_verticalScrollBar ? (VSCROLLBAR_WIDTH * 2) : 0));

    // If we have a document and we need to recalculate the wrapwidth
    if (m_document != nullptr && newWrapWidth != m_document->m_wrapWidth) {
      
      m_document->setWrapWidth(newWrapWidth);      
      m_document->recalculateDocumentLines();

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

  bool CodeView::isControlReady() const {
    return m_codeViewInitialized;
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
      m_verticalScrollBar->paint();
      canvas.drawBitmap(m_verticalScrollBar->getBitmap(), m_verticalScrollBar->getRect().fLeft, 
                        m_verticalScrollBar->getRect().fTop);
    }

    if (m_document == nullptr)
      return; // Nothing to display

    struct {
      float x;
      float y;
    } startpoint = { 5, 20 };

    size_t documentRelativePos = 0;
    size_t lineRelativePos = 0;

    m_fontPaint.setColor(SK_ColorWHITE);

    // Implement the main rendering loop algorithm which renders characters segment by segment
    // on the viewport area
    for (auto& pl : m_document->m_physicalLines) {

      size_t editorLineIndex = 0; // This helps tracking the last EditorLine of a PhysicalLine
      for (auto& el : pl.m_editorLines) {
        ++editorLineIndex;

        do {
          startpoint.x = 5.f + lineRelativePos * m_characterWidthPixels;


            // Multiple lines will have to be rendered, just render this till the end and continue

            int charsRendered = 0;
            if (el.m_characters.size() > 0) { // Empty lines must be skipped
              std::string ts(el.m_characters.data() + lineRelativePos, static_cast<int>(el.m_characters.size() - lineRelativePos));

              canvas.drawText(ts.data(), ts.size(), startpoint.x, startpoint.y, m_fontPaint);
              //painter.drawText(tpoint, ts);
              charsRendered = (int)ts.size();
            }

            lineRelativePos = 0; // Next editor line will just start from the beginning
            documentRelativePos += charsRendered + /* Plus a newline if a physical line ended (NOT an EditorLine) */
              (editorLineIndex == pl.m_editorLines.size() ? 1 : 0);

            break; // Go and fetch a new line for the next cycle

        } while (true);

        // Move the rendering cursor (carriage-return)
        startpoint.y = startpoint.y + m_characterHeightPixels;
      }
    }


    canvas.flush();

  }

  void CodeView::repaint() {
    // Signals the container to repaint
    m_parentContainer.repaint();
  }

}
