#pragma once
#include <glfw3.h>

class CVEWindow
{
public:
    CVEWindow(const uint32_t inWidth, const uint32_t inHeight, const char* inTitle);
    ~CVEWindow();
    
    CVEWindow(const CVEWindow&) = delete;
    CVEWindow &operator=(const CVEWindow&) = delete;
    
    bool GetShouldClose() const;
    void Terminate();
    GLFWwindow* GetGLFWWindow() const;
    
private:
    GLFWwindow* Window = nullptr;
};
