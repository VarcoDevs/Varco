#ifndef VARCO_CODEEDITCTRL_HPP
#define VARCO_CODEEDITCTRL_HPP

#include <UI/UIElement.hpp>
#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
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

    void loadDocument(Document& doc);
    SkScalar getCharacterWidthPixels() const;
    bool isControlReady() const;

  private:

    Document *m_document = nullptr;
    std::unique_ptr<ScrollBar> m_verticalScrollBar;

    SkTypeface *m_typeface = nullptr; // Font used throughout the control
    SkPaint m_fontPaint; // Paint data for the font

    SkScalar m_characterWidthPixels, m_characterHeightPixels;
    bool m_codeViewInitialized = false; // This control is initialized and ready to render
                                                             // documents as soon as the first resize happens
  };

}

#endif // VARCO_CODEEDITCTRL_HPP
