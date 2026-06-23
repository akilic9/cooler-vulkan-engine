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
    Device.CleanUp();
}

void CVEApplication::TerminateWindow()
{
    Window.Terminate();
}