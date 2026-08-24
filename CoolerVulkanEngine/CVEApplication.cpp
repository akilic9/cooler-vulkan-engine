#include "CVEApplication.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "CVETypes.h"

void CVEApplication::Run()
{
    while (!Window.GetShouldClose())
    {
        glfwPollEvents();
        if (VkCommandBuffer commandBuffer = Renderer.BeginDraw())
        {
            Update();
            Renderer.BeginRecordCommandBuffer();
            
            RenderSystem.Render(commandBuffer);            
            
            Renderer.EndDraw();
        }
    }
    vkDeviceWaitIdle(Device.GetLogicalDevice());
}

void CVEApplication::Update()
{    
    CVEUniformBufferObject ubo{};
    ubo.View = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Projection = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(SwapChain.GetExtent().width) / static_cast<float>(SwapChain.GetExtent().height),
                                0.1f,
                                10.0f);
    
    ubo.Projection[1][1] *= -1; // Flip Y to compensate glm coord system
    
    RenderSystem.Update(Renderer.GetCurrentFrameIndex(), ubo);
}