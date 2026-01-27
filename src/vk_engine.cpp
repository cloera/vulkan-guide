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

        SDL_DestroyWindow(pWindow);
        instance = nullptr;
        debugMessenger = nullptr;
        chosenGPU = nullptr;
        device = nullptr;
        surface = nullptr;
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
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    // Vulkan Bootstrap instance
    vkb::Instance vkbInstance = builderResult.value();

    // Store Vulkan instance
    instance = vkbInstance.instance;
    debugMessenger = vkbInstance.debug_messenger;

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
}

void VulkanEngine::initCommands()
{
}

void VulkanEngine::initSyncStructures()
{
}
