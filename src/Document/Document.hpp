#ifndef VARCO_DOCUMENT_HPP
#define VARCO_DOCUMENT_HPP

#include <UI/UIElement.hpp>
#include <Lexers/Lexer.hpp>
#include <vector>
#include <string>

namespace varco {

  // Two classes help to organize a document to be displayed and are set up by a document
  // recalculate operation:
  //
  // PhysicalLine {
  //   A physical line corresponds to a real file line (until a newline character)
  //   it should have 0 (only a newline) or more EditorLine
  //
  //   EditorLine {
  //     An editor line is a line for the editor, i.e. a line that might be the result
  //     of wrapping or be equivalent to a physical line. EditorLine stores the characters
  //   }
  // }

  struct EditorLine {
    EditorLine(std::string str);

    std::vector<char> m_characters;
  };

  struct PhysicalLine {
    PhysicalLine(EditorLine editorLine) {
      m_editorLines.emplace_back(std::move(editorLine));
    }
    PhysicalLine(std::vector<EditorLine>&& editorLines) {
      m_editorLines = std::forward<std::vector<EditorLine>>(editorLines);
    }
    PhysicalLine(const PhysicalLine&) = default;
    PhysicalLine() = default;

    std::vector<EditorLine> m_editorLines;
  };

  enum SyntaxHighlight { NONE, CPP };

  class CodeView;

  class Document : public UIElement<ui_control_tag> {
  public:
    Document(CodeView& codeView);    

    bool loadFromFile(std::string file);
    void applySyntaxHighlight(SyntaxHighlight s);

  private:
    friend class CodeView;

    void setWrapWidthInPixels(int width);    
    void recalculateDocumentLines();

    void paint() override; // Renders the entire document on its bitmap

    CodeView& m_codeView;
    std::vector<std::string> m_plainTextLines;
    std::vector<PhysicalLine> m_physicalLines;

    // Variables related to how the control renders lines
    SkScalar m_characterWidthPixels;
    SkScalar m_characterHeightPixels;
    int m_wrapWidthPixels = -1;
    int m_numberOfEditorLines;
    int m_maximumCharactersLine; // According to wrapWidth   

    std::unique_ptr<LexerBase> m_lexer;
    bool m_needReLexing = false;
    bool m_firstDocumentRecalculate = true;
    StyleDatabase m_styleDb;
  };

}

#endif // VARCO_DOCUMENT_HPP
