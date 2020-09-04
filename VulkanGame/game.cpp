#include <cstring>
#include "game.hpp"
#include "constants.hpp"

void Game::run() {
	initWindow();
	initVulkan();
	main();
	cleanup();
}

void Game::initWindow() {
	// initialize components for GLFW
	glfwInit();

	// indicate that we don't want OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// turn off resizable window
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// create window
	window = glfwCreateWindow(constants::width, constants::height, constants::title, nullptr, nullptr);
}

bool Game::hasRequiredExtensions(const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &available) {
	// attempt to find required extensions inside available extensions
	for (const auto &req : required) {
		bool extFound{ false };

		// check each available extension
		for (const auto &extension : available) {
			if (strcmp(req, extension.extensionName) == 0) {
				extFound = true;
				break;
			}
		}

		if (!extFound) {
			return false;
		}
	}

	return true;
}

bool Game::hasValidationLayerSupport() {
	uint32_t numLayers{ 0 };

	// request number of layers available
	// call vkEnumerate... instead of vk::enumerate... because latter causes errors
	vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

	// put available layers into std::vector
	std::vector<vk::LayerProperties> layers(numLayers);
	vk::enumerateInstanceLayerProperties(&numLayers, layers.data());

	std::cout << "Available layers (" << layers.size() << "):\n";

	for (const auto &layer : layers) {
		std::cout << '\t' << layer.layerName << '\n';
	}

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

void Game::createInstance() {
	// inform driver about how to best optimise the application
	vk::ApplicationInfo appInfo{};

	// application name and version
	appInfo.pApplicationName = "Testing Triangles";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

	// engine name and version
	appInfo.pEngineName = "Carbon Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	// tells driver which global extensions and validation layers to use
	vk::InstanceCreateInfo createInfo{};

	// give driver application information
	createInfo.pApplicationInfo = &appInfo;

	// keep track of number of available extensions
	uint32_t numExtensions{ 0 };

	// request number of extensions available
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

	// put available extensions into std::vector
	std::vector<vk::ExtensionProperties> extensions(numExtensions);
	vk::enumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data());

	bool t = hasValidationLayerSupport();

	std::cout << "Available extensions (" << extensions.size() << "):\n";

	for (const auto &extension : extensions) {
		std::cout << '\t' << extension.extensionName << '\n';
	}

	numExtensions = 0;

	// get required extensions and convert to std::vector<string>
	const char** reqExts{ glfwGetRequiredInstanceExtensions(&numExtensions) };
	const std::vector<const char *> required(reqExts, reqExts + numExtensions);

	// check if available extensions have required extensions
	if (!hasRequiredExtensions(required, extensions)) {
		throw std::runtime_error("Failed to find required extensions!");
	}

	createInfo.enabledExtensionCount = numExtensions;
	createInfo.ppEnabledExtensionNames = reqExts;
	createInfo.enabledLayerCount = 0;

	// attempt to create instance
	if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create instance!");
	}
}

void Game::initVulkan() {
	// create the Vulkan instance
	createInstance();
}

void Game::main() {
	// keep pulling from event queue until window is closed
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Game::cleanup() {
	// destroy and free window
	glfwDestroyWindow(window);

	// close up all processes
	glfwTerminate();
}