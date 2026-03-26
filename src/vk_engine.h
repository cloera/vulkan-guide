// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#include <vk_types.h>
#include <FrameData.h>
#include <SwapChain.h>

class VulkanEngine {
public:

	VkInstance vkInstance = nullptr;
	VkDevice vkDevice = nullptr;
	VkSurfaceKHR vkSurface = nullptr;
	SwapChain swapChain;
	FrameData frames[FRAME_OVERLAP];


#if DEBUG
	VkDebugUtilsMessengerEXT vkDebugMessenger = nullptr;
#endif
	struct SDL_Window* pWindow{ nullptr };
	const VkExtent2D vkWindowExtent{ 1700 , 900 };

	VkPhysicalDevice vkChosenGPU = nullptr;
	VkQueue vkGraphicsQueue = nullptr;
	uint32_t graphicsQueueFamily = 0;
	
	int frameNumber = 0 ;
	bool isInitialized = false ;
	bool stopRendering = false ;
	const bool requestValidationLayers = true ;
	const bool padding1 = false;

	const static VulkanEngine& Get();
	const FrameData& getCurrentFrame() const { return frames[frameNumber % FRAME_OVERLAP]; };

	//initializes everything in the engine
	void init();
	//shuts down the engine
	void cleanup();
	//draw loop
	void draw();
	//run main loop
	void run();

private:
	
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initSyncStructures();

	//void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();
};

#endif // VK_ENGINE_H
