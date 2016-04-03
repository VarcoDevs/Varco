#include <WindowHandling/BaseOSWindow_Linux.hpp>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include <SkEvent.h>
#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>
#include <GrContext.h>
//#include <SkGr.h>

namespace varco {

  // Events we'll be listening for
  const long EVENT_MASK = StructureNotifyMask|ButtonPressMask|
                          ButtonReleaseMask|ExposureMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|LeaveWindowMask;

  BaseOSWindow::BaseOSWindow(int argc, char **argv)
    : Argc(argc), Argv(argv), Width(545), Height(355)
  {
    // Connect to the X server
    fDisplay = XOpenDisplay(nullptr);
    if (fDisplay == nullptr) {
      SkDebugf("Could not open a connection to an X Display");
      return;
    }
    
    GLint att[] = { // Create a window which supports GL
      GLX_RGBA,
      GLX_DEPTH_SIZE, 24,
      GLX_DOUBLEBUFFER,
      GLX_STENCIL_SIZE, 8,
      None
    };
    
    if (requestedMSAASampleCount > 0) {
      static const GLint kAttCount = SK_ARRAY_COUNT(att);
      GLint msaaAtt[kAttCount + 4];
      memcpy(msaaAtt, att, sizeof(att));
      SkASSERT(None == msaaAtt[kAttCount - 1]);
      msaaAtt[kAttCount - 1] = GLX_SAMPLE_BUFFERS_ARB;
      msaaAtt[kAttCount + 0] = 1;
      msaaAtt[kAttCount + 1] = GLX_SAMPLES_ARB;
      msaaAtt[kAttCount + 2] = requestedMSAASampleCount;
      msaaAtt[kAttCount + 3] = None;
      fVi = glXChooseVisual(fDisplay, DefaultScreen(fDisplay), msaaAtt);
      fMSAASampleCount = requestedMSAASampleCount;
    }
    if (fVi == nullptr) {
      fVi = glXChooseVisual(fDisplay, DefaultScreen(fDisplay), att);
      fMSAASampleCount = 0;
    }

    if (fVi == nullptr)
      throw std::runtime_error("Could not get a XVisualInfo structure");

    glXGetConfig(fDisplay, fVi, GLX_SAMPLES_ARB, &fAttachmentInfo.fSampleCount);
    glXGetConfig(fDisplay, fVi, GLX_STENCIL_SIZE, &fAttachmentInfo.fStencilBits);

    
    Colormap colorMap = XCreateColormap(fDisplay,
                                        RootWindow(fDisplay, fVi->screen),
                                        fVi->visual,
                                        AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = colorMap;
    swa.event_mask = EVENT_MASK;
    fWin = XCreateWindow(fDisplay,
                         RootWindow(fDisplay, fVi->screen),
                         0, 0, // x, y
                         Width, Height,
                         0, // border width
                         fVi->depth,
                         InputOutput,
                         fVi->visual,
                         CWEventMask | CWColormap,
                         &swa);

    XStoreName(fDisplay, fWin, "Varco");

    mapWindowAndWait();
    fGc = XCreateGC(fDisplay, fWin, 0, nullptr);

    // OpenGL initialization
    fGLContext = glXCreateContext(fDisplay, fVi, nullptr, GL_TRUE);
    if (fGLContext == nullptr)
      throw std::runtime_error("Could not create an OpenGL context");

    auto res = glXMakeCurrent(fDisplay, fWin, fGLContext);
    if (res == false)
      throw std::runtime_error("Could not attach OpenGL context");
    glViewport(0, 0,
                    SkScalarRoundToInt(Width),
                    SkScalarRoundToInt(Height));
    glClearColor(0, 0, 0, 255);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    fInterface = GrGLCreateNativeInterface();
    if (fInterface == nullptr)
      throw std::runtime_error("Could not create an OpenGL backend native interface");

    fContext = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)fInterface);
    if (fContext == nullptr)
      throw std::runtime_error("Could not create an OpenGL backend");

    fSurfaceProps = std::make_unique<const SkSurfaceProps>(SkSurfaceProps::kLegacyFontHost_InitType);
    fRenderTarget = setupRenderTarget(); // uses current width and height
    fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());
  }

  BaseOSWindow::~BaseOSWindow() {
    glXMakeCurrent(fDisplay, None, nullptr);
    glXDestroyContext(fDisplay, fGLContext);
  }

  GrRenderTarget* BaseOSWindow::setupRenderTarget() {
    GrBackendRenderTargetDesc desc;
    desc.fWidth = SkScalarRoundToInt(Width);
    desc.fHeight = SkScalarRoundToInt(Height);
    desc.fConfig = fContext->caps()->srgbSupport() &&
                   (Bitmap.info().profileType() == kSRGB_SkColorProfileType ||
                    Bitmap.info().colorType() == kRGBA_F16_SkColorType)
        ? kSkiaGamma8888_GrPixelConfig
        : kSkia8888_GrPixelConfig;
    desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
    desc.fSampleCnt = fAttachmentInfo.fSampleCount;
    desc.fStencilBits = fAttachmentInfo.fStencilBits;
    GrGLint buffer;
    GR_GL_GetIntegerv(fInterface, GR_GL_FRAMEBUFFER_BINDING, &buffer);
    desc.fRenderTargetHandle = buffer;
    return fContext->textureProvider()->wrapBackendRenderTarget(desc);
  }

  void BaseOSWindow::mapWindowAndWait() {
    SkASSERT(fDisplay);
    // Map the window on screen (i.e. make it visible)
    XMapWindow(fDisplay, fWin);

    // Ask to be notified of mapping accomplished
    long eventMask = StructureNotifyMask;
    XSelectInput(fDisplay, fWin, eventMask);

    // Wait for the window mapping confirmation
    XEvent evt;
    do {
      XNextEvent(fDisplay, &evt);
    } while(evt.type != MapNotify);
  }

  void BaseOSWindow::resize(int width, int height) {
    this->Width = width;
    this->Height = height;
    fRenderTarget = setupRenderTarget(); // render target has to be reset
    fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());
  }

  static Atom wm_delete_window_message;

  namespace {

    VirtualKeycode remapKeyToVarcoKey(unsigned int keysym) { // X server-specific keycodes remapping (from keysymdef.h)
      using enumType = std::underlying_type<VirtualKeycode>::type;

      if (keysym >= 'A' && keysym <= 'Z')
        return static_cast<VirtualKeycode>(static_cast<enumType>(VirtualKeycode::VK_A) + (keysym - 'A'));
      else if (keysym >= '0' && keysym <= '9')
        return static_cast<VirtualKeycode>(static_cast<enumType>(VirtualKeycode::VK_0) + (keysym - '0'));
      else
        return VirtualKeycode::VK_UNRECOGNIZED;
    }

  }

  void BaseOSWindow::invalidateWindow() { // Fire a redraw() event

    // X expose events are often fired at a blazing speed (for instance when resizing).
    // Since we can't keep up with all of them, this code prevents two events from being
    // fired in a < 100ms interval
//    auto now = std::chrono::system_clock::now();
//    auto elapsedInterval = now - lastExposeEventTime;
//    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedInterval).count();
    //if (elapsedMs < 100) {
    //  return;
    //}


    //printf("redrawing!! %ld\n", elapsedMs);

    //if (!fNeedDraw) {
    //        fNeedDraw = true;

    XEvent exppp;
    memset(&exppp, 0, sizeof(exppp));
    exppp.type = Expose;
    exppp.xexpose.display = fDisplay;
    XSendEvent(fDisplay, fWin, False, ExposureMask, &exppp);
    //  }
    //lastExposeEventTime = std::chrono::system_clock::now();
  }

  bool BaseOSWindow::wndProc(XEvent *evt) {

    switch (evt->type) {

      case Expose: {

        if (evt->xexpose.count == 0) { // Only handle the LAST expose redraw event
                                       // if there are multiple ones

          bool resizePending = false;
          while (XCheckTypedWindowEvent(fDisplay, fWin, ConfigureNotify, evt)) {
              this->Width = evt->xconfigure.width;
              this->Height = evt->xconfigure.height;
              resizePending = true;
          }
          if (resizePending)
            this->resize(this->Width, this->Height);

          //while (XCheckTypedWindowEvent(fDisplay, fWin, Expose, evt));

          if (!(Width > 0 && Height > 0))
            return 1; // Nonsense painting a 0 area
//          else {
//            // A resize might have occurred since last time
////            if (Width != Bitmap.width() || Height != Bitmap.height()) {
//////              Bitmap.allocPixels(SkImageInfo::Make(Width, Height,
//////                                                   kN32_SkColorType,
//////                                                   kPremul_SkAlphaType));
////                glViewport(0, 0,
////                                SkScalarRoundToInt(Width),
////                                SkScalarRoundToInt(Height));
////              fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget,
////                                                               fSurfaceProps.get()).release());
////            }
//          }

          // Call the OS-independent draw function
          auto surfacePtr = fSurface->getCanvas();
          this->draw(*surfacePtr);

          // Finally do the painting after the drawing is done
          this->paint();
        }

        return true;

      } break;

      case ConfigureNotify: {
        this->resize(evt->xconfigure.width, evt->xconfigure.height);

//          auto now = std::chrono::system_clock::now();
//              auto elapsedInterval = now - lastExposeEventTime;
//              auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedInterval).count();
//              if (elapsedMs < 500) {
//                return true;
//              }

//              invalidateWindow();
//          lastExposeEventTime = std::chrono::system_clock::now();

          //this->paint();

        return true;
      } break;

      case ClientMessage: {
        if ((Atom)evt->xclient.data.l[0] == wm_delete_window_message)
          return false;
      } break;

      case ButtonPress: { // Mouse input down

        switch (evt->xbutton.button) {

          case Button1: { // Left mouse
            auto x = evt->xbutton.x;
            auto y = evt->xbutton.y;
            this->onLeftMouseDown(x, y);
          } break;

          default:
            break;
        }
      } break;

      case ButtonRelease: { // Mouse input up

        switch (evt->xbutton.button) {

          case Button1: { // Left mouse
            auto x = evt->xbutton.x;
            auto y = evt->xbutton.y;
            this->onLeftMouseUp(x, y);
          } break;

          default:
            break;
        }
      } break;

      case KeyPress: {
        auto keysym = XkbKeycodeToKeysym(this->fDisplay, evt->xkey.keycode, 0,
                                         /*evt->xkey.state & ShiftMask ? 1 : 0*/ 1);
        this->onKeyDown(remapKeyToVarcoKey(keysym));
      } break;

      case LeaveNotify: {
        this->onMouseLeave();
      } break;

      case MotionNotify: {

        int x, y;

        while (XCheckTypedWindowEvent(fDisplay, fWin, MotionNotify, evt)) {
          x = evt->xmotion.x;
          y = evt->xmotion.y;
        }

        if (evt->xmotion.state & Button1Mask) { // Left mouse is down
          x = evt->xmotion.x;
          y = evt->xmotion.y;
          this->onLeftMouseMove(x, y);
        }
      } break;

      default: // fallthrough
        break;
      }
    return true; // Handled
  }

  int BaseOSWindow::show() {
    if (fDisplay == nullptr) {
      return 1; // Error
    }

    wm_delete_window_message = XInternAtom(fDisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(fDisplay, fWin, &wm_delete_window_message, 1);

    XSelectInput(fDisplay, fWin, EVENT_MASK);

    auto surfacePtr = fSurface->getCanvas();
    this->draw(*surfacePtr);

    // Finally do the painting after the drawing is done
    this->paint();

    while(1) {
      XEvent evt;
      XNextEvent(fDisplay, &evt);

      auto ret = wndProc(&evt);

      if(ret == false) // Exit requested
        break;
    }
  }

  void BaseOSWindow::startMouseCapture() { // Track the mouse to be notified when it leaves the client area
    // Do nothing, linux tracks mouse exit if LeaveWindowMask is used
  }

  void BaseOSWindow::redraw() {
    invalidateWindow();
  }

  void BaseOSWindow::paint() {


    fContext->flush();

    glXSwapBuffers(fDisplay, fWin);

    return;


    if (fDisplay == nullptr)
      return; // No X display
    //if (this->fGLContext) // With GL no need for XPutImage
    //  return;

    // Draw the bitmap to the screen.
    int width = Bitmap.width();
    int height = Bitmap.height();

    // Convert the bitmap to XImage
    XImage image;
    sk_bzero(&image, sizeof(image));

    int bitsPerPixel = Bitmap.bytesPerPixel() * 8;
    image.width = Bitmap.width();
    image.height = Bitmap.height();
    image.format = ZPixmap;
    image.data = (char*) Bitmap.getPixels();
    image.byte_order = LSBFirst;
    image.bitmap_unit = bitsPerPixel;
    image.bitmap_bit_order = LSBFirst;
    image.bitmap_pad = bitsPerPixel;
    image.depth = 24;
    image.bytes_per_line = Bitmap.rowBytes() - Bitmap.width() * 4;
    image.bits_per_pixel = bitsPerPixel;

    auto res = XInitImage(&image);
    if(res == false)
      throw std::runtime_error("Could not initialize X server image structures");

    XPutImage(fDisplay, fWin, fGc, &image,
              0, 0,     // src x,y
              0, 0,     // dst x,y
              width, height);
  }

} // namespace varco
