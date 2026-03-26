#include "SwapChain.h"

#include <VkBootstrap.h>

SwapChain::SwapChain(VkDevice& device)
{
	this->vkDevice = device;
}

SwapChain::~SwapChain()
{
}

SwapChain::SwapChain(const SwapChain& other)
{
    this->vkDevice = other.vkDevice;
    this->vkSwapchain = other.vkSwapchain;
    this->vkSwapchainImages = other.vkSwapchainImages;
    this->vkSwapchainImageViews = other.vkSwapchainImageViews;
    this->vkSwapchainExtent = other.vkSwapchainExtent;
    this->vkSwapchainImageFormat = other.vkSwapchainImageFormat;
}

SwapChain& SwapChain::operator=(const SwapChain& other)
{
    if (this != &other)
    {
        this->vkDevice = other.vkDevice;
        this->vkSwapchain = other.vkSwapchain;
        this->vkSwapchainImages = other.vkSwapchainImages;
        this->vkSwapchainImageViews = other.vkSwapchainImageViews;
        this->vkSwapchainExtent = other.vkSwapchainExtent;
        this->vkSwapchainImageFormat = other.vkSwapchainImageFormat;
    }
	return *this;
}

SwapChain::SwapChain(SwapChain&& other) noexcept
{
    this->vkDevice = other.vkDevice;
    this->vkSwapchain = other.vkSwapchain;
    this->vkSwapchainImages = other.vkSwapchainImages;
    this->vkSwapchainImageViews = other.vkSwapchainImageViews;
    this->vkSwapchainExtent = other.vkSwapchainExtent;
    this->vkSwapchainImageFormat = other.vkSwapchainImageFormat;

	// Reset the other object's members to default values  
    other.vkDevice = nullptr;
    other.vkSwapchain = nullptr;
    other.vkSwapchainImages.clear();
    other.vkSwapchainImageViews.clear();
    other.vkSwapchainExtent = { 0, 0 };
	other.vkSwapchainImageFormat = VK_FORMAT_UNDEFINED;
}

SwapChain& SwapChain::operator=(SwapChain&& other) noexcept
{
    if (this != &other)
    {
        this->vkDevice = other.vkDevice;
        this->vkSwapchain = other.vkSwapchain;
        this->vkSwapchainImages = other.vkSwapchainImages;
        this->vkSwapchainImageViews = other.vkSwapchainImageViews;
        this->vkSwapchainExtent = other.vkSwapchainExtent;
        this->vkSwapchainImageFormat = other.vkSwapchainImageFormat;

        // Reset the other object's members to default values  
        other.vkDevice = nullptr;
        other.vkSwapchain = nullptr;
        other.vkSwapchainImages.clear();
        other.vkSwapchainImageViews.clear();
        other.vkSwapchainExtent = { 0, 0 };
        other.vkSwapchainImageFormat = VK_FORMAT_UNDEFINED;
    }
    return *this;
}

void SwapChain::create(VkPhysicalDevice &vkChosenGPU, VkSurfaceKHR &vkSurface, uint32_t width, uint32_t height)
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

void SwapChain::destroy()
{
    vkDestroySwapchainKHR(vkDevice, vkSwapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < vkSwapchainImageViews.size(); i++)
    {
        vkDestroyImageView(vkDevice, vkSwapchainImageViews[i], nullptr);
    }
}

VkSwapchainKHR& SwapChain::getSwapchain()
{
	return vkSwapchain;
}

const std::vector<VkImage>& SwapChain::getSwapchainImages() const
{
	return vkSwapchainImages;
}

const std::vector<VkImageView>& SwapChain::getSwapchainImageViews() const
{
	return vkSwapchainImageViews;
}

VkExtent2D SwapChain::getSwapchainExtent() const
{
	return vkSwapchainExtent;
}

VkFormat SwapChain::getSwapchainImageFormat() const
{
    return vkSwapchainImageFormat;
}
