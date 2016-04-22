#ifndef VARCO_CODEEDITCTRL_HPP
#define VARCO_CODEEDITCTRL_HPP

#include <UI/UICtrlBase.hpp>
#include <UI/ScrollBar/ScrollBar.hpp>
#include <Document/Document.hpp>
#include <memory>

class SkTypeface;

namespace varco {

  class CodeView : public UICtrlBase, public UIContainer { // The main code edit control
  public:
    CodeView(UIContainer& parentWindow);

    void resize(SkRect rect) override;
    void paint() override;

    void loadDocument(Document& doc);
    int getCharacterWidthPixels() const;
    bool isControlReady() const;

  private:

    Document *m_document = nullptr;
    std::unique_ptr<ScrollBar> m_verticalScrollBar;
    SkTypeface *m_typeface = nullptr; // Font used throughout the control
    int m_characterWidthPixels;
    bool m_codeViewInitialized = false; // This control is initialized and ready to render
                                      // documents as soon as the first resize happens
  };

}

#endif // VARCO_CODEEDITCTRL_HPP
