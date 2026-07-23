#include "CVEApplication.h"

void CVEApplication::Run()
{
    Update();
}

void CVEApplication::Update()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
        Renderer.Draw();
    }
    vkDeviceWaitIdle(Device.GetLogicalDevice());
}