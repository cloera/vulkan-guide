// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#include <vk_types.h>
#include <FrameData.h>

class VulkanEngine {
public:

	VkInstance instance = nullptr;
	VkDevice device = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;

	FrameData frames[FRAME_OVERLAP];
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

#if DEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif
	VkPhysicalDevice chosenGPU = nullptr;
	struct SDL_Window* pWindow{ nullptr };

	VkQueue graphicsQueue = nullptr;
	uint32_t graphicsQueueFamily = 0;

	VkExtent2D windowExtent{ 1700 , 900 };
	VkExtent2D swapchainExtent{ 0, 0 };
	VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
	
	int frameNumber = 0 ;
	bool isInitialized = false ;
	bool stopRendering = false ;
	bool requestValidationLayers = true ;
	bool padding1 = false;
	
	

	static VulkanEngine& Get();
	FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; };

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

	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();
};

#endif // VK_ENGINE_H
