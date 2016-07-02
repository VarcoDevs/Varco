#ifndef VARCO_VKEYCODES_HPP
#define VARCO_VKEYCODES_HPP

namespace varco {

  // This file defines a cross platform series of enums for keyboard inputs.
  // If a key on Windows maps to the Ctrl key and has value VK_CONTROL (0x11),
  // on Linux it will be XK_Control_R. Therefore both must map to the CTRL value here.
  enum class VirtualKeycode {
    VK_A, VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N, 
    VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,
    VK_0, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
    VK_CTRL,
    VK_UNRECOGNIZED
  };

}

#endif // VARCO_VKEYCODES_HPP
