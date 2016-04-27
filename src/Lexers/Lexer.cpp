#include <Lexers/Lexer.hpp>
#include <Lexers/CPPLexer.hpp>

namespace varco {

  LexerBase* LexerBase::createLexerOfType(LexerType t) {
    switch (t) {
    case CPPLexerType: {
      return new CPPLexer();
    } break;
    default:
      return nullptr;
    }
  }

}