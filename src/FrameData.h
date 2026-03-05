#ifndef FRAME_DATA_H
#define FRAME_DATA_H

#include <vk_types.h>

struct FrameData
{
public:
	VkSemaphore swapchainSemaphore;
	VkSemaphore renderSemaphore;
	VkFence renderFence;
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

#endif // !FRAME_DATA_H
