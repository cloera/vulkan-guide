//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <VkBootstrap.h>
#include <vk_initializers.h>

#include <chrono>
#include <thread>

VulkanEngine* pLoadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *pLoadedEngine; }
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
        windowExtent.width,
        windowExtent.height,
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
        destroySwapchain();
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
#if DEBUG
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
#endif // DEBUG
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(pWindow);
        
        chosenGPU = nullptr;
    }

    // clear engine pointer
    pLoadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // nothing yet
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
    instance = vkbInstance.instance;
#if DEBUG
    debugMessenger = vkbInstance.debug_messenger;
#endif // DEBUG

    // Create Vulkan specific surface
    // Surface stores pixels in main memory. Pixels can be accessed and modify with CPU
    SDL_Vulkan_CreateSurface(pWindow, instance, &surface);

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
        .set_surface(surface)
        .select()
        .value();

    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder(physicalDevice);

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkbDevice.device;
    chosenGPU = physicalDevice.physical_device;
}

void VulkanEngine::initSwapchain()
{
    createSwapchain(windowExtent.width, windowExtent.height);
}

void VulkanEngine::initCommands()
{
}

void VulkanEngine::initSyncStructures()
{
}

void VulkanEngine::createSwapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder(chosenGPU, device, surface);

    swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    swapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    swapchain = vkbSwapchain.swapchain;
    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroySwapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < swapchainImageViews.size(); i++)
    {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
}
