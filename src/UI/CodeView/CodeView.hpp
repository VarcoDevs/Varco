#ifndef VARCO_CODEEDITCTRL_HPP
#define VARCO_CODEEDITCTRL_HPP

#include <UI/UIElement.hpp>
#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <Utils/Concurrent.hpp>
#include <Utils/Interpolators.hpp>
#include <SkPaint.h>
#include <memory>

class SkTypeface;

namespace varco {

  class DocumentManager;

  class CodeView : public UIElement<ui_container_tag, ui_control_tag> { // The main code edit control
  public:
    CodeView(UIElement<ui_container_tag>& parentContainer);
    ~CodeView();

    void resize(SkRect rect) override;
    void paint() override;
    void repaint() override;

    void startMouseCapture() override;
    void stopMouseCapture() override;

    void loadDocument(Document& doc, SkScalar vScrollbarPos = 0);
    SkScalar getCharacterWidthPixels() const;
    SkScalar getCharacterHeightPixels() const;
    SkPaint::FontMetrics getFontMetrics() const;
    bool isControlReady() const;
    bool isTrackingActive() const;

    // These might also forward the event to a scrollbar if present
    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseUp(SkScalar x, SkScalar y);
    void onMouseWheel(SkScalar x, SkScalar y, int direction);
    
    void setViewportYOffset(SkScalar value);

  private:
    friend class DocumentManager;
    friend class Document;

    Document *m_document = nullptr;
    std::unique_ptr<ScrollBar> m_verticalScrollBar;    

    inline void setVScrollbarValue(SkScalar value) {
      m_verticalScrollBar->m_value = value;
    }
    inline SkScalar getVScrollbarValue() {
      return m_verticalScrollBar->m_value;
    }

    sk_sp<SkTypeface> m_typeface; // Font used throughout the control
    SkPaint m_fontPaint; // Paint data for the font
    int m_textSize = 14;

    SkScalar m_characterWidthPixels, m_characterHeightPixels;
    SkPaint::FontMetrics m_fontMetrics; // Font metrics for advanced rendering
    int m_wrapWidthInPixels = 0;
    bool m_codeViewInitialized = false; // This control is initialized and ready to render
                                        // documents as soon as the first resize happens

    InterpolationSequence m_caretInterpolatorSequence;

    SkScalar m_currentYoffset = 0; // Y offset percentage in the current document (also the line we're at)

    ThreadPool m_threadPool;
  };

}

#endif // VARCO_CODEEDITCTRL_HPP
