#include "utils.hpp"

// ----------------
// ---- PUBLIC ----
// ----------------

std::vector<vk::ExtensionProperties> Utils::getAvailableExtensions() {
	uint32_t numExtensions{ 0 };

	// request number of extensions available
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

	// pack available extensions into vector
	std::vector<vk::ExtensionProperties> available(numExtensions);
	vk::enumerateInstanceExtensionProperties(nullptr, &numExtensions, available.data());

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
	const std::vector<vk::ExtensionProperties> &available
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
	std::vector<vk::LayerProperties> layers(numLayers);
	vk::enumerateInstanceLayerProperties(&numLayers, layers.data());

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
