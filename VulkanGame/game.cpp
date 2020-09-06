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
	VkApplicationInfo appInfo{};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Work In Progress: Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Carbon Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	// tells driver which global extensions and validation layers to use
	VkInstanceCreateInfo createInfo{};

	// give driver application information
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const auto available{ Utils::getAvailableExtensions() };
	const auto required{ Utils::getRequiredExtensions() };

	// check if available extensions have required extensions
	if (!Utils::hasRequiredExtensions(required, available)) {
		throw std::runtime_error("Failed to find required extensions!");
	}

	// enable required extensions
	createInfo.enabledExtensionCount = static_cast<uint32_t>(required.size());
	createInfo.ppEnabledExtensionNames = required.data();

	// for checking any errors during debug messenger creation and deletion
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	// enable validation layers if flag set
	if (constants::enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(constants::validationLayers.size());
		createInfo.ppEnabledLayerNames = constants::validationLayers.data();

		Utils::fillDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// attempt to create instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}
}

void Game::setupDebugMessenger() {
	// no need for a debug messenger if validation layers are disabled
	if (!constants::enableValidationLayers) {
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	Utils::fillDebugMessengerCreateInfo(createInfo);

	// check result of creating debug messenger
	if (Utils::createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup debug messenger!");
	}
}

void Game::pickPhysicalDevice() {
	// query number of graphics cards
	uint32_t numDevices{ 0 };
	vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);

	if (numDevices == 0) {
		throw std::runtime_error("Failed to find any GPUs with Vulkan support!");
	}

	// store all devices in vector
	std::vector<VkPhysicalDevice> devices(numDevices);
	vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());

	// ordered map to automatically sort by device score
	std::multimap<int, VkPhysicalDevice> candidates;

	int score{ 0 };

	// select single GPU to use as `physicalDevice`
	for (const auto &d : devices) {
		score = Utils::getDeviceScore(d);
		candidates.insert(std::make_pair(score, d));
	}

	// check if best candidate is even suitable
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
	} else {
		throw std::runtime_error("Failed to find a suitable GPU!");
	}

#ifndef NDEBUG
	std::cout << "-- Selected device --\n";
	Utils::showDeviceProperties(physicalDevice);
#endif // !NDEBUG
}

void Game::createLogicalDevice() {
	// find queue family index
	Utils::QueueFamilyIndices index{ Utils::findQueueFamilies(physicalDevice) };

	VkDeviceQueueCreateInfo queueCreateInfo{};

	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = index.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	// priority given to this queue
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// specify device features to use (such as geometry shaders etc)
	VkPhysicalDeviceFeatures deviceFeats{};

	// start creating device info
	VkDeviceCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeats;
	createInfo.enabledExtensionCount = 0;

	if (constants::enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(constants::validationLayers.size());
		createInfo.ppEnabledLayerNames = constants::validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	// attempt to create device
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a logical device!");
	}

	// create single graphics queue from device
	vkGetDeviceQueue(device, index.graphicsFamily.value(), 0, &graphicsQueue);
}

void Game::initVulkan() {
	// create the Vulkan instance
	createInstance();

	// setup a debug messenger
	setupDebugMessenger();

	// pick a single graphics card to use
	pickPhysicalDevice();

	// create and link logical device to physical device
	createLogicalDevice();
}

void Game::main() {
	// keep pulling from event queue until window is closed
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Game::cleanup() {
	// destroy logical device
	vkDestroyDevice(device, nullptr);

	// destory debug messenger
	if (constants::enableValidationLayers) {
		Utils::destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
	}

	// free instance
	vkDestroyInstance(instance, nullptr);

	// destroy and free window
	glfwDestroyWindow(window);

	// close up all processes
	glfwTerminate();
}