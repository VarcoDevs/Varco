#ifndef VARCO_CPPLEXER_H
#define VARCO_CPPLEXER_H

#include <Lexers/Lexer.hpp>
#include <unordered_set>
#include <string>
#include <stack>

namespace varco {

  class CPPLexer : public LexerBase {
  public:
    CPPLexer();

    void reset() override;
    void lexInput(std::string input, StyleDatabase& sdb) override;

  private:
    //// States the lexer can find itself into
    //enum LexerStates {CODE, STRING, COMMENT, MULTILINECOMMENT, INCLUDE};
    //LexerStates m_state;
    std::unordered_set<std::string> m_reservedKeywords;
    std::stack<int> m_scopesStack;
    int m_classKeywordActiveOnScope; // This signals that there's a 'class' keyword pending
    std::vector<int> m_adaptPreviousSegments;

    // The contents of the document and the position we're lexing at
    std::string *str;
    size_t pos;
    size_t curLine;
    StyleDatabase *styleDb;

    void addSegment(size_t line, size_t pos, size_t len, Style style);
    void incrementLineNumberIfNewline(size_t pos);

    void classDeclarationOrDefinition();
    void declarationOrDefinition();
    void defineStatement();
    void lineCommentStatement();
    void usingStatement();
    void includeStatement();
    void multilineComment();
    void globalScope();

  };

}

#endif // VARCO_CPPLEXER_H