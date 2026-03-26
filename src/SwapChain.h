#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include <vk_types.h>

class SwapChain
{
public:
	// Constructor
	SwapChain() = default;
	SwapChain(VkDevice &device);
	// Destructor
	~SwapChain();
	// Copy Constructor
	SwapChain(const SwapChain& other);
	// Copy Assignment Operator
	SwapChain& operator=(const SwapChain& other);
	// Move Constructor
	SwapChain(SwapChain&& other) noexcept;
	// Move Assignment Operator
	SwapChain& operator=(SwapChain&& other) noexcept;


	void create(VkPhysicalDevice &vkChosenGPU, VkSurfaceKHR &vkSurface, uint32_t width, uint32_t height);
	void destroy();

	VkSwapchainKHR& getSwapchain();
	const std::vector<VkImage>& getSwapchainImages() const;
	const std::vector<VkImageView>& getSwapchainImageViews() const;
	VkExtent2D getSwapchainExtent() const;
	VkFormat getSwapchainImageFormat() const;

private:
	VkDevice vkDevice = nullptr;

	VkSwapchainKHR vkSwapchain = nullptr;
	std::vector<VkImage> vkSwapchainImages;
	std::vector<VkImageView> vkSwapchainImageViews;
	VkExtent2D vkSwapchainExtent{ 0, 0 };
	VkFormat vkSwapchainImageFormat = VK_FORMAT_UNDEFINED;
	const int padding1 = 0;
};

#endif // !SWAP_CHAIN_H

