#include <stdio.h>
#include <stdexcept>

#include "vk-def.h"

void TestMain::initWindow()
{
  glfwInit();

  /* No need to create a context because I am not going to share it with OpenGL */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  /* Window is not resizable */
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

void TestMain::createInstance()
{
  VkResult res = VK_SUCCESS;
  VkApplicationInfo applicationInfo = {};
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pNext = VK_NULL_HANDLE;
  applicationInfo.pApplicationName = PACKAGE_STRING;
  applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInfo.pEngineName = VK_NULL_HANDLE;
  applicationInfo.engineVersion = 0;
  applicationInfo.apiVersion = VK_API_VERSION_1_0;

  unsigned requiredInstanceExtensionsCount = 0;
  const char **instanceExtensionsGLFW;
  instanceExtensionsGLFW = glfwGetRequiredInstanceExtensions(&requiredInstanceExtensionsCount);
  printf("Required instance extensions for GLFW: \n");
  for (int i = 0; i < requiredInstanceExtensionsCount; i++) {
    instanceExtensions.push_back(instanceExtensionsGLFW[i]);
    printf("\t%s\n", instanceExtensionsGLFW[i]);
  }

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = VK_NULL_HANDLE;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &applicationInfo;
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = VK_NULL_HANDLE;
  createInfo.enabledExtensionCount = (unsigned)instanceExtensions.size();
  createInfo.ppEnabledExtensionNames = &instanceExtensions[0];

  /* Creating the instance */
  res = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating the instance\n");

  printf("Instance created\n");
}

void TestMain::createDevice()
{
  VkResult res = VK_SUCCESS;

  /* Enumerate physical devices */
  std::vector<VkPhysicalDevice> physicalDevices;
  unsigned count = 0;

  res = vkEnumeratePhysicalDevices(instance, &count, VK_NULL_HANDLE);
  if (res != VK_SUCCESS)
     throw std::runtime_error("Error enumerating devices\n");

  physicalDevices.resize(count);

  res = vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());
  if (res != VK_SUCCESS)
    throw std::runtime_error( "Error enumerating devices\n");

#if 0
  /* Ask for device properties in order to choose one. */
  VkPhysicalDeviceProperties phyProperties;
  for (unsigned i = 0; i < count; i++) {
    vkGetPhysicalDeviceProperties(physicalDevices[i], &phyProperties);
    // XXX: Fill with code
  }

  /* Choose the best one */
  phyDevice = physicalDevices[0];
#else
  /* Select the first one */
  phyDevice = physicalDevices[0];
#endif
  /* Ask for queues properties, etc */

  vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &count, VK_NULL_HANDLE);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error getting device queue family properties\n");

  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  queueFamilyProperties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &count, queueFamilyProperties.data());

  queueGraphicsFamilyIndex = -1;
  for (unsigned i = 0; i < count; i++) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, i, surface, &presentSupport);
    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
	queueFamilyProperties[i].queueCount >= 1 && presentSupport)
      queueGraphicsFamilyIndex = i;
  }

  if (queueGraphicsFamilyIndex < 0)
    throw std::runtime_error("Device doesn't have a queue useful for us\n");

  /* Create Logical Device */
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.pNext = VK_NULL_HANDLE;
  queueCreateInfo.flags = 0;
  queueCreateInfo.queueFamilyIndex = (unsigned)queueGraphicsFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = VK_NULL_HANDLE;

  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pNext = VK_NULL_HANDLE;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = VK_NULL_HANDLE;
  deviceCreateInfo.enabledExtensionCount = (unsigned)deviceExtensions.size();
  deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions[0];
  deviceCreateInfo.pEnabledFeatures = VK_NULL_HANDLE;

  res = vkCreateDevice(phyDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating device\n");

  printf("Created logical device\n");
}

void TestMain::getQueue()
{
  /* Get queue */
  vkGetDeviceQueue(device, queueGraphicsFamilyIndex, 0, &queue);
}

void TestMain::createSurface()
{
  VkResult res = VK_SUCCESS;
  /* Create Surface */
  res = glfwCreateWindowSurface(instance, window, NULL, &surface);

  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
}

void TestMain::createCommandBuffer()
{
  VkResult res = VK_SUCCESS;
  /* Create command buffer pool */
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.pNext = VK_NULL_HANDLE;
  cmdPoolInfo.flags = 0;
  cmdPoolInfo.queueFamilyIndex = queueGraphicsFamilyIndex;

  res = vkCreateCommandPool(device, &cmdPoolInfo, VK_NULL_HANDLE, &cmdPool);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating command pool\n");

  /* Create command buffer */
  VkCommandBufferAllocateInfo cmd = {};
  cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd.pNext = VK_NULL_HANDLE;
  cmd.commandPool = cmdPool;
  cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd.commandBufferCount = 1;

  res = vkAllocateCommandBuffers(device, &cmd, &cmdBuffer);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating command buffer\n");

  printf("Created command buffer\n");
}

void TestMain::createPipelineLayout()
{
  /* Create Descriptor Set Layout */
  VkResult res = VK_SUCCESS;
  // VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
  // descriptorSetLayoutBinding.binding = 0;
  // descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // descriptorSetLayoutBinding.stageFlags = (VkShaderStageFlagBits)0;
  // descriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.pNext = VK_NULL_HANDLE;
  descriptorSetLayoutInfo.flags = 0;
  descriptorSetLayoutInfo.bindingCount = 0;
  descriptorSetLayoutInfo.pBindings = VK_NULL_HANDLE;

  res = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, VK_NULL_HANDLE, &setLayout);

  /* Create Pipeline Layout */
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pNext = VK_NULL_HANDLE;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = VK_NULL_HANDLE;

  vkCreatePipelineLayout(device, &pipelineLayoutInfo, VK_NULL_HANDLE, &pipelineLayout);

  printf("Created pipeline layout\n");
}

void TestMain::createSwapchain()
{
  VkResult res = VK_SUCCESS;
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;

  /* Ask for the surface capabilities in the device */
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &capabilities);

  printf("Surface capabilities:\n");
  printf("\tminImageCount: %d\n", capabilities.minImageCount);
  printf("\tmaxImageCount: %d\n", capabilities.maxImageCount);
  printf("\tcurrentExtent: %d x %d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
  printf("\tminImageExtent: %d x %d\n", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
  printf("\tmaxImageExtent: %d x %d\n", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
  printf("\tsupportedTransforms: %d\n", capabilities.supportedTransforms);
  printf("\tcurrentTransform: %d\n", capabilities.currentTransform);
  printf("\tsupportedCompositeAlpha: %d\n", capabilities.supportedCompositeAlpha);
  printf("\tsupportedUsageFlags: %d\n", capabilities.supportedUsageFlags);

  /* Get supported formats by the surface */
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, VK_NULL_HANDLE);

  if (formatCount != 0) {
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, formats.data());
  }

  VkSurfaceFormatKHR surfaceFormat;
  surfaceFormat.format = VK_FORMAT_UNDEFINED;

  for (unsigned i = 0; i < formatCount; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
      surfaceFormat = formats[i];
      break;
    }
  }

  /* XXX: Implement a way of selecting a different format if this fails */
  if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
    throw std::runtime_error("VK_FORMAT_B8G8R8A8_UNORM is not supported\n");

#if 0
  /* Get the presentation modes supported */
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, VK_NULL_HANDLE);

  if (presentModeCount != 0) {
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
  }
  /* XXX: Implement a way of selecting the presentation mode */
  VkPresentModeKHR presentMode = presentModes[0];
#else
  /* This presentation mode is always supported according to the spec */
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

  /* Select the extend to current window size */
  swapChainExtent = {WIDTH, HEIGHT};

  unsigned imageCount = (capabilities.minImageCount >= 2) ?
    capabilities.minImageCount : capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = swapChainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating swapchain");


  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
}

void TestMain::createImageViews()
{
  VkResult res = VK_SUCCESS;
  swapChainImageViews.resize(swapChainImages.size());

  for (unsigned i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    res = vkCreateImageView(device, &createInfo, VK_NULL_HANDLE, &swapChainImageViews[i]);
    if (res != VK_SUCCESS)
      throw std::runtime_error("Error creating image views");
  }
}

void TestMain::cleanup()
{
  for (unsigned i = 0; i < swapChainImageViews.size(); i++)
    vkDestroyImageView(device, swapChainImageViews[i], VK_NULL_HANDLE);

  vkDestroySwapchainKHR(device, swapChain, VK_NULL_HANDLE);
  vkDestroyPipelineLayout(device, pipelineLayout, VK_NULL_HANDLE);
  vkDestroyDescriptorSetLayout(device, setLayout, VK_NULL_HANDLE);
  vkDestroyCommandPool(device, cmdPool, VK_NULL_HANDLE);
  vkDestroyDevice(device, VK_NULL_HANDLE);
  vkDestroyInstance(instance, VK_NULL_HANDLE);

  glfwDestroyWindow(window);
  glfwTerminate();
  printf("Cleaned up all\n");
}

void TestMain::init()
{
  initWindow();
  createInstance();
  createSurface();
  createDevice();
  getQueue();
  createCommandBuffer();
  createPipelineLayout();
  createSwapchain();
  createImageViews();
}

void TestMain::run()
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}
