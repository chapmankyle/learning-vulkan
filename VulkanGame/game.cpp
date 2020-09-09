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

void Game::createSurface() {
	// attempt to create window surface
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
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

#ifndef NDEBUG
	std::cout << '\n';
#endif // !NDEBUG

	// select single GPU to use as `physicalDevice`
	for (const auto &d : devices) {
		score = Utils::getDeviceScore(d, surface);
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
	Utils::QueueFamilyIndices indices{ Utils::findQueueFamilies(physicalDevice, surface) };

	std::vector<VkDeviceQueueCreateInfo> createInfoQueues;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	// priority given to this queue
	float queuePriority = 1.0f;

	for (uint32_t queueFam : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		queueCreateInfo.queueFamilyIndex = queueFam;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		createInfoQueues.push_back(queueCreateInfo);
	}

	// specify device features to use (such as geometry shaders etc)
	VkPhysicalDeviceFeatures deviceFeats{};

	// start creating device info
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(createInfoQueues.size());
	createInfo.pQueueCreateInfos = createInfoQueues.data();

	createInfo.pEnabledFeatures = &deviceFeats;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(constants::deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = constants::deviceExtensions.data();

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
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &graphicsQueue);
}


void Game::createSwapchain() {
	Utils::SwapChainSupportDetails swapchainSupport{ Utils::querySwapChainSupport(physicalDevice, surface) };

#ifndef NDEBUG
	std::cout << "\n-- Physical device capabilities --\n";
	std::cout << "\twidth = " << swapchainSupport.capabilities.currentExtent.width << '\n';
	std::cout << "\theight = " << swapchainSupport.capabilities.currentExtent.height << '\n';
	std::cout << "\tmax image count = " << swapchainSupport.capabilities.maxImageCount << '\n';
	std::cout << "\tmin image count = " << swapchainSupport.capabilities.minImageCount << '\n';
	std::cout << "\tmax image extent (width) = " << swapchainSupport.capabilities.maxImageExtent.width << '\n';
	std::cout << "\tmax image extent (height) = " << swapchainSupport.capabilities.maxImageExtent.height << '\n';
	std::cout << "\tmin image extent (width) = " << swapchainSupport.capabilities.minImageExtent.width << '\n';
	std::cout << "\tmin image extent (height) = " << swapchainSupport.capabilities.minImageExtent.height << '\n';
#endif // !NDEBUG

	// get surface formate, present mode and extent
	VkSurfaceFormatKHR surfaceFormat{ Utils::chooseSwapSurfaceFormat(swapchainSupport.formats) };
	VkPresentModeKHR presentMode{ Utils::chooseSwapPresentMode(swapchainSupport.presentModes) };
	VkExtent2D extent{ Utils::chooseSwapExtent(swapchainSupport.capabilities) };

	// choose number of images to have in swap chain
	uint32_t imageCount{ swapchainSupport.capabilities.minImageCount + 1 };
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}
}


void Game::initVulkan() {
	// create the Vulkan instance
	createInstance();

	// setup a debug messenger
	setupDebugMessenger();

	// create KHR surface
	createSurface();

	// pick a single graphics card to use
	pickPhysicalDevice();

	// create and link logical device to physical device
	createLogicalDevice();

	// create the swap chain
	createSwapchain();
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

	// destory surface
	vkDestroySurfaceKHR(instance, surface, nullptr);

	// free instance
	vkDestroyInstance(instance, nullptr);

	// destroy and free window
	glfwDestroyWindow(window);

	// close up all processes
	glfwTerminate();
}