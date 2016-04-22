#include <UI/CodeView/CodeView.hpp>
#include <SkCanvas.h>
#include <SkTypeface.h>

namespace varco {

#define VSCROLLBAR_WIDTH 20

  CodeView::CodeView(UIContainer& parentWindow)
    : UICtrlBase(parentWindow)
  {
    // Create the vertical scrollbar
    m_verticalScrollBar = std::make_unique<ScrollBar>(*this);

    // Create a monospace typeface
    m_typeface = SkTypeface::CreateFromName("Consolas", SkTypeface::kNormal); // Or closest match

    // Stores the width of a single character in pixels with the given font (cache this value for
    // every document to use it)
    auto lol = m_typeface->getBounds().width();

    {
      SkPaint paintTemp;
      paintTemp.setTextSize(SkIntToScalar(13));
      paintTemp.setAntiAlias(true);
      paintTemp.setLCDRenderText(true);
      paintTemp.setTypeface(m_typeface);
      SkScalar width = paintTemp.measureText("A", 1);
      m_characterWidthPixels = static_cast<int>(width);
    }
  }

  void CodeView::resize(SkRect rect) {
    UICtrlBase::resize(rect);

    SkRect scrollBarRect = SkRect::MakeLTRB((SkScalar)(m_rect.fRight - VSCROLLBAR_WIDTH),
      0, (SkScalar)m_rect.fRight, m_rect.fBottom);
    m_verticalScrollBar->resize(scrollBarRect);

    m_codeViewInitialized = true;

    if (m_document != nullptr && static_cast<int>(rect.width()) != m_document->m_wrapWidth) {
      m_document->setWrapWidth(static_cast<int>(rect.width()));
      m_document->recalculateDocumentLines();
    }
  }

  void CodeView::loadDocument(Document& doc) {
    m_document = &doc;

    // Calculate the new document size
    m_document->recalculateDocumentLines();

    // Update the document rendering dimensions
    auto lineHeight = m_typeface->getBounds().height();

    // Emit a documentSizeChanged signal. This will trigger scrollbars 'maxViewableLines' calculations
    m_verticalScrollBar->documentSizeChanged(m_document->m_maximumCharactersLine, m_document->m_numberOfEditorLines, lineHeight);

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

    //  this->viewport()->repaint(); // Trigger a cache invalidation for the viewport (necessary)
  }

  int CodeView::getCharacterWidthPixels() const {
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

  }

}