#pragma once
#include <array>
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
    
    std::array<int, 2> GetWindowExtent() const;
    
private:
    GLFWwindow* Window = nullptr;
};
