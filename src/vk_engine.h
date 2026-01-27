// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#include <vk_types.h>

class VulkanEngine {
public:

	VkInstance instance = nullptr;
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	VkPhysicalDevice chosenGPU = nullptr;
	VkDevice device = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkExtent2D windowExtent { 1700 , 900 };
	int frameNumber = 0 ;
	bool isInitialized = false ;
	bool stopRendering = false ;
	bool requestValidationLayers = true ;
	bool padding4;

	struct SDL_Window* pWindow{ nullptr };

	static VulkanEngine& Get();

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
};

#endif // VK_ENGINE_H
