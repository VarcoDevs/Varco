#include <Document/Document.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <Utils/Concurrent.hpp>
#include <SkCanvas.h>
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

  Document::Document(CodeView& codeView)
    : UIElement(static_cast<UIElement<ui_container_tag>&>(codeView)), m_codeView(codeView)
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
    if (m_wrapWidth != width)
      m_dirty = true;
    m_wrapWidth = width;
  }  

  void Document::recalculateDocumentLines() {

    if (!m_codeView.isControlReady())
      return; // A document can be loaded at any time

    // Load parameters from codeview parent
    m_characterWidthPixels = m_codeView.getCharacterWidthPixels();
    m_characterHeightPixels = m_codeView.getCharacterHeightPixels();

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

    m_physicalLines = std::move(blockingOrderedMapReduce<std::vector<PhysicalLine>>(m_plainTextLines, mapFn, reduceFn, 30U));    
  }

  void Document::paint() {
    if (!m_dirty)
      return;

    m_dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->m_bitmap);
    SkRect rect = getRect(absoluteRect); // Drawing is performed on the bitmap - absolute rect

    canvas.save();

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 39, 40, 34));
      canvas.drawRect(rect, background);
    }    

    size_t documentRelativePos = 0;
    size_t lineRelativePos = 0;

    m_codeView.m_fontPaint.setColor(SK_ColorWHITE);

    struct {
      float x;
      float y;
    } startpoint = { 5, 20 }; // Start point where to start rendering

    // Implement the main rendering loop algorithm which renders characters segment by segment
    // on the viewport area
    for (auto& pl : m_physicalLines) {

      size_t editorLineIndex = 0; // This helps tracking the last EditorLine of a PhysicalLine
      for (auto& el : pl.m_editorLines) {
        ++editorLineIndex;

        do {
          startpoint.x = 5.f + lineRelativePos * m_characterWidthPixels;


          // Multiple lines will have to be rendered, just render this till the end and continue

          int charsRendered = 0;
          if (el.m_characters.size() > 0) { // Empty lines must be skipped
            std::string ts(el.m_characters.data() + lineRelativePos, static_cast<int>(el.m_characters.size() - lineRelativePos));

            canvas.drawText(ts.data(), ts.size(), startpoint.x, startpoint.y, m_codeView.m_fontPaint);
            //painter.drawText(tpoint, ts);
            charsRendered = (int)ts.size();
          }

          lineRelativePos = 0; // Next editor line will just start from the beginning
          documentRelativePos += charsRendered + /* Plus a newline if a physical line ended (NOT an EditorLine) */
            (editorLineIndex == pl.m_editorLines.size() ? 1 : 0);

          break; // Go and fetch a new line for the next cycle

        } while (true);

        // Move the rendering cursor (carriage-return)
        startpoint.y = startpoint.y + m_codeView.m_characterHeightPixels;
      }
    }

    canvas.flush();
  }

}