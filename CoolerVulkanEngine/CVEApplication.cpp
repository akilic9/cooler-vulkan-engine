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
    }
}

void CVEApplication::TerminateWindow()
{
    Window.Terminate();
}