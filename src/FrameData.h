#ifndef FRAME_DATA_H
#define FRAME_DATA_H

#include <vk_types.h>

struct FrameData
{
public:
	VkSemaphore vkSwapchainSemaphore;
	VkSemaphore vkRenderSemaphore;
	VkFence vkRenderFence;
	VkCommandPool vkCommandPool;
	VkCommandBuffer vkMainCommandBuffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

#endif // !FRAME_DATA_H
