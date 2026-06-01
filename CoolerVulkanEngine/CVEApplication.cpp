#include "CVEApplication.h"

#include "CVEVulkanInstance.h"

void CVEApplication::Run()
{
    InitApp();
    Update();
}

void CVEApplication::InitApp()
{
    InitWindow();
    CVEVulkanInstance::CreateVulkanInstance(VulkanInstance, DebugMessenger);
}

void CVEApplication::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Window = glfwCreateWindow(CVEDefaultWindowParams::DEFAULT_WINDOW_WIDTH, CVEDefaultWindowParams::DEFAULT_WINDOW_HEIGHT, CVEDefaultWindowParams::DEFAULT_WINDOW_TITLE, nullptr, nullptr);
}

void CVEApplication::Update()
{
    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
    }
}

void CVEApplication::TerminateWindow()
{
    glfwDestroyWindow(Window);

    glfwTerminate();
}