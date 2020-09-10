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

	// fill in swapchain info
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	// specify surface to which the swap chain is tied
	createInfo.surface = surface;

	// fill in details of swap chain images
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // specifies number of layers each image consists of
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies kind of operations that image will be used for
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT -> used when rendering images to seperate image for post-processing

	// specify how to handle swap chain images that will be used across multiple queue families
	Utils::QueueFamilyIndices indices{ Utils::findQueueFamilies(physicalDevice, surface) };
	uint32_t queueIndices[]{ indices.graphicsFamily.value(), indices.presentFamily.value() };

#ifndef NDEBUG
	std::cout << "\nGraphics family index: " << indices.graphicsFamily.value() << '\n';
	std::cout << "Presentation family index: " << indices.presentFamily.value() << '\n';
#endif // !NDEBUG

	// ways to handle images accessed from multiple queues
	// - VK_SHARING_MODE_EXCLUSIVE : image is owned by one queue family at a time and ownership
	//   must be explicitly transferred before being used by another queue family (best performance)
	// - VK_SHARING_MODE_CONCURRENT : images can be used across multiple queue families without
	//   explicit ownership transfers
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	// you can apply a transform to the image (set to current transform for no transformation)
	createInfo.preTransform = swapchainSupport.capabilities.currentTransform;

	// ignore alpha channel (can be used for blending with other windows)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;

	// clip objects that are obscured (best performance)
	createInfo.clipped = VK_TRUE;

	// can specify previous swapchain if current one becomes invalid or unoptimized
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// create swapchain
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}

	// get images from swapchain and put into vector
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapchainImages.data());

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;
}


void Game::createImageViews() {
	swapchainImageViews.resize(swapchainImages.size());

	// loop over swapchain images
	for (size_t i{ 0 }; i < swapchainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];

		// view type and format specify how image data should be interpreted
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainImageFormat;

		// components allow you to map colour channels
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// subresourceRange describes what the purpose of the image is
		// and which parts should be accessed
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// create image views
		if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
	}
}


void Game::createGraphicsPipeline() {
	// get bytecode of shaders
	std::vector<char> vertShaderCode{ Utils::readFile("shaders/vert.spv") };
	std::vector<char> fragShaderCode{ Utils::readFile("shaders/frag.spv") };

#ifndef NDEBUG
	std::cout << "\nVertex shader size : " << vertShaderCode.size() << " bytes\n";
	std::cout << "Fragment shader size : " << fragShaderCode.size() << " bytes\n";
#endif // NDEBUG

	// create modules from bytecode
	VkShaderModule vertShaderModule{ Utils::createShaderModule(device, vertShaderCode) };
	VkShaderModule fragShaderModule{ Utils::createShaderModule(device, fragShaderCode) };

	// create vertex pipeline stage
	VkPipelineShaderStageCreateInfo vertShaderInfo{};
	vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderInfo.module = vertShaderModule;
	vertShaderInfo.pName = "main";

	// create fragment pipeline stage
	VkPipelineShaderStageCreateInfo fragShaderInfo{};
	fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderInfo.module = fragShaderModule;
	fragShaderInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderInfo, fragShaderInfo };

	// free shader modules
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
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
	createImageViews();

	// create customizable graphics pipeline
	createGraphicsPipeline();
}

void Game::main() {
	// keep pulling from event queue until window is closed
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Game::cleanup() {
	// destroy image views
	for (auto imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	// destroys the swap chain
	vkDestroySwapchainKHR(device, swapChain, nullptr);

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

