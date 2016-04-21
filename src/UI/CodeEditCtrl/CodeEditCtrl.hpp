#ifndef VARCO_CODEEDITCTRL_HPP
#define VARCO_CODEEDITCTRL_HPP

#include <UI/UICtrlBase.hpp>

namespace varco {

  class CodeEditCtrl : public UICtrlBase { // The main code edit control
  public:
    CodeEditCtrl(MainWindow& parentWindow);

    void paint() override;
  };

}

#endif // VARCO_CODEEDITCTRL_HPP
