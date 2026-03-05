#ifndef VK_IMAGES_H
#define VK_IMAGES_H

#include <vulkan/vulkan.h>

namespace vkutil {

	void transitionImage(VkCommandBuffer cmdBuff, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
};

#endif // !VK_IMAGES_H