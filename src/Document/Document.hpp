#ifndef VARCO_DOCUMENT_HPP
#define VARCO_DOCUMENT_HPP

namespace varco {

  // TODO document classes

  class CodeEditCtrl;

  class Document {
  public:
    Document(CodeEditCtrl& codeTextEdit);

  private:
    CodeEditCtrl& codeTextEdit;
  };

}

#endif // VARCO_DOCUMENT_HPP
