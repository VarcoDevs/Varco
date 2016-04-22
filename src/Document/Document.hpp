#ifndef VARCO_DOCUMENT_HPP
#define VARCO_DOCUMENT_HPP

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
    PhysicalLine(EditorLine&& editorLine) {
      m_editorLines.emplace_back(std::forward<EditorLine>(editorLine));
    }
    PhysicalLine() = default;

    std::vector<EditorLine> m_editorLines;
  };

  class CodeView;

  class Document {
  public:
    Document(const CodeView& codeView);

    bool loadFromFile(std::string file);

  private:
    friend class CodeView;

    void setWrapWidth(int width);
    void recalculateDocumentLines();

    const CodeView& m_codeView;
    std::vector<std::string> m_plainTextLines;
    std::vector<PhysicalLine> m_physicalLines;

    // Variables related to how the control renders lines
    int m_characterWidthPixels;
    int m_wrapWidth;
    int m_numberOfEditorLines;
    int m_maximumCharactersLine; // According to wrapWidth
  };

}

#endif // VARCO_DOCUMENT_HPP
