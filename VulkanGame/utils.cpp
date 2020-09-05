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
#endif // NDEBUG

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
#endif // NDEBUG

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

