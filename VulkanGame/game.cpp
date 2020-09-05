#include "game.hpp"

// ----------------
// ---- PUBLIC ----
// ----------------

void Game::run() {
	initWindow();
	initVulkan();
	main();
	cleanup();
}

// -----------------
// ---- PRIVATE ----
// -----------------

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

void Game::createInstance() {
	// check support for validation layers
	if (constants::enableValidationLayers && !Utils::hasValidationLayerSupport()) {
		throw std::runtime_error("No support for validation layers!");
	}

	// inform driver about how to best optimise the application
	vk::ApplicationInfo appInfo{};

	appInfo.pApplicationName = "Testing Triangles";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Carbon Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	// tells driver which global extensions and validation layers to use
	vk::InstanceCreateInfo createInfo{};

	// give driver application information
	createInfo.pApplicationInfo = &appInfo;

	const auto available{ Utils::getAvailableExtensions() };
	const auto required{ Utils::getRequiredExtensions() };

	// check if available extensions have required extensions
	if (!Utils::hasRequiredExtensions(required, available)) {
		throw std::runtime_error("Failed to find required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(required.size());
	createInfo.ppEnabledExtensionNames = required.data();
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