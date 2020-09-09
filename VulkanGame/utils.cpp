#include "utils.hpp"

// ----------------
// ---- PUBLIC ----
// ----------------

std::vector<VkExtensionProperties> Utils::getAvailableExtensions() {
	uint32_t numExtensions{ 0 };

	// request number of extensions available
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

	// pack available extensions into vector
	std::vector<VkExtensionProperties> available(numExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, available.data());

#ifndef NDEBUG
	std::cout << available.size() << " supported extensions.\n";
#endif // !NDEBUG

	return available;
}

std::vector<const char*> Utils::getRequiredExtensions() {
	uint32_t numExtensions{ 0 };

	// get required extensions and convert to std::vector<string>
	const char** reqExts{ glfwGetRequiredInstanceExtensions(&numExtensions) };
	std::vector<const char *> required(reqExts, reqExts + numExtensions);

	// additional extension if validation layers are included
	if (constants::enableValidationLayers) {
		required.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return required;
}

bool Utils::hasRequiredExtensions(
	const std::vector<const char *> &required,
	const std::vector<VkExtensionProperties> &available
) {
	// keep track of if extension has been found
	bool extensionFound;

	// attempt to find required extensions inside available extensions
	for (const auto &req : required) {
		extensionFound = false;

		for (const auto &ext : available) {
			if (strcmp(req, ext.extensionName) == 0) {
				extensionFound = true;
			}
		}

		if (!extensionFound) {
			return false;
		}
	}

	return true;
}

bool Utils::hasValidationLayerSupport() {
	uint32_t numLayers{ 0 };

	// request number of layers available
	// call vkEnumerate... instead of vk::enumerate... because latter causes errors
	vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

	// put available layers into std::vector
	std::vector<VkLayerProperties> layers(numLayers);
	vkEnumerateInstanceLayerProperties(&numLayers, layers.data());

#ifndef NDEBUG
	std::cout << layers.size() << " supported layers.\n";
#endif // !NDEBUG

	// check if each validation layer is in available layers
	for (const auto &valLayer : constants::validationLayers) {
		bool layerFound{ false };

		// check all layers
		for (const auto &layer : layers) {
			if (strcmp(valLayer, layer.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

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

void Utils::showDeviceProperties(const VkPhysicalDeviceProperties &deviceProps) {
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

	std::cout << "[Physical Device] " << deviceProps.deviceName << '\n';
	std::cout << "\tType: " << devType << '\n';
	std::cout << "\tVendor ID: " << deviceProps.vendorID << '\n';
	std::cout << "\tMaximum clip distances: " << deviceProps.limits.maxClipDistances << '\n';
	std::cout << "\tMaximum cull distances: " << deviceProps.limits.maxCullDistances << '\n';
	std::cout << "\tMaximum size of 2D textures: " << deviceProps.limits.maxImageDimension2D << '\n';
	std::cout << "\tMaximum size of 3D textures: " << deviceProps.limits.maxImageDimension3D << '\n';
	std::cout << "\tMaximum number of viewports: " << deviceProps.limits.maxViewports << '\n';
}

void Utils::showDeviceProperties(const VkPhysicalDevice &device) {
	// get properties of device
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

	showDeviceProperties(deviceProps);
}

Utils::QueueFamilyIndices Utils::findQueueFamilies(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
	Utils::QueueFamilyIndices indices;

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
			indices.graphicsFamily = i;
		}

		// query surface support
		VkBool32 presentSupport{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.containsValue()) {
			break;
		}

		i++;
	}

	return indices;
}

bool Utils::hasDeviceExtensionSupport(const VkPhysicalDevice &device) {
	// query device extension properties
	uint32_t numExtensions{ 0 };
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);

	// put into vector
	std::vector<VkExtensionProperties> available(numExtensions);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, available.data());

	std::set<std::string> required(constants::deviceExtensions.begin(), constants::deviceExtensions.end());

	// remove each found extension
	for (const auto& ext : available) {
		required.erase(ext.extensionName);
	}

	// empty if all extensions were found and removed
	return required.empty();
}

int Utils::getDeviceScore(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
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

	// cannot use swap chain without device extensions
	if (!hasDeviceExtensionSupport(device)) {
		return 0;
	}

	// check swap chain support
	SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(device, surface) };
	if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
		return 0;
	}

	// find queue family that supports needed computation
	QueueFamilyIndices index{ findQueueFamilies(device, surface) };

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

#ifndef NDEBUG
	std::cout << "\tScore: " << score << "\n\n";
#endif // !NDEBUG

	return score;
}

Utils::SwapChainSupportDetails Utils::querySwapChainSupport(
	const VkPhysicalDevice &device,
	const VkSurfaceKHR &surface
) {
	SwapChainSupportDetails details;

	// get surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// query supported surface formats
	uint32_t formatCount{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	// query supported presentation modes
	uint32_t presentModeCount{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


VkSurfaceFormatKHR Utils::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	// B8G8R8A8 -> store as B, G, R and A in that order (as 8 bit unsigned integers, so 32 bits per pixel)
	// SRGB is standard format for textures
	for (const auto &fmt : availableFormats) {
		if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return fmt;
		}
	}

	return availableFormats[0];
}


VkPresentModeKHR Utils::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
	for (const auto &presentMode : availablePresentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D Utils::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}

	// create extent with current window bounds
	VkExtent2D extent{ constants::width, constants::height };

	// clamp values between allowed min and max extents
	extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
	extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

	return extent;
}

