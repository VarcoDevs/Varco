#include <WindowHandling/BaseOSWindow_Linux.hpp>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include <SkEvent.h>
#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>
#include <GrContext.h>
//#include <SkGr.h>
#include <vector>
#include <string>
#include <cctype>

namespace varco {

  // Events we'll be listening for
  const long EVENT_MASK = StructureNotifyMask|ButtonPressMask|
                          ButtonReleaseMask|ExposureMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|LeaveWindowMask;

  static Bool WaitForNotify(Display *d, XEvent *e, char *arg) { // Callback for window ready
     return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
  }

  // A list of atoms - an atom is a short string with an associated number representing properties
  // or messages within the X11 system.
  enum {
      _X_ATOM_INDEX_UTF8_STRING,
      _X_ATOM_INDEX_WM_PROTOCOLS,
      _X_ATOM_INDEX_WM_DELETE_WINDOW,
      _X_ATOM_INDEX_WM_SAVE_YOURSELF,
      _X_ATOM_INDEX_WM_TAKE_FOCUS,
      _X_ATOM_INDEX_WM_CLIENT_MACHINE,
      _X_ATOM_INDEX_WM_CLIENT_LEADER,
      _X_ATOM_INDEX__MOTIF_WM_HINTS,
      _X_ATOM_INDEX__NET_WM_NAME,
      _X_ATOM_INDEX__NET_WM_STATE,
      _X_ATOM_INDEX__NET_WM_STATE_HIDDEN,
      _X_ATOM_INDEX__NET_WM_STATE_FULLSCREEN,
      _X_ATOM_INDEX__NET_WM_PID,
      _X_ATOM_INDEX__NET_WM_PING,
      _X_ATOM_INDEX__NET_WM_SYNC_REQUEST,
      _X_ATOM_INDEX__NET_WM_SYNC_REQUEST_COUNTER,
      _X_ATOM_INDEX_XdndAware,
      _X_ATOM_INDEX_XdndEnter,
      _X_ATOM_INDEX_XdndLeave,
      _X_ATOM_INDEX_XdndPosition,
      _X_ATOM_INDEX_XdndStatus,
      _X_ATOM_INDEX_XdndDrop,
      _X_ATOM_INDEX_XdndFinished,
      _X_ATOM_INDEX_XdndTypeList,
      _X_ATOM_INDEX_XdndSelection,
      _X_ATOM_INDEX_XdndActionCopy,
      _X_ATOM_INDEX_XdndActionPrivate,
      _X_ATOM_INDEX_MIME_TYPE_text__plain,
      _X_ATOM_INDEX_MIME_TYPE_text__unicode,
      _X_ATOM_INDEX_MIME_TYPE_text__x_moz_url,
      _X_ATOM_INDEX_MIME_TYPE_text__uri_list,
      _X_ATOM_INDEX_PRIMARY,
      _X_ATOM_INDEX_CLIPBOARD
  };
  static const char *const atom_names[] = {
      "UTF8_STRING",
      "WM_PROTOCOLS",
      "WM_DELETE_WINDOW",
      "WM_SAVE_YOURSELF",
      "WM_TAKE_FOCUS",
      "WM_CLIENT_MACHINE",
      "WM_CLIENT_LEADER",
      "_MOTIF_WM_HINTS",
      "_NET_WM_NAME",
      "_NET_WM_STATE",
      "_NET_WM_STATE_HIDDEN",
      "_NET_WM_STATE_FULLSCREEN",
      "_NET_WM_PID",
      "_NET_WM_PING",
      "_NET_WM_SYNC_REQUEST",
      "_NET_WM_SYNC_REQUEST_COUNTER",
      "XdndAware",
      "XdndEnter",
      "XdndLeave",
      "XdndPosition",
      "XdndStatus",
      "XdndDrop",
      "XdndFinished",
      "XdndTypeList",
      "XdndSelection",
      "XdndActionCopy",
      "XdndActionPrivate",
      "text/plain",
      "text/unicode",
      "text/x-moz-url",
      "text/uri-list",
      "PRIMARY",
      "CLIPBOARD"
  };
  #define X_ATOM(name) __lwi_atoms[_X_ATOM_INDEX_##name]
  #define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

  Atom __lwi_atoms[NELEMS(atom_names)];

  // X extensions for sync protocol
  static const int WF_NETWM_SYNC  = 0x20;

  enum {
      _X_EXTENSION_DBE,
      _X_EXTENSION_SHM,
      _X_EXTENSION_GLX,
      _X_EXTENSION_RENDER,
      _X_EXTENSION_VIDMODE,
      _X_EXTENSION_FIXES,
      _X_EXTENSION_SYNC,
  };

  #define X __lwi_context

  #define X_HAS_EXTENSION(n) (X.extensions & (1 << (n)))

  /* X extensions */
  #define X_HAS_DBE     X_HAS_EXTENSION(_X_EXTENSION_DBE)
  #define X_HAS_SHM     X_HAS_EXTENSION(_X_EXTENSION_SHM)
  #define X_HAS_GL      X_HAS_EXTENSION(_X_EXTENSION_GLX)
  #define X_HAS_RENDER  X_HAS_EXTENSION(_X_EXTENSION_RENDER)
  #define X_HAS_VIDMODE X_HAS_EXTENSION(_X_EXTENSION_VIDMODE)
  #define X_HAS_FIXES   X_HAS_EXTENSION(_X_EXTENSION_FIXES)
  #define X_HAS_SYNC    X_HAS_EXTENSION(_X_EXTENSION_SYNC)


  BaseOSWindow::BaseOSWindow(int argc, char **argv)
    : Argc(argc), Argv(argv), Width(545), Height(355)
  {

    auto threadStat = XInitThreads();
    if (!threadStat)
      SkDebugf("XInitThreads() returned 0 (failure- this program may fail)");

    // Connect to the X server
    fDisplay = XOpenDisplay(nullptr);
    if (fDisplay == nullptr) {
      SkDebugf("Could not open a connection to an X Display");
      return;
    }





    int a, b;
    auto ret = XSyncQueryExtension(fDisplay, &a, &b);
    ret = XSyncInitialize(fDisplay, &a, &b);

    // Load up the atom ids for the required ones
    for (unsigned i = 0; i < NELEMS(atom_names); i++) {
      __lwi_atoms[i] = XInternAtom(fDisplay, atom_names[i], False);
            //printf("%s is now [%d]\n", atom_names[i], (int)__lwi_atoms[i]);
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

    // Register for the WM_SYNC protocol and ping requests
    Atom wm_protocols[] = {
        X_ATOM(WM_DELETE_WINDOW),
        X_ATOM(WM_TAKE_FOCUS),
        X_ATOM(_NET_WM_PING),
        X_ATOM(_NET_WM_SYNC_REQUEST),
        X_ATOM(_NET_WM_SYNC_REQUEST_COUNTER)
    };
    ret = XSetWMProtocols(fDisplay, fWin, wm_protocols, NELEMS(wm_protocols));

    // WM protocol for killing hung process
    XSetWMProperties(fDisplay, fWin, NULL, NULL, NULL, 0, NULL, NULL, NULL);

    Atom *list;
    int len;
    ret = XGetWMProtocols(fDisplay, fWin, &list, &len);
    //for(int i=0; i < len; ++i)
    //  printf("%d\n", (int)list[i]);

    pid_t pid = getpid();
    ret = XChangeProperty (fDisplay, fWin, X_ATOM(_NET_WM_PID), XA_CARDINAL, 32,
                           PropModeReplace, (unsigned char *) &pid, 1);

    netwm_sync_value.hi = 0;
    netwm_sync_value.lo = 0;
    netwm_sync_counter = XSyncCreateCounter(fDisplay, netwm_sync_value);
    ret = XChangeProperty (fDisplay, fWin, X_ATOM(_NET_WM_SYNC_REQUEST_COUNTER), XA_CARDINAL, 32,
                           PropModeReplace, (unsigned char *) &netwm_sync_counter, 1);

    flags |= WF_NETWM_SYNC;


    XStoreName (fDisplay, fWin, "Varco");

    fGc = XCreateGC (fDisplay, fWin, 0, nullptr);


    // Signal XDND (Drag'n'Drop) support
    Atom XdndAware = X_ATOM(XdndAware);
    Atom version = 5;
    auto status = XChangeProperty (fDisplay, fWin, XdndAware, XA_ATOM, 32, PropModeReplace,
                                   (unsigned char*)&version, 1);

    if (status == BadAlloc || status == BadAtom || status == BadMatch || status == BadValue
        || status == BadWindow)
      throw std::runtime_error("Could not initialize XDND");

    // Signal the MIME types we support for XDND
    m_supportedXDNDMimes.push_back(X_ATOM(MIME_TYPE_text__plain));
    m_supportedXDNDMimes.push_back(X_ATOM(MIME_TYPE_text__uri_list));

    status = XChangeProperty(fDisplay, fWin, X_ATOM(XdndTypeList), XA_ATOM, 32, PropModeAppend,
                             (unsigned char *)m_supportedXDNDMimes.data(), m_supportedXDNDMimes.size());
    if (status == BadAlloc || status == BadAtom || status == BadMatch || status == BadValue
        || status == BadWindow)
      throw std::runtime_error("Could not initialize XDND");

    // OpenGL initialization
    fGLContext = glXCreateContext(fDisplay, fVi, nullptr, GL_TRUE);
    fSharedGLContext = glXCreateContext(fDisplay, fVi, fGLContext, GL_TRUE);
    if (fGLContext == nullptr || fSharedGLContext == nullptr)
      throw std::runtime_error("Could not create an OpenGL context");

    auto res = glXMakeCurrent(fDisplay, fWin, fGLContext);
    if (res == false)
      throw std::runtime_error("Could not attach OpenGL context");

    //setVsync(false);

    glViewport(0, 0,
                    SkScalarRoundToInt(Width),
                    SkScalarRoundToInt(Height));
    glClearColor(39, 40, 34, 255);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    fSurfaceProps = std::make_unique<const SkSurfaceProps>(SkSurfaceProps::kLegacyFontHost_InitType);
    //fRenderTarget = setupRenderTarget(); // uses current width and height
    //fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());

    XMapWindow(fDisplay, fWin); // Map and wait for the window to be ready
    XEvent event;
    XIfEvent(fDisplay, &event, WaitForNotify, (char*)fWin);

//    XSynchronize(fDisplay, True); // for debugging purposes, report all errors immediately
//    Atom *atomList;
//    int atomLen;
//    auto status = XGetWMProtocols(fDisplay, fWin, &atomList, &atomLen);
//    if(status == 0)
//      printf("no");

//    auto Atom_wmProtocols = XInternAtom(fDisplay, "WM_PROTOCOLS", True);
//    if (Atom_wmProtocols == None)
//      printf("no");

//    auto Atom_syncRequest = XInternAtom(fDisplay, "_NET_WM_SYNC_REQUEST", True);
//    if (Atom_syncRequest == None)
//      printf("no");

//    auto Atom_syncRequestCounter = XInternAtom(fDisplay, "_NET_WM_SYNC_REQUEST_COUNTER", True);
//    if (Atom_syncRequestCounter == None)
//      printf("no");


//    atoms.push_back(Atom_wmProtocols);
//    atoms.push_back(Atom_syncRequest);
//    atoms.push_back(Atom_syncRequestCounter);

//    status = XSetWMProtocols(fDisplay, fWin, atoms.data(), atoms.size());
//    if(status == 0)
//      printf("no");

//    size_t length = atoms->length;
//    jint* atomsBegin = elements(atoms);
//    jint* atomsEnd   = atomsBegin + length;


//    std::vector<XLibAtom> atomVector(atomsBegin, atomsEnd);
//    XLibAtom* atomsArray = &(atomVector.front());

//    XSetWMProtocols(dpy, xid, atomsArray, length);

  }

  BaseOSWindow::~BaseOSWindow() {
    XDestroyWindow(fDisplay, fWin);
    glXMakeCurrent(fDisplay, None, nullptr);
    glXDestroyContext(fDisplay, fSharedGLContext);
    glXDestroyContext(fDisplay, fGLContext);
    XCloseDisplay(fDisplay);
  }

  GrRenderTarget* BaseOSWindow::setupRenderTarget(int width, int heigth) {
    GrBackendRenderTargetDesc desc;
    desc.fWidth = SkScalarRoundToInt(width);
    desc.fHeight = SkScalarRoundToInt(heigth);
    desc.fConfig = fContext->caps()->srgbSupport() && Bitmap.info().colorType() == kRGBA_F16_SkColorType
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

  void BaseOSWindow::resize(int width, int height) {    
    this->Width = width;
    this->Height = height;    
  }




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

  void BaseOSWindow::invalidateWindow(bool flush) { // Fire a redraw() event

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

    XEvent evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = Expose;
    evt.xexpose.window = fWin;
    XSendEvent(fDisplay, fWin, False, ExposureMask, &evt);
    if (flush)
      XFlush(fDisplay);
    //  }
    //lastExposeEventTime = std::chrono::system_clock::now();
  }

  // Some helper code to load the correct version of glXSwapInterval
  #define GLX_GET_PROC_ADDR(name) glXGetProcAddress(reinterpret_cast<const GLubyte*>((name)))
  #define EXT_WRANGLE(name, type, ...) \
      if (GLX_GET_PROC_ADDR(#name)) { \
          static type k##name; \
          if (!k##name) { \
              k##name = (type) GLX_GET_PROC_ADDR(#name); \
          } \
          k##name(__VA_ARGS__); \
          /*SkDebugf("using %s\n", #name);*/ \
          return; \
      }

  static void glXSwapInterval(Display* dsp, GLXDrawable drawable, int interval) {
      EXT_WRANGLE(glXSwapIntervalEXT, PFNGLXSWAPINTERVALEXTPROC, dsp, drawable, interval);
      EXT_WRANGLE(glXSwapIntervalMESA, PFNGLXSWAPINTERVALMESAPROC, interval);
      EXT_WRANGLE(glXSwapIntervalSGI, PFNGLXSWAPINTERVALSGIPROC, interval);
  }

  void BaseOSWindow::setVsync(bool vsync) {
    if (fDisplay && fGLContext && fWin) {
      int swapInterval = vsync ? 1 : 0;
      glXSwapInterval(glXGetCurrentDisplay(),glXGetCurrentReadDrawable(), swapInterval);
    }
  }

  void BaseOSWindow::renderThreadFn() {

    {
      std::unique_lock<std::mutex> lk(renderMutex);

      auto res = glXMakeCurrent(fDisplay, fWin, fSharedGLContext);
      if (res == false)
        throw std::runtime_error("Could not attach OpenGL context");

      fInterface = GrGLCreateNativeInterface();
      if (fInterface == nullptr)
        throw std::runtime_error("Could not create an OpenGL backend native interface");

      fContext = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)fInterface);
      if (fContext == nullptr)
        throw std::runtime_error("Could not create an OpenGL backend");

      fRenderTarget = setupRenderTarget(Width, Height); // render target has to be reset
      fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());

      //setVsync(false);
    }

    int threadWidth = Width, threadHeight = Height;

    while(true) {

      std::unique_lock<std::mutex> lk(renderMutex);
      while (!redrawNeeded) {
        renderCV.wait(lk);
        if (!redrawNeeded) {} // Spurious wakeup
      }

      if (!(Width > 0 && Height > 0))
        continue; // Nonsense painting a 0 area

      if(stopRendering == true) {
        fContext->releaseResourcesAndAbandonContext();
        return;
      }

      bool resizing = false;
      if (Width != threadWidth || Height != threadHeight) {
        // Area has been changed, we're resizing
        resizing = true;
      }

      auto res = glXMakeCurrent(fDisplay, fWin, fSharedGLContext);
      if (res == false)
        throw std::runtime_error("Could not attach OpenGL context");

      if (Width != threadWidth || Height != threadHeight) {
        fRenderTarget = setupRenderTarget(Width, Height); // render target has to be reset
        fSurface.reset(SkSurface::MakeRenderTargetDirect(fRenderTarget, fSurfaceProps.get()).release());
        threadWidth = Width;
        threadHeight = Height;
      }

      // Call the OS-independent draw function
      auto surfacePtr = fSurface->getCanvas();
      this->draw(*surfacePtr);

      if(stopRendering == true) {
        fContext->releaseResourcesAndAbandonContext();
        return;
      }

      //if (resizing) {
     //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
     // } else {


      fContext->flush();
      glXSwapBuffers(fDisplay, fWin);      

      if (Width == threadWidth && Height == threadHeight) {// Check for size to be updated
        redrawNeeded = false;

        // set netwm sync counter to signal that we're done redrawing (WM/DM is waiting)
        if (flags & WF_NETWM_SYNC) {
            XSyncSetCounter(fDisplay, netwm_sync_counter, netwm_sync_value);
            flags &= ~WF_NETWM_SYNC;
            //printf("sync done\n");
        }
      }

      //std::this_thread::sleep_for(std::chrono::milliseconds(5));

      //}
    }

  }

  bool BaseOSWindow::wndProc(XEvent *evt) {

    switch (evt->type) {

      case Expose: { // Paint event

        if (evt->xexpose.count == 0) { // Only handle the LAST expose redraw event
                                       // if there are multiple ones

            while (XCheckTypedWindowEvent(fDisplay, fWin, Expose, evt));

            std::unique_lock<std::mutex> lk(renderMutex);
            redrawNeeded = true;
            renderCV.notify_one();
        }        

        return true;

      } break;

      case ConfigureNotify: {

        //if (evt->xconfigure.window != fWin)
        //  break;



//        while (XCheckTypedWindowEvent(fDisplay, evt->xany.window, ConfigureNotify, evt)) {
//          this->resize(evt->xconfigure.width, evt->xconfigure.height);
//        }

        this->resize(evt->xconfigure.width, evt->xconfigure.height);
        redrawNeeded = true;
        renderCV.notify_one();

      } break;

      case ClientMessage: {

        //printf("Received %d\n", (int)evt->xclient.data.l[0]);

        // Synchronization protocol support
        if ((Atom)evt->xclient.data.l[0] == X_ATOM(_NET_WM_PING)) {
            evt->xclient.window = fWin;
            //xe->window = w->wid;
            XSendEvent(fDisplay, DefaultRootWindow(fDisplay), False,
                       SubstructureNotifyMask | SubstructureRedirectMask, (XEvent *) evt);
            //printf("ping!\n");
          } else if ((Atom)evt->xclient.data.l[0] == X_ATOM(_NET_WM_SYNC_REQUEST)) {
//           if (flags & WF_NETWM_SYNC)
//               XSyncSetCounter(fDisplay, netwm_sync_counter, netwm_sync_value);
            flags |= WF_NETWM_SYNC;
            netwm_sync_value.hi = evt->xclient.data.l[3];
            netwm_sync_value.lo = evt->xclient.data.l[2];
            //printf("sync request\n");
          }

          if ((Atom)evt->xclient.data.l[0] == X_ATOM(WM_DELETE_WINDOW)) // wm_delete_window_message
            return false;

          // Drag and drop support
          if (evt->xclient.message_type == X_ATOM(XdndEnter)) {
            // Drag and drop was initiated, keep track of drop position
            m_XDNDInProgress = true;
          } else if (m_XDNDInProgress && evt->xclient.message_type == X_ATOM(XdndDrop)) {
            // Drop event
            // xdnd_source_window = e.xclient.data.l[0];
            Window ownerOfSelection = XGetSelectionOwner(this->fDisplay, X_ATOM(XdndSelection));

            // Use primary buffer to store the selection
            XConvertSelection (this->fDisplay, X_ATOM(XdndSelection), m_supportedXDNDMimes[0],
                               X_ATOM(XdndSelection), this->fWin, CurrentTime);

            // Send "Finished"
            XEvent xevent;
            memset (&xevent, 0, sizeof(xevent));
            xevent.xany.type = ClientMessage;
            xevent.xany.display = this->fDisplay;
            xevent.xclient.window = ownerOfSelection;
            xevent.xclient.message_type = X_ATOM(XdndFinished);
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = this->fWin;
            XSendEvent (this->fDisplay, ownerOfSelection, 0, 0, &xevent);

            XFlush (this->fDisplay);

          } else if (m_XDNDInProgress && evt->xclient.message_type == X_ATOM(XdndPosition)) {
            // Change of position while drag'n'dropping
            m_XDNDPos.set(evt->xclient.data.l[2]  >> 16, evt->xclient.data.l[2] & 0xffff);

            // Reply to source with XDND ready
            XEvent xevent;
            memset(&xevent, 0, sizeof(xevent));
            xevent.xany.type = ClientMessage;
            xevent.xany.display = this->fDisplay;
            xevent.xclient.window = evt->xclient.data.l[0]; // source
            xevent.xclient.message_type = X_ATOM(XdndStatus);
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = this->fWin;
            xevent.xclient.data.l[1] = 1; // Yes, we'll handle it
            xevent.xclient.data.l[2] = 0; // rectangle
            xevent.xclient.data.l[3] = 0;
            xevent.xclient.data.l[4] = X_ATOM(XdndActionPrivate);

            XSendEvent (this->fDisplay, evt->xclient.data.l[0] /* to source */, False, NoEventMask, &xevent);
            XFlush (this->fDisplay);

          } else if(m_XDNDInProgress && evt->xclient.message_type == X_ATOM(XdndLeave)) {
            // Drag and drop cancelled
            m_XDNDInProgress = false;
          }

      } break;

      // Selections in X are transfers between a source and target window. We need those to handle events
      // like XDND (drag'n'drop) or clipboard operations
      case SelectionNotify: {

          Window ownerOfSelection = XGetSelectionOwner(this->fDisplay, X_ATOM(XdndSelection));

          if (evt->xselection.property == None)
            return false; // Could not get filenames

          // Get the list of filenames dropped
          std::vector<std::string> filenames;
          int             offset = 0;
          unsigned long   bytesRemaining;
          unsigned long   numItems = 0;
          unsigned char*  s = nullptr;
          Atom            actualType;
          int             actualFormat;
          do {
            if (XGetWindowProperty(this->fDisplay, evt->xany.window, X_ATOM(XdndSelection),
                                   offset / sizeof(unsigned char *), 1024, False, AnyPropertyType,
                                   &actualType, &actualFormat, &numItems, &bytesRemaining, &s) != Success)
              return false; // Failure

            filenames.push_back(std::string((char*)s));
            // Strip URI identifiers and end whitespaces
            {
              std::string& str = filenames.back();
              int i = str.size() - 1;
              for(; i >= 0; --i) {
                  if (std::isspace(str[i]) == 0)
                    break;
              }
              if (str.size() != i)
                str = str.substr(0, i + 1);
              if(str.find("file://") == 0)
                str = str.substr(sizeof("file://") - 1);
            }

            offset += numItems;
          } while (bytesRemaining > 0);

          this->onFileDrop(m_XDNDPos.x(), m_XDNDPos.y(), filenames);

      } break;

      case ButtonPress: { // Mouse input down

        switch (evt->xbutton.button) {

          case Button1: { // Left mouse
            auto x = evt->xbutton.x;
            auto y = evt->xbutton.y;
            this->onLeftMouseDown(x, y);
          } break;

          case Button3: { // Right mouse
          } break;

          case Button4: { // Mouse wheel up
            auto x = evt->xbutton.x;
            auto y = evt->xbutton.y;
            this->onMouseWheel(x, y, -1);
          } break;
          case Button5: { // Mouse wheel down
            auto x = evt->xbutton.x;
            auto y = evt->xbutton.y;
            this->onMouseWheel(x, y, 1);
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

        while (XCheckTypedWindowEvent(fDisplay, fWin, MotionNotify, evt)) {}

        x = evt->xmotion.x;
        y = evt->xmotion.y;

        if (evt->xmotion.state & Button1Mask) // Left mouse is down
          this->onLeftMouseMove(x, y);

        this->onMouseMove(x, y);
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

    XSelectInput(fDisplay, fWin, EVENT_MASK);

    std::function<void(void)> renderProc = std::bind(&BaseOSWindow::renderThreadFn, this);
    renderThread = std::thread(renderProc);

    invalidateWindow(false); // Fire a redraw event and keep it in the queue

    //bool exitRequested = false;
    while(true) {

      XEvent evt;
      //while(XPending(fDisplay)) // Do not use - skips events
      XNextEvent(fDisplay, &evt);

      bool continueLoop = true;
      //if (evt.xany.window == fWin) {

        continueLoop = wndProc(&evt);

        if(continueLoop == false) { // Exit requested
          stopRendering = true;
        }
      //}

      if (stopRendering)
        break;
    }

    renderThread.join();

    return 0;
  }

  void BaseOSWindow::startMouseCapture() { // Track the mouse to be notified when it leaves the client area
    // Do nothing, linux tracks mouse exit if LeaveWindowMask is used
  }

  void BaseOSWindow::stopMouseCapture() {
    // Ditto
  }

  void BaseOSWindow::repaint() {
    invalidateWindow();
  }

  void BaseOSWindow::paint() {


  //  fContext->flush();

  //  glXSwapBuffers(fDisplay, fWin);

    //return;


//    if (fDisplay == nullptr)
//      return; // No X display
//    //if (this->fGLContext) // With GL no need for XPutImage
//    //  return;

//    // Draw the bitmap to the screen.
//    int width = Bitmap.width();
//    int height = Bitmap.height();

//    // Convert the bitmap to XImage
//    XImage image;
//    sk_bzero(&image, sizeof(image));

//    int bitsPerPixel = Bitmap.bytesPerPixel() * 8;
//    image.width = Bitmap.width();
//    image.height = Bitmap.height();
//    image.format = ZPixmap;
//    image.data = (char*) Bitmap.getPixels();
//    image.byte_order = LSBFirst;
//    image.bitmap_unit = bitsPerPixel;
//    image.bitmap_bit_order = LSBFirst;
//    image.bitmap_pad = bitsPerPixel;
//    image.depth = 24;
//    image.bytes_per_line = Bitmap.rowBytes() - Bitmap.width() * 4;
//    image.bits_per_pixel = bitsPerPixel;

//    auto res = XInitImage(&image);
//    if(res == false)
//      throw std::runtime_error("Could not initialize X server image structures");

//    XPutImage(fDisplay, fWin, fGc, &image,
//              0, 0,     // src x,y
//              0, 0,     // dst x,y
//              width, height);
  }

} // namespace varco
