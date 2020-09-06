#include "utils.hpp"

// ----------------
// ---- PUBLIC ----
// ----------------

VKAPI_ATTR VkBool32 VKAPI_CALL Utils::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData
) {
	// show debug message as error
	std::cerr << "Validation layer: " << pCallbackData->pMessage << '\n';

	return VK_FALSE;
}

void Utils::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = Utils::debugCallback;
	createInfo.pUserData = nullptr;
}

VkResult Utils::createDebugUtilsMessenger(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pDebugMessenger
) {
	// get function since it is external (not loaded in Vulkan initially)
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Utils::destroyDebugUtilsMessenger(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks *pAllocator
) {
	// get function to destroy debug messenger
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Utils::showDeviceProperties(VkPhysicalDeviceProperties deviceProps) {
	const char *devType;
	switch (deviceProps.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			devType = "Integrated GPU";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			devType = "Discrete GPU";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			devType = "Virtual GPU";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			devType = "CPU";
			break;
		default:
			devType = "Other";
			break;
	}

	std::cout << "[Physical Device] Name: " << deviceProps.deviceName << '\n';
	std::cout << "                  Type: " << devType << '\n';
	std::cout << "                  Vendor ID: " << deviceProps.vendorID << '\n';
	std::cout << "                  Maximum clip distances: " << deviceProps.limits.maxClipDistances << '\n';
	std::cout << "                  Maximum cull distances: " << deviceProps.limits.maxCullDistances << '\n';
	std::cout << "                  Maximum size of 2D textures: " << deviceProps.limits.maxImageDimension2D << '\n';
	std::cout << "                  Maximum size of 3D textures: " << deviceProps.limits.maxImageDimension3D << '\n';
	std::cout << "                  Maximum number of viewports: " << deviceProps.limits.maxViewports << "\n\n";
}

void Utils::showDeviceProperties(VkPhysicalDevice device) {
	// get properties of device
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

	showDeviceProperties(deviceProps);
}

Utils::QueueFamilyIndices Utils::findQueueFamilies(VkPhysicalDevice device) {
	Utils::QueueFamilyIndices index;

	// get number of queue families
	uint32_t numQueueFamilies{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);

	// store queue families
	std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, queueFamilies.data());

	int i{ 0 };

	// find first index of queue family that supports graphics commands
	for (const auto &queueFam : queueFamilies) {
		if (queueFam.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			index.graphicsFamily = i;
			break;
		}

		i++;
	}

	return index;
}

int Utils::getDeviceScore(VkPhysicalDevice device) {
	// device properties (name, type, supported Vulkan version etc.)
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

#ifndef NDEBUG
	showDeviceProperties(deviceProps);
#endif // !NDEBUG

	// device features (texture compression, 64-bit floats, multi-viewport rendering etc.)
	VkPhysicalDeviceFeatures deviceFeats;
	vkGetPhysicalDeviceFeatures(device, &deviceFeats);

	// cannot function without geometry shader
	if (!deviceFeats.geometryShader) {
		return 0;
	}

	// find queue family that supports needed computation
	QueueFamilyIndices index{ findQueueFamilies(device) };

	if (!index.containsValue()) {
		return 0;
	}

	int score{ 0 };

	// favour discrete GPU
	if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// maximum size of textures affects quality
	score += deviceProps.limits.maxImageDimension2D;

	return score;
}

