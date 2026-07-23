#include "CVEWindow.h"

CVEWindow::CVEWindow(const uint32_t inWidth, const uint32_t inHeight, const char* inTitle)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    Window = glfwCreateWindow(inWidth, inHeight, inTitle, nullptr, nullptr);
    glfwSetWindowUserPointer(Window, this);
    glfwSetFramebufferSizeCallback(Window, FrameBufferResizeCallback);
}

CVEWindow::~CVEWindow()
{
    glfwDestroyWindow(Window);

    glfwTerminate();
}

bool CVEWindow::GetShouldClose() const
{
    return glfwWindowShouldClose(Window);
}

GLFWwindow* CVEWindow::GetGLFWWindow() const
{
    return Window;
}

bool CVEWindow::GetWasResized() const
{
    return bFrameBufferResized;
}

void CVEWindow::ResetResizeFlag()
{
    bFrameBufferResized = false;
}

std::array<int, 2> CVEWindow::GetWindowExtent() const
{
    int width, height;
    glfwGetFramebufferSize(Window, &width, &height);
    return { width, height };
}

void CVEWindow::FrameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
    CVEWindow* thisWindow = reinterpret_cast<CVEWindow*>(glfwGetWindowUserPointer(window));
    thisWindow->bFrameBufferResized = true;
}
