#ifndef VARCO_DOCUMENT_HPP
#define VARCO_DOCUMENT_HPP

#include <UI/UIElement.hpp>
#include <Lexers/Lexer.hpp>
#include <Utils/Concurrent.hpp>
#include <vector>
#include <string>
#include <future>

namespace varco {

  class CodeView;

  class Document : public UIElement<ui_control_tag> {
  public:
    Document(CodeView& codeView);    

    bool loadFromFile(std::string file);
    void applySyntaxHighlight(SyntaxHighlight s);

  private:
    friend class CodeView;

    void setWrapWidthInPixels(int width);    
    void scheduleRender();
    void collectResult(std::shared_ptr<ThreadRequest> request);

    void paint() override; // Renders the entire document on its bitmap

    // The document is offset by these amounts when rendered to avoid
    // having it too attached to the borders
    static constexpr const float BITMAP_OFFSET_X = 5.f;
    static constexpr const float BITMAP_OFFSET_Y = 0.f;

    CodeView& m_codeView;
    int m_wrapWidthPixels = -1;
    int m_numberOfEditorLines;
    int m_maximumCharactersLine; // According to wrapWidth
    SkScalar m_characterWidthPixels;
    SkScalar m_characterHeightPixels;

    std::mutex m_documentMutex;
    StyleDatabase m_latestStyleDb;
    std::vector<std::string> m_plainTextLines;
    std::vector<PhysicalLine> m_physicalLines;

    std::unique_ptr<LexerBase> m_lexer;
    bool m_needReLexing = false;
    bool m_firstDocumentRecalculate = true;

    struct {
      int x = 0;
      int y = 0;
    } m_cursorPos; // Latest known cursor position
    
    void threadProcessChunk(size_t threadIdx, std::shared_ptr<ThreadRequest> data);
  };

}

#endif // VARCO_DOCUMENT_HPP
