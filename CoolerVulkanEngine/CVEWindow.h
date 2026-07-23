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
    GLFWwindow* GetGLFWWindow() const;
    bool GetWasResized() const;
    void ResetResizeFlag();
    
    std::array<int, 2> GetWindowExtent() const;
    
private:
    static void FrameBufferResizeCallback(GLFWwindow* window, int width, int height);
    
    GLFWwindow* Window = nullptr;
    bool bFrameBufferResized = false;
};
