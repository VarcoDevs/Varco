#include <Document/Document.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <Utils/Concurrent.hpp>
#include <SkCanvas.h>
#include <fstream>
#include <regex>
#include <functional>

// DEBUG
#include "timerClass.h"
#include <iostream>
#include <fstream>

namespace {
  template<typename T> // Can't move this into Utils.hpp due to MSVC ICE
  void moveAppendVector(std::vector<T>& dst, std::vector<T>& src) {
    if (dst.empty())
      dst = std::move(src);
    else {
      dst.reserve(dst.size() + src.size());
      std::move(std::begin(src), std::end(src), std::back_inserter(dst));
      src.clear();
    }
  }
}

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

  void Document::setWrapWidthInPixels(int width) {
    if (m_wrapWidthPixels != width)
      m_dirty = true;
    m_wrapWidthPixels = width;
  }

  void Document::applySyntaxHighlight(SyntaxHighlight s) {
    m_needReLexing = false;
    switch (s) {
    case NONE: {
      if (m_lexer) { // Check if there were a lexer before (i.e. the smart pointer was set)
        m_lexer.release();
        m_needReLexing = true; // Syntax has been changed, re-lex the document at the next recalculate
      }
    } break;
    case CPP: {
      if (!m_lexer || isLexer(m_lexer.get()).ofType(CPPLexerType) == false) {
        m_lexer.reset(LexerBase::createLexerOfType(CPPLexerType));
        m_needReLexing = true; // Syntax has been changed, re-lex the document at the next recalculate
      }
    } break;
    }


    // If the document was already recalculated, just recalculate and apply the new syntax highlight
    if (m_firstDocumentRecalculate == false)
      recalculateDocumentLines();
  }

#define MAX_WRAPS_PER_LINE 10

  void Document::threadProcessChunk(size_t threadIdx) {

      // Process a chunk of data
      if (threadIdx >= m_numThreads)
        return;

      // Calculate per-thread work bounds
      size_t start = threadIdx * m_linesPerThread;
      size_t end;
      if (m_numThreads == 1)
        end = m_plainTextLines.size();
      else
        end = std::min(m_plainTextLines.size(), start + m_linesPerThread);

      // Precalculate the allowed number of characters per editor line
      int maxChars = static_cast<int>(m_wrapWidthPixels / m_codeView.getCharacterWidthPixels());
      if (maxChars < 10)
        maxChars = 10; // Keep it to a minimum

      SkScalar bitmapEffectiveHeight = 0; // This is NOT know before the computation
      SkScalar bitmapEffectiveWidth = 0;

      struct {
        float x;
        float y;
      } startpoint = { 5, 20 }; // Start point where to start rendering
      bitmapEffectiveHeight += startpoint.y;

      SkBitmap bitmap; // Allocate partial rendering result (maximum size)
      bitmap.allocPixels(SkImageInfo::Make((int)(m_wrapWidthPixels + startpoint.x),
        (int)((end - start) * MAX_WRAPS_PER_LINE * m_codeView.getCharacterHeightPixels()),
        kN32_SkColorType, kPremul_SkAlphaType));

      bitmapEffectiveWidth = m_wrapWidthPixels + startpoint.x;

      SkCanvas canvas(bitmap);
      SkRect rect = SkRect::MakeIWH(bitmap.width(), bitmap.height()); // Drawing is performed on the bitmap - absolute rect      

      { // Draw partial bitmap background
        SkPaint background;
        background.setColor(SkColorSetARGB(255, 39, 40, 34));
        canvas.drawRect(rect, background);
      }

      // Set up thread-only painter
      SkPaint painter;
      painter.setTextSize(SkIntToScalar(13));
      painter.setAntiAlias(true);
      painter.setLCDRenderText(true);
      painter.setTypeface(m_codeView.m_typeface);
      painter.setColor(SK_ColorWHITE);

      auto setColor = [&painter](Style s) {
        switch (s) {
          case Comment: {
            painter.setColor(SkColorSetARGB(255, 117, 113, 94)); // Gray-ish 
          } break;
          case Keyword: {
            painter.setColor(SkColorSetARGB(255, 249, 38, 114)); // Pink-ish
          } break;
          case QuotedString: {
            painter.setColor(SkColorSetARGB(255, 230, 219, 88)); // Yellow-ish
          } break;
          case Identifier: {
            painter.setColor(SkColorSetARGB(255, 166, 226, 46)); // Green-ish
          } break;
          case KeywordInnerScope:
          case FunctionCall: {
            painter.setColor(SkColorSetARGB(255, 102, 217, 239)); // Light blue
          } break;
          case Literal: {
            painter.setColor(SkColorSetARGB(255, 174, 129, 255)); // Purple-ish
          } break;
          default: {
            painter.setColor(SK_ColorWHITE);
          } break;
        };
      };

      auto styleEnd = m_styleDb.styleSegment.end();
      auto currentStyleIt = m_styleDb.styleSegment.begin();
      bool currentlyInSegment = false;

      // Find first style for the first line to process (if any)
      {
        auto previousSegmentIndex = m_styleDb.previousSegment[start];
        if (previousSegmentIndex == -1) {
          // There was no segment before this line, check if there's one beginning right here at character 0,
          // otherwise it means no segment was *ever* present and we switch to normal style
          auto firstIt = m_styleDb.styleSegment.begin();
          if (firstIt != styleEnd && firstIt->line == start && firstIt->start == 0) {
            // A segment begins right at the first line (pos == 0) that we have to process, get it
            setColor(firstIt->style);
            currentlyInSegment = true;
            currentStyleIt = firstIt;
          } else
            setColor(Normal);
        } else {
          // There was a previous segment, that doesn't mean its style still lasts here, we have to check
          auto previousStyle = m_styleDb.styleSegment.begin() + previousSegmentIndex;
          if (previousStyle->absStartPos + previousStyle->count > m_styleDb.m_absOffsetWhereLineBegins[start]) {
            // Yes, it still lasts
            currentStyleIt = previousStyle;
            currentlyInSegment = true;
            setColor(previousStyle->style);
          } else 
            setColor(Normal);
        }
      }

      auto getFirstSegmentOnLine = [&](size_t line) {
        auto res = m_styleDb.firstSegmentOnLine.find(line);
        if (res == m_styleDb.firstSegmentOnLine.end())
          return styleEnd;
        else
          return m_styleDb.styleSegment.begin() + res->second;
      };

      auto renderEditorLine = [&](EditorLine& el, size_t currentPhysicalLine, size_t physicalLineOffset)
      {
        const size_t editorLineSize = el.m_characters.size();

        if (editorLineSize == 0) // Do not render empty lines
          return;

        {
          std::unique_lock<std::mutex> syncBarrier;
          if (editorLineSize > m_maximumCharactersLine) // Check if this is the longest line found ever
            m_maximumCharactersLine = (int)editorLineSize;
        }

        startpoint.x = 5.f;

        size_t charsRendered = 0;
        size_t absPosition = m_styleDb.m_absOffsetWhereLineBegins[currentPhysicalLine] + physicalLineOffset;

        do {

          if (charsRendered >= editorLineSize)
            break; // We rendered everything on this line

          //
          // Set the current style (if any)
          //

          if (currentStyleIt != styleEnd) {
            if (currentStyleIt->line == currentPhysicalLine && currentStyleIt->start <= physicalLineOffset + charsRendered &&
                currentStyleIt->start + currentStyleIt->count > physicalLineOffset + charsRendered) 
            {
              currentlyInSegment = true;
              setColor(currentStyleIt->style);
            } else
              currentlyInSegment = false;
            // Is there a segment which starts exactly where we are or do we stick with the previous one already set?
            auto nextSegment = currentStyleIt + 1;
            while (nextSegment != styleEnd && nextSegment->absStartPos == absPosition) {
              currentStyleIt = nextSegment; // Set this as the active one
              setColor(currentStyleIt->style);
              currentlyInSegment = true;
              ++nextSegment;
            }
          }
          
          //
          // Calculate next position to reach
          //

          size_t nextPosToReach = 0;          
          // Three things can happen here:
          //  1) We have a segment and our current one ends before the end of the line OR our editor line ends before our segment ends
          //  3) We don't have a segment and there's no one left
          //  4) We don't have a segment and we reach either the end of the line or a new segment (whatever comes first)

          if (currentlyInSegment && currentStyleIt != styleEnd) { // Handles 1) and 2)
            nextPosToReach = std::min(editorLineSize, currentStyleIt->absStartPos + currentStyleIt->count - absPosition);
          } else { // Handles 3) and 4)
            auto seg = getFirstSegmentOnLine(currentPhysicalLine);
            if (seg == styleEnd)
              nextPosToReach = editorLineSize; // No other segments ever
            else {
              // Try to find the next segment from this position onward on this very line
              while (seg != styleEnd && seg->line == currentPhysicalLine && seg->start < physicalLineOffset + charsRendered)
                ++seg;

              if (seg == styleEnd || seg->line != currentPhysicalLine)
                nextPosToReach = editorLineSize; // No other segments
              else {
                if (seg->start == physicalLineOffset + charsRendered) {
                  // Segment starts right here, get it
                  currentlyInSegment = true;
                  currentStyleIt = seg;
                  setColor(seg->style);
                  continue; // We will still have to find a valid goal position..
                } else {
                  nextPosToReach = std::min(editorLineSize, seg->absStartPos - absPosition);
                }
              }
                
            }
          }

          //
          // Finally draw the text
          //
          std::string ts(el.m_characters.data() + charsRendered, nextPosToReach - charsRendered);


          if (ts.find("protected") != std::string::npos)
            printf("LOL");

          canvas.drawText(ts.data(), ts.size(), startpoint.x, startpoint.y, painter);
          charsRendered += ts.size();
          startpoint.x += m_characterWidthPixels * ts.size();

          //
          // Update the state before continuing
          //
          if (currentlyInSegment && nextPosToReach == currentStyleIt->start + currentStyleIt->count) {
            ++currentStyleIt; // Segment has been exhausted
            currentlyInSegment = false;
            setColor(Normal);
          }

        } while (true);
      };

      std::vector<PhysicalLine> phLineVec;
      std::string line;
      for (size_t i = start; i < end; ++i) {

        line = m_plainTextLines[i];

        std::string restOfLine;
        std::vector<EditorLine> edLines;
        edLines.reserve(MAX_WRAPS_PER_LINE); // Should be enough for every splitting

                                             // Check if the monospace'd width isn't exceeding the viewport
        if (line.size() * m_codeView.getCharacterWidthPixels() > m_wrapWidthPixels) {
          // We have a wrap and the line is too big - WRAP IT

          edLines.clear();
          restOfLine = line;

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

          size_t physicalLineOffset = 0;
          for (auto& el : edLines) {
            renderEditorLine(el, i, physicalLineOffset);
            physicalLineOffset += el.m_characters.size();

            // Move the rendering cursor (carriage-return)
            startpoint.y += m_characterHeightPixels;
            bitmapEffectiveHeight += m_characterHeightPixels;
          }

          phLineVec.emplace_back(std::move(edLines));

        } else { // No wrap or the line fits perfectly within the wrap limits

          EditorLine el(line);

          renderEditorLine(el, i, 0);

          phLineVec.emplace_back(std::move(el)); // Save it
        }

        // Move the rendering cursor (carriage-return)
        startpoint.y += m_characterHeightPixels;
        bitmapEffectiveHeight += m_characterHeightPixels;
      }

      // Time to fulfill the promise
      {
        std::unique_lock<std::mutex> lock(syncBarrier);
        m_totalBitmapHeight += bitmapEffectiveHeight;
        m_maxBitmapWidth = std::max(m_maxBitmapWidth, bitmapEffectiveWidth);
        m_partials[threadIdx].set_value(
          std::make_tuple<std::vector<PhysicalLine>, SkBitmap, SkScalar, SkScalar>(
            std::move(phLineVec), std::move(bitmap), std::move(bitmapEffectiveWidth), std::move(bitmapEffectiveHeight))
        );
      }      
  }


  void Document::recalculateDocumentLines() {

    if (!m_codeView.isControlReady())
      return; // A document can be loaded at any time

    m_firstDocumentRecalculate = false;

    if (m_needReLexing) {
      std::string m_plainText;
      for (auto& line : m_plainTextLines) { // Expensive, hopefully this doesn't happen too often - LEX DIRECTLY FROM VECTOR
        m_plainText.append(line);
        m_plainText += '\n';
      }
      m_lexer->lexInput(std::move(m_plainText), m_styleDb);
      m_needReLexing = false;
    }

    // Load parameters from codeview parent
    m_characterWidthPixels = m_codeView.getCharacterWidthPixels();
    m_characterHeightPixels = m_codeView.getCharacterHeightPixels();


    // Subdivide the document's lines into a suitable amount of workload per thread
    size_t numThreads = m_codeView.m_threadPool.m_NThreads;
    size_t minLinesPerThread = 20u;

    while (true) {
      m_linesPerThread = static_cast<size_t>(
        std::ceil(m_plainTextLines.size() / static_cast<float>(numThreads))
        );
      if (m_linesPerThread < minLinesPerThread) {
        numThreads /= 2;
        if (numThreads < 1)
          numThreads = 1;
        else if (numThreads > 1)
          continue;
      }
      break;
    }

    m_numThreads = numThreads;
    m_partials.clear();
    m_futures.clear();
    m_partials.resize(numThreads);
    m_futures.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
      m_futures.emplace_back(m_partials[i].get_future());

    
    m_totalBitmapHeight = 0;
    m_maxBitmapWidth = 0;
    m_maximumCharactersLine = 0;

    m_codeView.m_threadPool.setCallback(std::bind(&Document::threadProcessChunk, this, std::placeholders::_1));


    m_codeView.m_threadPool.dispatch();

    
    for (auto& el : m_futures) { // Wait for the threads to finish and collect futures
      el.wait();
    }

    // Resize the document bitmap to fit the new render that will take place

    SkRect bitmapRect;
    {
      std::unique_lock<std::mutex> lock(syncBarrier);
      bitmapRect = SkRect::MakeLTRB(0, 0, m_maxBitmapWidth, m_totalBitmapHeight);
      this->resize(bitmapRect);
    }

    m_physicalLines.clear();
    

    SkCanvas canvas(this->m_bitmap);

    // Draw background for the entire document
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 39, 40, 34));
      canvas.drawRect(bitmapRect, background);
    }

    
    
    SkScalar yOffset = 0;
    m_numberOfEditorLines = 0;
    for (auto& fut : m_futures) {

      auto data = std::move(fut.get());
      SkBitmap partialBitmap = std::move(std::get<1>(data));

      m_numberOfEditorLines += (int)std::get<0>(data).size();
      
      moveAppendVector<PhysicalLine>(m_physicalLines, std::get<0>(data));
      
      // Calculate source and destination rect
      SkRect partialRect = SkRect::MakeLTRB(0, 0, std::get<2>(data), std::get<3>(data));
      SkRect documentDestRect = SkRect::MakeLTRB(0, yOffset,
        std::get<2>(data), (yOffset + std::get<3>(data)));

      canvas.drawBitmapRect(partialBitmap, partialRect, documentDestRect, nullptr, SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint);
      yOffset += (size_t)std::get<3>(data);
    }

 

    m_dirty = false;


   






    //m_firstDocumentRecalculate = false;

    //if (m_needReLexing) {
    //  std::string m_plainText;
    //  for (auto& line : m_plainTextLines) { // Expensive, hopefully this doesn't happen too often - LEX DIRECTLY FROM VECTOR
    //    m_plainText.append(line);
    //    m_plainText += '\n';
    //  }
    //  m_lexer->lexInput(std::move(m_plainText), m_styleDb);
    //  m_needReLexing = false;
    //}

    //// Load parameters from codeview parent
    //m_characterWidthPixels = m_codeView.getCharacterWidthPixels();
    //m_characterHeightPixels = m_codeView.getCharacterHeightPixels();

    //CStopWatch m_tmr;
    //m_tmr.startTimer();

    //// Drop previous lines
    //m_physicalLines.clear();
    //m_numberOfEditorLines = 0;
    //m_maximumCharactersLine = 0;

    //std::function<std::vector<PhysicalLine>(const std::string&)> mapFn = [&, this](const std::string& line) {

    //  std::vector<PhysicalLine> phLineVec;
    //  std::string restOfLine;
    //  std::vector<EditorLine> edLines;
    //  edLines.reserve(10); // Should be enough for every splitting
    //  if (m_wrapWidthPixels != -1 && // Also check if the monospace'd width isn't exceeding the viewport
    //      line.size() * m_codeView.getCharacterWidthPixels() > m_wrapWidthPixels) {
    //    // We have a wrap and the line is too big - WRAP IT

    //    edLines.clear();
    //    restOfLine = line;

    //    // Calculate the allowed number of characters per editor line
    //    int maxChars = static_cast<int>(m_wrapWidthPixels / m_codeView.getCharacterWidthPixels());
    //    if (maxChars < 10)
    //      maxChars = 10; // Keep it to a minimum

    //                     // Start the wrap-splitting algorithm or resort to a brute-force character splitting one if
    //                     // no suitable spaces could be found to split the line
    //    while (restOfLine.size() > maxChars) {

    //      int bestSplittingPointFound = -1;
    //      for (int i = 0; i < restOfLine.size(); ++i) {
    //        if (i > maxChars)
    //          break; // We couldn't find a suitable space split point for restOfLine
    //        if (restOfLine[i] == ' ' && i != 0 /* Doesn't make sense to split at 0 pos */)
    //          bestSplittingPointFound = i;
    //      }

    //      if (bestSplittingPointFound != -1) { // We found a suitable space point to split this line
    //        edLines.push_back(restOfLine.substr(0, bestSplittingPointFound));
    //        restOfLine = restOfLine.substr(bestSplittingPointFound);
    //      }
    //      else {
    //        // No space found, brutally split characters (last resort)
    //        edLines.push_back(restOfLine.substr(0, maxChars));
    //        restOfLine = restOfLine.substr(maxChars);
    //      }
    //    }
    //    edLines.push_back(restOfLine); // Insert the last part and proceed

    //    // No need to do anything special for tabs - they're automatically converted into spaces

    //    std::vector<EditorLine> edVector;
    //    std::copy(edLines.begin(), edLines.end(), std::back_inserter(edVector));
    //    phLineVec.resize(1);
    //    phLineVec[0].m_editorLines = std::move(edVector);

    //  }
    //  else { // No wrap or the line fits perfectly within the wrap limits

    //    EditorLine el(line);
    //    phLineVec.emplace_back(std::move(el));
    //  }
    //  return phLineVec;
    //};


    //std::function<void(std::vector<PhysicalLine>&, const std::vector<PhysicalLine>&)> reduceFn =
    //  [&, this](std::vector<PhysicalLine>& accumulator, const std::vector<PhysicalLine>& pl) {

    //  accumulator.insert(accumulator.end(), pl.begin(), pl.end());

    //  m_numberOfEditorLines += static_cast<int>(pl[0].m_editorLines.size()); // Some more EditorLine
    //  std::for_each(pl[0].m_editorLines.begin(), pl[0].m_editorLines.end(), [this](const EditorLine& eline) {
    //    int lineLength = static_cast<int>(eline.m_characters.size());
    //    if (lineLength > m_maximumCharactersLine) // Check if this is the longest line found ever
    //      m_maximumCharactersLine = lineLength;
    //  });

    //};

    //m_physicalLines = std::move(blockingOrderedMapReduce<std::vector<PhysicalLine>>(m_plainTextLines, mapFn, reduceFn, 30U));    

    //m_tmr.stopTimer();
    //		double sec = m_tmr.getElapsedTimeInSeconds();
    //		double msec = sec * 1000;
    //    std::ofstream f("C:\\Users\\Alex\\Desktop\\Varco\\test.txt", std::ios_base::app | std::ios_base::out);
    //    f << "wrap calculations took " << msec << "\n";
    //    f.close();
    //    
    //		//cout << "Done in " << sec << " seconds / " << msec << " milliseconds";
  }

  void Document::paint() {
    if (!m_dirty)
      return;


    recalculateDocumentLines();

    //m_dirty = false; // It will be false at the end of this function, unless overridden

    //SkCanvas canvas(this->m_bitmap);
    //SkRect rect = getRect(absoluteRect); // Drawing is performed on the bitmap - absolute rect

    //CStopWatch m_tmr;
    //m_tmr.startTimer();

    ////////////////////////////////////////////////////////////////////////
    //// Draw the background of the document
    ////////////////////////////////////////////////////////////////////////
    //{
    //  SkPaint background;
    //  background.setColor(SkColorSetARGB(255, 39, 40, 34));
    //  canvas.drawRect(rect, background);
    //}    

    ////////////////////////////////////////////////////////////////////////
    //// Draw the entire document with a classic monokai style
    ////////////////////////////////////////////////////////////////////////
    //SkPaint& painter = m_codeView.m_fontPaint;
    //painter.setColor(SK_ColorWHITE);

    //auto setColor = [&painter](Style s) {
    //  switch (s) {
    //    case Comment: {
    //      painter.setColor(SkColorSetARGB(255, 117, 113, 94)); // Gray-ish 
    //    } break;
    //    case Keyword: {
    //      painter.setColor(SkColorSetARGB(255, 249, 38, 114)); // Pink-ish
    //    } break;
    //    case QuotedString: {
    //      painter.setColor(SkColorSetARGB(255, 230, 219, 88)); // Yellow-ish
    //    } break;
    //    case Identifier: {
    //      painter.setColor(SkColorSetARGB(255, 166, 226, 46)); // Green-ish
    //    } break;
    //    case KeywordInnerScope:
    //    case FunctionCall: {
    //      painter.setColor(SkColorSetARGB(255, 102, 217, 239)); // Light blue
    //    } break;
    //    case Literal: {
    //      painter.setColor(SkColorSetARGB(255, 174, 129, 255)); // Purple-ish
    //    } break;
    //    default: {
    //      painter.setColor(SK_ColorWHITE);
    //    } break;
    //  };
    //};

    //size_t documentRelativePos = 0;
    //size_t lineRelativePos = 0;

    //struct {
    //  float x;
    //  float y;
    //} startpoint = { 5, 20 }; // Start point where to start rendering

    //auto styleIt = m_styleDb.styleSegment.begin();
    //auto styleEnd = m_styleDb.styleSegment.end();
    //size_t nextDestination = -1;

    //auto calculateNextDestination = [&]() {
    //  // We can have 2 cases here:
    //  // 1) Our position hasn't still reached a style segment (apply regular style and continue)
    //  // 2) Our position is exactly on the start of a style segment (apply segment style and continue)
    //  // If there are no other segments, use a regular style and continue till the end of the lines

    //  if (styleIt == styleEnd) { // No other segments
    //    nextDestination = -1;
    //    setColor(Normal);
    //    return;
    //  }

    //  if (styleIt->start > documentRelativePos) { // Case 1
    //    setColor(Normal);
    //    nextDestination = styleIt->start;
    //  }
    //  else if (styleIt->start == documentRelativePos) { // Case 2
    //    setColor(styleIt->style);
    //    nextDestination = styleIt->start + styleIt->count;
    //    ++styleIt; // This makes sure our document relative position is never ahead of a style segment
    //  }
    //};

    //// First time we don't have a destination set, just find one (if there's any)
    //calculateNextDestination();

    //for (auto& pl : m_physicalLines) {

    //  size_t editorLineIndex = 0; // This helps tracking the last EditorLine of a PhysicalLine
    //  for (auto& el : pl.m_editorLines) {
    //    ++editorLineIndex;

    //    do {
    //      startpoint.x = 5.f + lineRelativePos * m_characterWidthPixels;

    //      // If we don't have a destination OR we can't reach it within our line, just draw the entire line and continue
    //      if (nextDestination == -1 ||
    //        nextDestination > documentRelativePos + (el.m_characters.size() - lineRelativePos)) {

    //        // Multiple lines will have to be rendered, just render this till the end and continue

    //        size_t charsRendered = 0;
    //        if (el.m_characters.size() > 0) { // Empty lines must be skipped
    //          std::string ts(el.m_characters.data() + lineRelativePos, static_cast<int>(el.m_characters.size() - lineRelativePos));
    //          canvas.drawText(ts.data(), ts.size(), startpoint.x, startpoint.y, painter);
    //          charsRendered = ts.size();
    //        }

    //        lineRelativePos = 0; // Next editor line will just start from the beginning
    //        documentRelativePos += charsRendered + /* Plus a newline if a physical line ended (NOT an EditorLine) */
    //          (editorLineIndex == pl.m_editorLines.size() ? 1 : 0);

    //        break; // Go and fetch a new line for the next cycle
    //      }
    //      else {

    //        // We can reach the goal within this line

    //        size_t charsRendered = 0;
    //        if (el.m_characters.size() > 0) { // Empty lines must be skipped
    //          std::string ts(el.m_characters.data() + lineRelativePos, static_cast<int>(nextDestination - documentRelativePos));
    //          canvas.drawText(ts.data(), ts.size(), startpoint.x, startpoint.y, painter);
    //          charsRendered = ts.size();
    //        }

    //        bool goFetchNewLine = false; // If this goal also exhausted the current editor line, go fetch
    //                                     // another one
    //        bool addNewLine = false; // If this was the last editor line, also add a newline because it
    //                                 // corresponds to a new physical line starting
    //        if (nextDestination - documentRelativePos + lineRelativePos == el.m_characters.size()) {
    //          goFetchNewLine = true;

    //          // Do not allow EditorLine to insert a '\n'. They're virtual lines
    //          if (editorLineIndex == pl.m_editorLines.size())
    //            addNewLine = true;

    //          lineRelativePos = 0; // Next editor line will just start from the beginning
    //        }
    //        else
    //          lineRelativePos += charsRendered;

    //        documentRelativePos += charsRendered + (addNewLine ? 1 : 0); // Just add a newline if we also reached this line's
    //                                                                     // end AND a physical line ended, not an EditorLine

    //        calculateNextDestination(); // Need a new goal

    //        if (goFetchNewLine)
    //          break; // Go fetch a new editor line (possibly on another physical line),
    //                 // we exhausted this editor line
    //      }

    //    } while (true);

    //    // Move the rendering cursor (carriage-return)
    //    startpoint.y = startpoint.y + m_characterHeightPixels;
    //  }
    //}

    //canvas.flush();

    //m_tmr.stopTimer();
    //double sec = m_tmr.getElapsedTimeInSeconds();
    //double msec = sec * 1000;
    //std::ofstream f("C:\\Users\\Alex\\Desktop\\Varco\\test.txt", std::ios_base::app | std::ios_base::out);
    //f << "redraw calculations took " << msec << "\n";
    //f.close();

  }

}