//> includes
#include "vk_engine.h"
#include <vk_images.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <VkBootstrap.h>
#include <vk_initializers.h>

#include <chrono>
#include <thread>

VulkanEngine* pLoadedEngine = nullptr;

const VulkanEngine& VulkanEngine::Get() { return *pLoadedEngine; }

void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(pLoadedEngine == nullptr);
    pLoadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    pWindow = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        vkWindowExtent.width,
        vkWindowExtent.height,
        window_flags);

    // Initialize Vulkan
    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();

    // everything went fine
    isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (isInitialized) {
        // Wait for GPU to finish tasks
        vkDeviceWaitIdle(vkDevice);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(vkDevice, frames[i].vkCommandPool, nullptr);

            //destroy sync objects
            vkDestroyFence(vkDevice, frames[i].vkRenderFence, nullptr);
            vkDestroySemaphore(vkDevice, frames[i].vkRenderSemaphore, nullptr);
            vkDestroySemaphore(vkDevice, frames[i].vkSwapchainSemaphore, nullptr);
        }

        destroySwapchain();
        vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
        vkDestroyDevice(vkDevice, nullptr);
#if DEBUG
        vkb::destroy_debug_utils_messenger(vkInstance, vkDebugMessenger);
#endif // DEBUG
        vkDestroyInstance(vkInstance, nullptr);
        SDL_DestroyWindow(pWindow);
        
        vkChosenGPU = nullptr;
    }

    // clear engine pointer
    pLoadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout for 1 second
    VK_CHECK(vkWaitForFences(vkDevice, 1, &getCurrentFrame().vkRenderFence, true, 1000000000));
    VK_CHECK(vkResetFences(vkDevice, 1, &getCurrentFrame().vkRenderFence));

    //request image from the swapchain
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(vkDevice, vkSwapchain, 1000000000, getCurrentFrame().vkSwapchainSemaphore, nullptr, &swapchainImageIndex));

    //naming it cmd for shorter writing
    VkCommandBuffer cmdBuff = getCurrentFrame().vkMainCommandBuffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmdBuff, 0));

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmdBuff, &cmdBeginInfo));

    //make the swapchain image into writeable mode before rendering
    vkutil::transitionImage(cmdBuff, vkSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    //make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(frameNumber / 120.f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    //clear image
    vkCmdClearColorImage(cmdBuff, vkSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    //make the swapchain image into presentable mode
    vkutil::transitionImage(cmdBuff, vkSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmdBuff));

    //prepare the submission to the queue. 
    //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    //we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmdBuff);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().vkSwapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().vkRenderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(vkGraphicsQueue, 1, &submit, getCurrentFrame().vkRenderFence));

    //prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that, 
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &vkSwapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &getCurrentFrame().vkRenderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(vkGraphicsQueue, &presentInfo));

    //increase the number of frames drawn
    frameNumber++;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stopRendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stopRendering = false;
                }
            }

            if (e.type == SDL_KEYDOWN)
            {
                fmt::print("{}", (char)e.key.keysym.sym);
            }
        }

        // do not draw if we are minimized
        if (stopRendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

void VulkanEngine::initVulkan()
{
    vkb::InstanceBuilder vkInstBuilder;

    // Create Vulkan instance with default debug features
    vkb::Result<vkb::Instance> builderResult = vkInstBuilder.set_app_name("Sample Vulkan App")
        .request_validation_layers(requestValidationLayers)
#if DEBUG
        .use_default_debug_messenger()
#endif
        .require_api_version(1, 3, 0)
        .build();

    // Vulkan Bootstrap instance
    vkb::Instance vkbInstance = builderResult.value();

    // Store Vulkan instance
    vkInstance = vkbInstance.instance;
#if DEBUG
    vkDebugMessenger = vkbInstance.debug_messenger;
#endif // DEBUG

    // Create Vulkan specific surface
    // Surface stores pixels in main memory. Pixels can be accessed and modify with CPU
    SDL_Vulkan_CreateSurface(pWindow, vkInstance, &vkSurface);

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // Use vkbootstrap to select a gpu. 
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector(vkbInstance);
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(vkSurface)
        .select()
        .value();

    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder(physicalDevice);

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    vkDevice = vkbDevice.device;
    vkChosenGPU = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
    vkGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::initSwapchain()
{
    createSwapchain(vkWindowExtent.width, vkWindowExtent.height);
}

void VulkanEngine::initCommands()
{
    // Create a command pool for commands submitted to the graphics queue.
    // We also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(vkDevice, &commandPoolInfo, nullptr, &frames[i].vkCommandPool));

        // Allocate default command buffer
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].vkCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(vkDevice, &cmdAllocInfo, &frames[i].vkMainCommandBuffer));
    }
}

void VulkanEngine::initSyncStructures()
{
    //create syncronization structures
    //one fence to control when the gpu has finished rendering the frame,
    //and 2 semaphores to syncronize rendering with swapchain
    //we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &frames[i].vkRenderFence));

        VK_CHECK(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &frames[i].vkSwapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &frames[i].vkRenderSemaphore));
    }
}

void VulkanEngine::createSwapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder(vkChosenGPU, vkDevice, vkSurface);

    vkSwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = vkSwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    vkSwapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    vkSwapchain = vkbSwapchain.swapchain;
    vkSwapchainImages = vkbSwapchain.get_images().value();
    vkSwapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroySwapchain()
{
    vkDestroySwapchainKHR(vkDevice, vkSwapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < vkSwapchainImageViews.size(); i++)
    {
        vkDestroyImageView(vkDevice, vkSwapchainImageViews[i], nullptr);
    }
}
