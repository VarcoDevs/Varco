#include <Document/Document.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <Utils/Concurrent.hpp>
#include <fstream>
#include <regex>
#include <functional>

namespace varco {

  EditorLine::EditorLine(std::string str) {
    if (str.empty())
      return;
    m_characters.resize(str.length());
    std::copy(str.begin(), str.end(), m_characters.begin());
  }

  Document::Document(const CodeView& codeView)
    : m_codeView(codeView)
  {}

  // The following function loads the contents of a text file into memory.
  // This is a memory-expensive operation but documents need to be available at any time
  // Returns true on success
  bool Document::loadFromFile(std::string file) {
    
    std::ifstream f(file);
    if (!f.is_open())
      return false;

    // Load the entire file into memory
    std::string line;
    while (getline(f, line)) {

      // This is also necessary: normalize all line endings to \n (Unix-style)
      std::regex invalidEndings("\r\n|\r");
      line = std::regex_replace(line, invalidEndings, "\n");

      // For simplicity convert all tabs into 4 spaces and just deal with those
      std::regex tabs("\t");
      line = std::regex_replace(line, tabs, "    ");

      m_plainTextLines.push_back(std::move(line));
    }
    f.close();

    return true;
  }

  void Document::setWrapWidth(int width) {
    m_wrapWidth = width;
  }

  void Document::recalculateDocumentLines() {

    if (!m_codeView.isControlReady())
      return; // A document can be loaded at any time

    // TODO lexing

    // Drop previous lines
    m_physicalLines.clear();
    m_numberOfEditorLines = 0;
    m_maximumCharactersLine = 0;

    std::function<std::vector<PhysicalLine>(const std::string&)> mapFn = [&, this](const std::string& line) {

      std::vector<PhysicalLine> phLineVec;
      std::string restOfLine;
      std::vector<EditorLine> edLines;
      edLines.reserve(10); // Should be enough for every splitting
      if (m_wrapWidth != -1 && // Also check if the monospace'd width isn't exceeding the viewport
        line.size() * m_codeView.getCharacterWidthPixels() > m_wrapWidth) {
        // We have a wrap and the line is too big - WRAP IT

        edLines.clear();
        restOfLine = line;

        // Calculate the allowed number of characters per editor line
        int maxChars = static_cast<int>(m_wrapWidth / m_codeView.getCharacterWidthPixels());
        if (maxChars < 10)
          maxChars = 10; // Keep it to a minimum

                         // Start the wrap-splitting algorithm or resort to a brute-force character splitting one if
                         // no suitable spaces could be found to split the line
        while (restOfLine.size() > maxChars) {

          int bestSplittingPointFound = -1;
          for (int i = 0; i < restOfLine.size(); ++i) {
            if (i > maxChars)
              break; // We couldn't find a suitable space split point for restOfLine
            if (restOfLine[i] == ' ' && i != 0 /* Doesn't make sense to split at 0 pos */)
              bestSplittingPointFound = i;
          }

          if (bestSplittingPointFound != -1) { // We found a suitable space point to split this line
            edLines.push_back(restOfLine.substr(0, bestSplittingPointFound));
            restOfLine = restOfLine.substr(bestSplittingPointFound);
          }
          else {
            // No space found, brutally split characters (last resort)
            edLines.push_back(restOfLine.substr(0, maxChars));
            restOfLine = restOfLine.substr(maxChars);
          }
        }
        edLines.push_back(restOfLine); // Insert the last part and proceed

        // No need to do anything special for tabs - they're automatically converted into spaces

        std::vector<EditorLine> edVector;
        std::copy(edLines.begin(), edLines.end(), std::back_inserter(edVector));
        phLineVec.resize(1);
        phLineVec[0].m_editorLines = std::move(edVector);

      }
      else { // No wrap or the line fits perfectly within the wrap limits

        EditorLine el(line);
        phLineVec.emplace_back(std::move(el));
      }
      return phLineVec;
    };


    std::function<void(std::vector<PhysicalLine>&, const std::vector<PhysicalLine>&)> reduceFn =
      [&, this](std::vector<PhysicalLine>& accumulator, const std::vector<PhysicalLine>& pl) {

      accumulator.insert(accumulator.end(), pl.begin(), pl.end());

      m_numberOfEditorLines += static_cast<int>(pl[0].m_editorLines.size()); // Some more EditorLine
      std::for_each(pl[0].m_editorLines.begin(), pl[0].m_editorLines.end(), [this](const EditorLine& eline) {
        int lineLength = static_cast<int>(eline.m_characters.size());
        if (lineLength > m_maximumCharactersLine) // Check if this is the longest line found ever
          m_maximumCharactersLine = lineLength;
      });

    };

    m_physicalLines = blockingOrderedMapReduce<std::vector<PhysicalLine>>(m_plainTextLines, mapFn, reduceFn, 50U);
  }

  // TODO multithread calculations

}