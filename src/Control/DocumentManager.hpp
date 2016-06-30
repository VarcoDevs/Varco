#ifndef VARCO_DOCUMENTMANAGER_HPP
#define VARCO_DOCUMENTMANAGER_HPP

#include <Document/Document.hpp>
#include <UI/CodeView/CodeView.hpp>
#include <UI/TabBar/TabBar.hpp>
#include <memory>
#include <map>

namespace varco {

  class DocumentManager {
  public:
    DocumentManager(CodeView& codeEditCtrl, TabBar& tabCtrl);

    void addNewFileDocument(std::string filePath);
    void changeSelectedDocument(int id /* Document id, also tab id in m_tabDocumentMap */);

  private:
    CodeView& m_codeEditCtrl;
    TabBar& m_tabCtrl;
    // A map that stores the association between a tab and a document
    std::map<int, std::unique_ptr<Document>> m_tabDocumentMap;
  };

}

#endif // VARCO_DOCUMENTMANAGER_HPP
