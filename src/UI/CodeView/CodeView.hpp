#ifndef VARCO_CODEEDITCTRL_HPP
#define VARCO_CODEEDITCTRL_HPP

#include <UI/UIElement.hpp>
#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <Utils/Concurrent.hpp>
#include <SkPaint.h>
#include <memory>

class SkTypeface;

namespace varco {

  class CodeView : public UIElement<ui_container_tag, ui_control_tag> { // The main code edit control
  public:
    CodeView(UIElement<ui_container_tag>& parentContainer);

    void resize(SkRect rect) override;
    void paint() override;
    void repaint() override;

    void startMouseCapture() override;
    void stopMouseCapture() override;

    void loadDocument(Document& doc);
    SkScalar getCharacterWidthPixels() const;
    SkScalar getCharacterHeightPixels() const;
    bool isControlReady() const;
    bool isTrackingActive() const;

    // These might also forward the event to a scrollbar if present
    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onLeftMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseUp(SkScalar x, SkScalar y);
    
    void setViewportYOffset(SkScalar value);

  private:
    friend class Document;

    Document *m_document = nullptr;
    std::unique_ptr<ScrollBar> m_verticalScrollBar;

    SkTypeface *m_typeface = nullptr; // Font used throughout the control
    SkPaint m_fontPaint; // Paint data for the font

    SkScalar m_characterWidthPixels, m_characterHeightPixels;
    int m_wrapWidthInPixels = 0;
    bool m_codeViewInitialized = false; // This control is initialized and ready to render
                                        // documents as soon as the first resize happens

    SkScalar m_currentYoffset = 0; // Y offset percentage in the current document

    ThreadPool<15> m_threadPool;
  };

}

#endif // VARCO_CODEEDITCTRL_HPP
