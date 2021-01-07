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

void Game::framebufferResizeCallback(GLFWwindow * window, int width, int height) {
	auto app = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void Game::initWindow() {
	// initialize components for GLFW
	glfwInit();

	// indicate that we don't want OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// create window
	window = glfwCreateWindow(constants::width, constants::height, constants::title, nullptr, nullptr);

	// tell GLFW that the current instance has the window
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
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
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
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
	VkExtent2D extent{ Utils::chooseSwapExtent(window, swapchainSupport.capabilities) };

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


void Game::createRenderPass() {
	// single colour buffer attachment
	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = swapchainImageFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	// determine what to do with data in attachment before and after rendering
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear values at start
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // rendered constants will be stored in memory

	// apply colour and depth data
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// decide on layout of images being rendered
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // images to be presented in swap chain

	// attachment reference for colours
	VkAttachmentReference colourAttachmentRef{};
	colourAttachmentRef.attachment = 0;
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// create the subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// add colour attachment to subpass
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;

	// subpass dependencies
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// describe render pass
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colourAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
}


void Game::createDescriptorSetLayout() {
	// create binding with vertex shader
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;

	// specify where descriptor will be referenced
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// used for image sampling
	uboBinding.pImmutableSamplers = nullptr;

	// create descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
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

	VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderInfo, fragShaderInfo };

	// vertex input pipeline stage
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDesc = Utils::Vertex::getBindingDescription();
	auto attributeDesc = Utils::Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

	// create input assembly stage
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport describes region of framebuffer that output will render to
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// scissor rectangle defines region in which pixels will be stored
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;

	// viewport state info
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport; // possible to have many viewports
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// create rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // if set to VK_TRUE, disables output to framebuffer
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // FILL indicates to fill area of polygons with fragments
	rasterizer.lineWidth = 1.0f; // describes thickness of lines in terms of number of fragments
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // type of face culling to use
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be considered

	rasterizer.depthBiasEnable = VK_FALSE; // useful for shadown mapping
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// outline multisampling pipeline
	// performs anti-aliasing by combining fragment shader results of multiple
	// polygons that rasterize to the same pixel
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// colour blending (add or combine)
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	// disable blending
	//colorBlendAttachment.blendEnable = VK_FALSE;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// enable alpha blending
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// array of structures for all framebuffers
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// change states in pipeline without recreating pipeline
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	// create pipeline layout
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	// add in the pipeline stages
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	// add fixed function stage to pipeline
	pipelineInfo.layout = pipelineLayout;

	// add render pass and subpass
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0; // index of subpass

	// can create a pipeline from an existing pipeline, but we don't have an existing one yet
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	// attempt to create graphics pipeline
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	// free shader modules
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}


void Game::createFramebuffers() {
	// hold all image views
	swapchainFramebuffers.resize(swapchainImageViews.size());

	// create framebuffer for each image view
	for (size_t i{ 0 }; i < swapchainImageViews.size(); i++) {
		VkImageView attachments[]{ swapchainImageViews[i] };

		// create framebuffer
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}


void Game::createCommandPool() {
	Utils::QueueFamilyIndices queueFamilyIndices{ Utils::findQueueFamilies(physicalDevice, surface) };

	// create command pool
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
}


void Game::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// begin recording command buffer
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	// copy buffer
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	// end recording command buffer
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}


void Game::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer &buffer,
	VkDeviceMemory &bufferMemory
) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// create buffer
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	// memory requirements
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, buffer, &memReqs);

	// setup allocation of memory
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = Utils::findMemoryType(
		physicalDevice,
		memReqs.memoryTypeBits,
		properties
	);

	// allocate memory
	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	// bind memory if allocation was successful
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}


void Game::createVertexBuffer() {
	VkDeviceSize bufferSize{ sizeof(vertices[0]) * vertices.size() };

	// create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// map staging buffer into CPU-accessible memory
	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	// create vertex buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory
	);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void Game::createIndexBuffer() {
	VkDeviceSize bufferSize{ sizeof(vertexIndices[0]) * vertexIndices.size() };

	// create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	// map staging buffer into CPU-accessible memory
	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertexIndices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	// create vertex buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory
	);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void Game::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	// resize vectors since we have per swapchain image
	uniformBuffers.resize(swapchainImages.size());
	uniformBuffersMemory.resize(swapchainImages.size());

	// create each buffer
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i],
			uniformBuffersMemory[i]
		);
	}
}


void Game::createCommandBuffers() {
	// create a command buffer for each framebuffer in swapchain
	commandBuffers.resize(swapchainFramebuffers.size());

	// allocation information
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // submit to queue for execution
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// create command buffer
	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	// record command buffers
	for (size_t i{ 0 }; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer for framebuffer " + i);
		}

		// start render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[i];

		// size of render area
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;

		// define the clear colour
		VkClearValue clearColour = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColour;

		// begin the render pass
		// VK_SUBPASS_CONTENTS_INLINE - embed render pass commands in primary command buffer
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind graphics pipeline
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		// specify vertex buffers
		VkBuffer vertexBuffers[]{ vertexBuffer };
		VkDeviceSize offsets[]{ 0 };

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// draw from index buffer
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(vertexIndices.size()), 1, 0, 0, 0);

		// end render pass
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}


void Game::recreateSwapchain() {
	int width{ 0 };
	int height{ 0 };

	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwWaitEvents();
		glfwGetFramebufferSize(window, &width, &height);
	}

	vkDeviceWaitIdle(device);

	cleanupSwapchain();

	createSwapchain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createUniformBuffers();
	createCommandBuffers();
}


void Game::createSyncObjects() {
	imageAvailableSemaphores.resize(constants::maxFramesInFlight);
	renderFinishedSemaphores.resize(constants::maxFramesInFlight);

	inFlightFences.resize(constants::maxFramesInFlight);
	imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i{ 0 }; i < constants::maxFramesInFlight; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
		) {
			throw std::runtime_error("Failed to create semaphores!");
		}
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
	createImageViews();

	// create render pass
	createRenderPass();

	// create binding between shader and model
	createDescriptorSetLayout();

	// create customizable graphics pipeline
	createGraphicsPipeline();

	// framebuffers for images in swap chain
	createFramebuffers();

	// creates the command pool for command buffers
	createCommandPool();

	// create buffers
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createCommandBuffers();

	// semaphores for synchronization
	createSyncObjects();
}


void Game::updateUniformBuffer(uint32_t currImg) {
	// set starting time to static so it is calculated once
	static auto startTime = std::chrono::steady_clock::now();

	// get elapsed time
	auto currTime = std::chrono::steady_clock::now();
	float elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currTime - startTime).count();

	// update UBO
	UniformBufferObject ubo{};

	ubo.model = glm::rotate(glm::mat4(1.0f), elapsed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / static_cast<float>(swapchainExtent.height), 0.1f, 10.0f);

	// flip Y co-ordinates (so they show the model in the correct orientation)
	ubo.proj[1][1] *= -1;

	// copy data to memory
	void *data;
	vkMapMemory(device, uniformBuffersMemory[currImg], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBuffersMemory[currImg]);
}


void Game::drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIdx;

	// acquire image from swapchain
	VkResult result = vkAcquireNextImageKHR(
		device,
		swapChain,
		UINT64_MAX,
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIdx
	);

	// out-of-date swapchain, recreate
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swapchain images!");
	}

	// check if previous frame is using image
	if (imagesInFlight[imageIdx] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);
	}

	// mark image as being in use
	imagesInFlight[imageIdx] = inFlightFences[currentFrame];

	// update UBOs
	updateUniformBuffer(imageIdx);

	// submit info
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[]{ imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// add command buffers
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIdx];

	VkSemaphore signalSemaphores[]{ renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// reset the fences
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	// submit to queue
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	// present to swapchain
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	// swapchains to present to
	VkSwapchainKHR swapchains[]{ swapChain };

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	// present queue
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapchain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain images!");
	}

	// get current frame
	currentFrame = (currentFrame + 1) % constants::maxFramesInFlight;
}


void Game::main() {
	// keep pulling from event queue until window is closed
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}

	// drawing calls are asynchronous, so we wait
	// before cleaning up
	vkDeviceWaitIdle(device);
}


void Game::cleanupSwapchain() {
	// destroy framebuffers
	for (size_t i{ 0 }; i < swapchainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device, swapchainFramebuffers[i], nullptr);
	}

	// free all command buffers
	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	// destroy pipeline
	vkDestroyPipeline(device, graphicsPipeline, nullptr);

	// destroy pipeline layout
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	// destroy render pass
	vkDestroyRenderPass(device, renderPass, nullptr);

	// destroy image views
	for (size_t i{ 0 }; i < swapchainImageViews.size(); i++) {
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}

	// destroys the swap chain
	vkDestroySwapchainKHR(device, swapChain, nullptr);

	// destroy uniform buffers
	for (size_t i = 0; i < swapchainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
}


void Game::cleanup() {
	cleanupSwapchain();

	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);

	// destroy index buffer
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	// destroy vertex buffer
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	// destroy semaphores
	for (size_t i{ 0 }; i < constants::maxFramesInFlight; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	// destroys the command pool
	vkDestroyCommandPool(device, commandPool, nullptr);

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
