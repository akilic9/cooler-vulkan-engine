#include "CVEApplication.h"

void CVEApplication::Run()
{
    Update();
    TerminateWindow();
}

void CVEApplication::Update()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
        Renderer.Draw();
    }
    Device.GetLogicalDevice().waitIdle();
}

void CVEApplication::TerminateWindow()
{
    SwapChain.CleanUp();
    Window.Terminate();
}