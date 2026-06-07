#include "CVEWindow.h"

CVEWindow::CVEWindow(const uint32_t inWidth, const uint32_t inHeight, const char* inTitle)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Window = glfwCreateWindow(inWidth, inHeight, inTitle, nullptr, nullptr);
}

CVEWindow::~CVEWindow()
{
}

bool CVEWindow::GetShouldClose() const
{
    return glfwWindowShouldClose(Window);
}

void CVEWindow::Terminate()
{
    glfwDestroyWindow(Window);

    glfwTerminate();
}

GLFWwindow* CVEWindow::GetGLFWWindow() const
{
    return Window;
}
