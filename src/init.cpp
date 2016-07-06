#include <WindowHandling/MainWindow.hpp>
#include <GLFW/glfw3.h>
#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>
#include <GrContext.h>
#include <SkSurface.h>
#include <memory>
#include <cassert>

GrContext* sContext = nullptr;
sk_sp<SkSurface> sSurface = nullptr;
std::unique_ptr<varco::MainWindow> sMainWindow;

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}


static void init_skia(int w, int h) {
    sContext = GrContext::Create(kOpenGL_GrBackend, 0);

    GrBackendRenderTargetDesc desc;
    desc.fWidth = w;
    desc.fHeight = h;
    desc.fConfig = kSkia8888_GrPixelConfig;
    desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
    desc.fSampleCnt = 1;
    desc.fStencilBits = 0;
    desc.fRenderTargetHandle = 0;  // assume default framebuffer

    sSurface = SkSurface::MakeFromBackendRenderTarget(sContext, desc, NULL);
}

static void resize_skia(int w, int h) {
    assert(sContext != nullptr);

    GrBackendRenderTargetDesc desc;
    desc.fWidth = w;
    desc.fHeight = h;
    desc.fConfig = kSkia8888_GrPixelConfig;
    desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
    desc.fSampleCnt = 1;
    desc.fStencilBits = 0;
    desc.fRenderTargetHandle = 0;  // assume default framebuffer

    sSurface = SkSurface::MakeFromBackendRenderTarget(sContext, desc, NULL);
}

static void cleanup_skia() {
    sSurface.reset();
    delete sContext;
}

// <glfw callbacks>

void window_size_callback(GLFWwindow* window, int width, int height) {
    resize_skia(width, height);
    sMainWindow->resize(width, height);
}

void window_mouse_button_callback (GLFWwindow* window, int button, int action, int mods) {

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        double xpos = 0, ypos = 0;
        glfwGetCursorPos(window, &xpos, &ypos);
        sMainWindow->onLeftMouseDown(xpos, ypos);

    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {

        double xpos = 0, ypos = 0;
        glfwGetCursorPos(window, &xpos, &ypos);
        sMainWindow->onLeftMouseUp(xpos, ypos);

    }
}

void window_cursor_pos_callback (GLFWwindow* window, double xpos, double ypos) {
      sMainWindow->onMouseMove(xpos, ypos);
}

// </glfw callbacks>

// Start window dimensions
const int kWidth = 960;
const int kHeight = 640;

int main(int argc, char **argv)
{
  using namespace varco;

  GLFWwindow* window;

  /* Initialize the library */
  if (!glfwInit())
      return EXIT_FAILURE;

  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(kWidth, kHeight, "Varco", NULL, NULL);
  if (!window) {
      glfwTerminate();
      return EXIT_FAILURE;
  }

  // glfw callbacks
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetMouseButtonCallback(window, window_mouse_button_callback);
  glfwSetCursorPosCallback(window, window_cursor_pos_callback);

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  // Set up Gr for OpenGL rendering
  init_skia(kWidth, kHeight);

  // Create an instance of MainWindow
  sMainWindow = std::make_unique<MainWindow>(argc, argv, kWidth, kHeight);

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {

      // Draw to the surface via its SkCanvas.
      SkCanvas* canvas = sSurface->getCanvas();   // We don't manage this pointer's lifetime.

      /* Render here */
      sMainWindow->draw(*canvas);
      canvas->flush();

      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      /* Poll for and process events */
      glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
