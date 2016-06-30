#include <Control/DocumentManager.hpp>
#include <Utils/Utils.hpp>
#include <config.hpp>

namespace varco {
  DocumentManager::DocumentManager(CodeView& codeEditCtrl, TabBar& tabCtrl) :
    m_codeEditCtrl(codeEditCtrl),
    m_tabCtrl(tabCtrl)
  {
    // DEBUG - Add some debug tabs
    int id = m_tabCtrl.addNewTab("BasicBlock.cpp");
    auto it = m_tabDocumentMap.emplace(id, std::make_unique<Document>(m_codeEditCtrl));
    Document *document = it.first->second.get();
    document->loadFromFile(TestData::SimpleFile);
    document->applySyntaxHighlight(CPP);
    m_codeEditCtrl.loadDocument(*document);

    // Register callback for tab selection change - do this AFTER any debug tab addition
    tabCtrl.signalDocumentChange = [this](int id) {
      this->changeSelectedDocument(id);
      return true;
    };
  }

  namespace {
    std::string stripFileName(const std::string& filePath) {
      for (int i = filePath.size() - 1; i >= 0; --i) {
        if (filePath[i] == '\\' || filePath[i] == '/')
          return filePath.substr(i + 1);
      }
      return filePath;
    }

    bool extensionEndsIn(const std::string& fileName, const std::string& ext) {
      if (fileName.empty() || fileName.find('.') == std::string::npos || fileName.size() < ext.size() + 1)
        return false;
      return fileName.compare(fileName.size() - ext.size() - 1, ext.size() + 1, "." + ext) == 0;
    }
  }

  void DocumentManager::addNewFileDocument(std::string filePath) {
    auto fileName = stripFileName(filePath);
    int id = m_tabCtrl.addNewTab(fileName);

    auto it = m_tabDocumentMap.emplace(id, std::make_unique<Document>(m_codeEditCtrl));
    Document *document = it.first->second.get();
    document->loadFromFile(filePath);

    if (extensionEndsIn(fileName, "cpp"))
      document->applySyntaxHighlight(CPP);
    m_codeEditCtrl.loadDocument(*document);
  }

  void DocumentManager::changeSelectedDocument(int id) {
    // Save current vertical scrollbar position
    m_tabDocumentVScrollPos[m_tabCtrl.tabs[m_tabCtrl.selectedTabIndex].uniqueId] =
        m_codeEditCtrl.getVScrollbarValue();

    // Restore (if any) vertical scrollbar position
    auto it = m_tabDocumentVScrollPos.find(id);
    SkScalar vScrollbarPos = 0;
    if (it != m_tabDocumentVScrollPos.end())
      vScrollbarPos = it->second;
    // And load the document
    m_codeEditCtrl.loadDocument(*m_tabDocumentMap[id], vScrollbarPos); // The document MUST be present
  }
}
