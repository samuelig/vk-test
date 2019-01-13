#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <unordered_map>

// For rotating MVP matrices
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include "vk-test.h"
#include "vk-util.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "src/models/chalet.obj";
const std::string TEXTURE_PATH = "src/textures/chalet.jpg";

std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
  auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
  if (func != VK_NULL_HANDLE) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  if (func != VK_NULL_HANDLE) {
    func(instance, callback, pAllocator);
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
  std::cerr << "validation layer: " << msg << std::endl;

  return VK_FALSE;
}

bool VulkanTest::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
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

void VulkanTest::setupDebugCallback()
{
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  if (CreateDebugReportCallbackEXT(instance, &createInfo, VK_NULL_HANDLE, &callback) != VK_SUCCESS) {
    throw std::runtime_error("Error setting up debug callback");
  }
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanTest*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
}

void VulkanTest::initWindow()
{
  glfwInit();

  /* No need to create a context because I am not going to share it with OpenGL */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  /* Window is resizable */
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanTest::createInstance()
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

  if (ENABLE_DEBUG)
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  else
    validationLayers.clear();

  if (ENABLE_DEBUG && !checkValidationLayerSupport())
    throw std::runtime_error("No validation layers");

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = VK_NULL_HANDLE;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &applicationInfo;
  createInfo.enabledLayerCount = (unsigned)validationLayers.size();
  createInfo.ppEnabledLayerNames = &validationLayers[0];
  createInfo.enabledExtensionCount = (unsigned)instanceExtensions.size();
  createInfo.ppEnabledExtensionNames = &instanceExtensions[0];

  /* Creating the instance */
  res = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating the instance");

  printf("Instance created\n");
}

void VulkanTest::createDevice()
{
  VkResult res = VK_SUCCESS;

  /* Enumerate physical devices */
  std::vector<VkPhysicalDevice> physicalDevices;
  unsigned count = 0;

  res = vkEnumeratePhysicalDevices(instance, &count, VK_NULL_HANDLE);
  if (res != VK_SUCCESS)
     throw std::runtime_error("Error enumerating devices");

  physicalDevices.resize(count);

  res = vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());
  if (res != VK_SUCCESS)
    throw std::runtime_error( "Error enumerating devices");

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

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(phyDevice, &supportedFeatures);

  /* XXX: If it is not enabled, we can disable it in the sampler
   *  samplerInfo.anisotropyEnable = VK_FALSE;
   *  samplerInfo.maxAnisotropy = 1;
   */
  if (!supportedFeatures.samplerAnisotropy)
    throw std::runtime_error("Device doesn't support anisotropy sampling");


  /* Ask for queues properties, etc */
  vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &count, VK_NULL_HANDLE);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error getting device queue family properties");

  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  queueFamilyProperties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &count, queueFamilyProperties.data());

  /* Graphics queue */
  queueGraphicsFamilyIndex = -1;
  for (unsigned i = 0; i < count; i++) {
    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
	queueFamilyProperties[i].queueCount > 0)
      queueGraphicsFamilyIndex = i;
  }

  if (queueGraphicsFamilyIndex < 0)
    throw std::runtime_error("Device doesn't have a graphics queue useful for us");

  /* Presentation queue */
  queuePresentationFamilyIndex = -1;
  for (unsigned i = 0; i < count; i++) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, i, surface, &presentSupport);
    if (presentSupport && queueFamilyProperties[i].queueCount > 0)
      queuePresentationFamilyIndex = i;
  }

  if (queuePresentationFamilyIndex < 0)
    throw std::runtime_error("Device doesn't have a presentation queue useful for us");

  /* Create Logical Device */
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::vector<int> uniqueQueueFamilies;
  uniqueQueueFamilies.push_back(queueGraphicsFamilyIndex);
  if (queuePresentationFamilyIndex != queueGraphicsFamilyIndex)
    uniqueQueueFamilies.push_back(queuePresentationFamilyIndex);
  for (unsigned i = 0; i < uniqueQueueFamilies.size(); i++) {
    float queuePriorities [] = { 0 };
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = VK_NULL_HANDLE;
    queueCreateInfo.flags = 0;
    queueCreateInfo.queueFamilyIndex = (unsigned)uniqueQueueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriorities[0];
    queueCreateInfos.push_back(queueCreateInfo);
  }

  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pNext = VK_NULL_HANDLE;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.queueCreateInfoCount = (unsigned)queueCreateInfos.size();
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfos[0];
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = VK_NULL_HANDLE;
  deviceCreateInfo.enabledExtensionCount = (unsigned)deviceExtensions.size();
  deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions[0];
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  res = vkCreateDevice(phyDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating device");

  printf("Created logical device\n");
}

void VulkanTest::getQueue()
{
  /* Get queue */
  vkGetDeviceQueue(device, queueGraphicsFamilyIndex, 0, &graphicsQueue);
  vkGetDeviceQueue(device, queuePresentationFamilyIndex, 0, &presentQueue);
}

void VulkanTest::createSurface()
{
  VkResult res = VK_SUCCESS;
  /* Create Surface */
  res = glfwCreateWindowSurface(instance, window, NULL, &surface);

  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating window surface");
}

void VulkanTest::createCommandPool()
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
    throw std::runtime_error("Error creating command pool");
}

void VulkanTest::createCommandBuffers()
{
  VkResult res = VK_SUCCESS;
  /* Create command buffers */
  commandBuffers.resize(swapChainFramebuffers.size());

  VkCommandBufferAllocateInfo cmd = {};
  cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd.pNext = VK_NULL_HANDLE;
  cmd.commandPool = cmdPool;
  cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd.commandBufferCount = (unsigned) swapChainFramebuffers.size();

  res = vkAllocateCommandBuffers(device, &cmd, commandBuffers.data());
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating command buffer");

  printf("Created command buffers\n");
}

void VulkanTest::createPipelineLayout()
{
  /* Create Descriptor Set Layout */
  VkResult res = VK_SUCCESS;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;
  samplerLayoutBinding.descriptorCount = 1;


  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  /* Uniform buffer only used in vertex shader */
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;
  uboLayoutBinding.descriptorCount = 1;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings  = {uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.pNext = VK_NULL_HANDLE;
  descriptorSetLayoutInfo.flags = 0;
  descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());;
  descriptorSetLayoutInfo.pBindings = bindings.data();

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

void VulkanTest::recreateSwapchain()
{
  vkDeviceWaitIdle(device);

  destroySwapchain();
  createSwapchain();
  createSwapchainImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffer();
  createPipeline();
  createCommandBuffers();
  recordCommandBuffers();
}

void VulkanTest::destroySwapchain()
{
  vkFreeCommandBuffers(device, cmdPool, commandBuffers.size(), &commandBuffers[0]);
  vkDestroyPipeline(device, graphicsPipeline, VK_NULL_HANDLE);
  vkDestroyPipelineLayout(device, pipelineLayout, VK_NULL_HANDLE);
  vkDestroyDescriptorSetLayout(device, setLayout, VK_NULL_HANDLE);

  vkDestroyRenderPass(device, renderPass, VK_NULL_HANDLE);

  for (unsigned i = 0; i < swapChainFramebuffers.size(); i++)
    vkDestroyFramebuffer(device, swapChainFramebuffers[i], VK_NULL_HANDLE);

  for (unsigned i = 0; i < swapChainImageViews.size(); i++)
    vkDestroyImageView(device, swapChainImageViews[i], VK_NULL_HANDLE);

  vkFreeMemory(device, depthImageMemory, VK_NULL_HANDLE);
  vkDestroyImageView(device, depthImageView, VK_NULL_HANDLE);
  vkDestroyImage(device, depthImage, VK_NULL_HANDLE);

  vkDestroySwapchainKHR(device, swapChain, VK_NULL_HANDLE);
}

void VulkanTest::createSwapchain()
{
  unsigned width, height;

  glfwGetFramebufferSize(window, (int *)&width, (int *)&height);

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
    throw std::runtime_error("VK_FORMAT_B8G8R8A8_UNORM is not supported");

  /* This presentation mode is always supported according to the spec */
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

  /* Select the extend to current window size */
  swapChainExtent = {width, height};

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

  res = vkCreateSwapchainKHR(device, &createInfo, VK_NULL_HANDLE, &swapChain);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating swapchain");


  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, VK_NULL_HANDLE);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
}

void VulkanTest::createSwapchainImageViews()
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

VkShaderModule VulkanTest::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
	VkResult res = VK_SUCCESS;
        res = vkCreateShaderModule(device, &createInfo, VK_NULL_HANDLE, &shaderModule);
	if (res != VK_SUCCESS)
            throw std::runtime_error("Cannot create shader module");

        return shaderModule;
}

void VulkanTest::createPipeline()
{
  VkResult res = VK_SUCCESS;
  auto vertShaderCode = readFile("src/shaders/vert.spv");
  auto fragShaderCode = readFile("src/shaders/frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  /* Vertex input description */
  VkVertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, color);
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = (unsigned)attributeDescriptions.size();
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) swapChainExtent.width;
  viewport.height = (float) swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  /* Create pipeline layout, includes descriptor set layout definition too */
  createPipelineLayout();

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  //depthStencil.minDepthBounds = 0.0f; // Optional
  //depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  //depthStencil.front = {}; // Optional
  //depthStencil.back = {}; // Optional

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &graphicsPipeline);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error when creating graphics pipeline");

  vkDestroyShaderModule(device, fragShaderModule, VK_NULL_HANDLE);
  vkDestroyShaderModule(device, vertShaderModule, VK_NULL_HANDLE);

  printf("Created pipeline\n");
}

void VulkanTest::createRenderPass()
{
  VkResult res = VK_SUCCESS;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  res = vkCreateRenderPass(device, &renderPassInfo, VK_NULL_HANDLE, &renderPass);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating render pass");

  printf("Render pass created\n");
}

void VulkanTest::createFramebuffer()
{
  VkResult res = VK_SUCCESS;
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (unsigned i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {
      swapChainImageViews[i],
      depthImageView
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    res = vkCreateFramebuffer(device, &framebufferInfo, VK_NULL_HANDLE, &swapChainFramebuffers[i]);
    if (res != VK_SUCCESS)
      throw std::runtime_error("Error creating framebuffer");
  }
}

void VulkanTest::createSyncObjects()
{
  imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);


  /* Create semaphores to know when an swapchain image is ready and when the rendering has finished */
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, VK_NULL_HANDLE, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, VK_NULL_HANDLE, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, VK_NULL_HANDLE, &inFlightFences[i]) != VK_SUCCESS)
      throw std::runtime_error("Error creating semaphores for a frame");
  }
  printf("Created the semaphores used for getting swapchain images and know when it finished rendering\n");
}

void VulkanTest::drawFrame()
{
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

  VkResult res = VK_SUCCESS;
  /* Acquire next image to draw into */
  uint32_t imageIndex;
  res = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniformBuffer();

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore[currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  /* Submit work to the queue */
  res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error submitting draw command buffer");

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  res = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapchain();
  } else if (res != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanTest::recordCommandBuffers()
{
  VkResult res = VK_SUCCESS;
  for (unsigned i = 0; i < swapChainFramebuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(commandBuffers[i], &beginInfo);


    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffers[i]);

    res = vkEndCommandBuffer(commandBuffers[i]);
    if (res != VK_SUCCESS)
      throw std::runtime_error("Error recording command buffer");
  }

  printf("Recorded command buffer commands\n");
}

void VulkanTest::createBuffer(VkDeviceSize bufferSize, unsigned bufferUsage, unsigned memoryProperties,
                              VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
  VkResult res = VK_SUCCESS;
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = bufferUsage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  res = vkCreateBuffer(device, &bufferInfo, VK_NULL_HANDLE, &buffer);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating vertex buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

  res = vkAllocateMemory(device, &allocInfo, VK_NULL_HANDLE, &bufferMemory);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error allocating vertex buffer memory");

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanTest::fillBuffer(VkDeviceMemory bufferMemory, VkDeviceSize size, const void *data)
{
  void* dataBuffer;
  vkMapMemory(device, bufferMemory, 0, size, 0, &dataBuffer);
  memcpy(dataBuffer, data, (size_t) size);
  vkUnmapMemory(device, bufferMemory);
}

void VulkanTest::loadModel()
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex = {};
      vertex.pos = {
        attrib.vertices[3 * index.vertex_index + 0],
        attrib.vertices[3 * index.vertex_index + 1],
        attrib.vertices[3 * index.vertex_index + 2]
      };

      vertex.texCoord = {
        attrib.texcoords[2 * index.texcoord_index + 0],
        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
      };

      vertex.color = {1.0f, 1.0f, 1.0f};
      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
}

void VulkanTest::createVertexBuffer()
{
  VkResult res = VK_SUCCESS;
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vertexBuffer, vertexBufferMemory);

  fillBuffer(vertexBufferMemory, bufferSize, vertices.data());
  printf("Created Vertex input buffer and filled it with data\n");
}

void VulkanTest::createIndexBuffer()
{
  VkResult res = VK_SUCCESS;
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  /* For testing, I am going to use a staging buffer, even when for Intel GPU
   * it is not needed (the memory is shared between host and device). However
   * I am following https://vulkan-tutorial.com/Vertex_buffers/Index_buffer
   * tutorial and I want to learn how to do it for other GPUs.
   */
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  fillBuffer(stagingBufferMemory, bufferSize, indices.data());

  printf("Created staging buffer for indices and filled it with data\n");

  /* Create index buffer in device memory */
  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               indexBuffer, indexBufferMemory);

  printf("Created index buffer device local memory\n");

  /* Copy staging buffer contents (host visible memory to index buffer (device memory) */
  VkCommandBuffer commandBuffer = beginCommandBuffer();

  VkBufferCopy copyRegion = {};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &copyRegion);

  endCommandBufferAndSubmit(commandBuffer);

  printf("Transfer data from staging buffer to index buffer done\n");

  /* Destroy staging buffer, it is no longer used */
  vkDestroyBuffer(device, stagingBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, stagingBufferMemory, VK_NULL_HANDLE);
}

void VulkanTest::endCommandBufferAndSubmit(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);
  vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

VkCommandBuffer VulkanTest::beginCommandBuffer()
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = cmdPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanTest::createUniformBuffer()
{
  VkResult res = VK_SUCCESS;
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  createBuffer(bufferSize,
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               uniformBuffer, uniformBufferMemory);
  printf("Created Uniform buffer\n");
}

void VulkanTest::updateUniformBuffer()
{
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
  UniformBufferObject ubo = {};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
  fillBuffer(uniformBufferMemory, sizeof(ubo), &ubo);
}

void VulkanTest::createDescriptorPool()
{
  VkResult res = VK_SUCCESS;

  std::array<VkDescriptorPoolSize, 2> poolSizes = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = 1;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 1;

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;

  res = vkCreateDescriptorPool(device, &poolInfo, VK_NULL_HANDLE, &descriptorPool);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating descriptor pool");

  printf("Created descriptor pool\n");
}

void VulkanTest::createDescriptorSet()
{
  VkResult res = VK_SUCCESS;

  VkDescriptorSetLayout layouts[] = {setLayout};
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = layouts;

  res = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error allocating descriptor set");

  /* Once allocated, indicate it has a uniform buffer and bind it to the one
   * we created before.
   */
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = uniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UniformBufferObject);

  /* Describe the texture sampler we use and bind it */
  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = textureImageView;
  imageInfo.sampler = textureSampler;

  std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = descriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = descriptorSet;
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, VK_NULL_HANDLE);
  printf("Created descriptor set\n");
}

uint32_t VulkanTest::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(phyDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  }

  throw std::runtime_error("Error finding suitable memory type");
}

void VulkanTest::createTextureImage()
{
  VkResult res = VK_SUCCESS;
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

  if (!pixels)
    throw std::runtime_error("Error loading texture image");

  /* Upload the read data into a staging buffer, we will copy it to a image later. */
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);
  fillBuffer(stagingBufferMemory, imageSize, pixels);
  stbi_image_free(pixels);

  printf("Created buffer to copy the read pixels from texture.jpg to\n");

  /* Create the image to copy the data to */
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(texWidth);
  imageInfo.extent.height = static_cast<uint32_t>(texHeight);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional

  res = vkCreateImage(device, &imageInfo, VK_NULL_HANDLE, &textureImage);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating image");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  res = vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error allocating image memory!");

  vkBindImageMemory(device, textureImage, textureImageMemory, 0);

  printf("Created image\n");

  /* Copy the data */
  VkCommandBufferAllocateInfo allocCmdInfo = {};
  allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocCmdInfo.commandPool = cmdPool;
  allocCmdInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = beginCommandBuffer();

  /* Change layout */
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = textureImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, VK_NULL_HANDLE,
    0, VK_NULL_HANDLE,
    1, &barrier);

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
    static_cast<uint32_t>(texWidth),
    static_cast<uint32_t>(texHeight),
    1
  };

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1, &region);

  // Generate mipmaps

  // Check if image format supports linear blitting
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(phyDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;
  for (uint32_t i = 1; i < mipLevels; i++) {
    /* Change layout to generate mipmaps */
    VkImageMemoryBarrier barrierMipmap = {};
    barrierMipmap.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierMipmap.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierMipmap.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierMipmap.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierMipmap.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierMipmap.image = textureImage;
    barrierMipmap.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrierMipmap.subresourceRange.baseMipLevel = i - 1;
    barrierMipmap.subresourceRange.levelCount = 1;
    barrierMipmap.subresourceRange.baseArrayLayer = 0;
    barrierMipmap.subresourceRange.layerCount = 1;
    barrierMipmap.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrierMipmap.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, VK_NULL_HANDLE,
                         0, VK_NULL_HANDLE,
                         1, &barrierMipmap);
    VkImageBlit blit = {};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);
    /* Change layout of i - 1 mipmap level */
    VkImageMemoryBarrier barrierPostMipmap = {};
    barrierPostMipmap.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierPostMipmap.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierPostMipmap.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrierPostMipmap.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierPostMipmap.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierPostMipmap.image = textureImage;
    barrierPostMipmap.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrierPostMipmap.subresourceRange.baseMipLevel = i - 1;
    barrierPostMipmap.subresourceRange.levelCount = 1;
    barrierPostMipmap.subresourceRange.baseArrayLayer = 0;
    barrierPostMipmap.subresourceRange.layerCount = 1;
    barrierPostMipmap.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrierPostMipmap.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, VK_NULL_HANDLE,
                         0, VK_NULL_HANDLE,
                         1, &barrierPostMipmap);
    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  /* Change layout of last mipmap level */
  VkImageMemoryBarrier barrierPostLastMipmap = {};
  barrierPostLastMipmap.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrierPostLastMipmap.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrierPostLastMipmap.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrierPostLastMipmap.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrierPostLastMipmap.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrierPostLastMipmap.image = textureImage;
  barrierPostLastMipmap.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrierPostLastMipmap.subresourceRange.baseMipLevel = mipLevels - 1;
  barrierPostLastMipmap.subresourceRange.levelCount = 1;
  barrierPostLastMipmap.subresourceRange.baseArrayLayer = 0;
  barrierPostLastMipmap.subresourceRange.layerCount = 1;
  barrierPostLastMipmap.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrierPostLastMipmap.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, VK_NULL_HANDLE,
                        0, VK_NULL_HANDLE,
                        1, &barrierPostLastMipmap);
  endCommandBufferAndSubmit(commandBuffer);

  vkDestroyBuffer(device, stagingBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, stagingBufferMemory, VK_NULL_HANDLE);

  printf("Copied the pixels in the buffer to the image\n");
}

void VulkanTest::createTextureImageView()
{
  VkResult res = VK_SUCCESS;

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = textureImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  res = vkCreateImageView(device, &viewInfo, VK_NULL_HANDLE, &textureImageView);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating texture image view");
}

void VulkanTest::createTextureSampler()
{
  VkResult res = VK_SUCCESS;
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(mipLevels);
  res = vkCreateSampler(device, &samplerInfo, VK_NULL_HANDLE, &textureSampler);
  if (res != VK_SUCCESS)
        throw std::runtime_error("Error creating texture sampler");

  printf("Created Texture Sampler\n");
}

void VulkanTest::createDepthResources()
{
  /* XXX: Don't use stencil format yet */
  VkResult res = VK_SUCCESS;
  VkFormat depthFormat = findDepthFormat();

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = swapChainExtent.width;
  imageInfo.extent.height = swapChainExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional

  res = vkCreateImage(device, &imageInfo, VK_NULL_HANDLE, &depthImage);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating image");

  printf("Created depth image\n");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, depthImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  res = vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error allocating image memory!");

  vkBindImageMemory(device, depthImage, depthImageMemory, 0);

  VkImageViewCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = depthImage;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = depthFormat;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  res = vkCreateImageView(device, &createInfo, VK_NULL_HANDLE, &depthImageView);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating depth image view");

  /* Change depth image layout */
  VkCommandBuffer commandBuffer = beginCommandBuffer();
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = depthImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    0,
    0, VK_NULL_HANDLE,
    0, VK_NULL_HANDLE,
    1, &barrier);
  endCommandBufferAndSubmit(commandBuffer);

  printf("Changed layout depth image\n");
}

VkFormat VulkanTest::findDepthFormat()
{
  return
    findSupportedFormat({VK_FORMAT_D32_SFLOAT,
          VK_FORMAT_D32_SFLOAT_S8_UINT,
          VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanTest::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanTest::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(phyDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("Error finding supported format!");
}

void VulkanTest::cleanup()
{
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphore[i], VK_NULL_HANDLE);
    vkDestroySemaphore(device, imageAvailableSemaphore[i], VK_NULL_HANDLE);
    vkDestroyFence(device, inFlightFences[i], VK_NULL_HANDLE);
  }

  vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);

  vkDestroySampler(device, textureSampler, VK_NULL_HANDLE);
  vkDestroyImageView(device, textureImageView, VK_NULL_HANDLE);
  vkDestroyImage(device, textureImage, VK_NULL_HANDLE);
  vkFreeMemory(device, textureImageMemory, VK_NULL_HANDLE);

  vkDestroyBuffer(device, uniformBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, uniformBufferMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(device, indexBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, indexBufferMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(device, vertexBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, vertexBufferMemory, VK_NULL_HANDLE);

  destroySwapchain();
  vkDestroyCommandPool(device, cmdPool, VK_NULL_HANDLE);

  vkDestroyDevice(device, VK_NULL_HANDLE);
  DestroyDebugReportCallbackEXT(instance, callback, VK_NULL_HANDLE);
  vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
  vkDestroyInstance(instance, VK_NULL_HANDLE);

  glfwDestroyWindow(window);
  glfwTerminate();
  printf("Cleaned up all\n");
}

void VulkanTest::init()
{
  initWindow();
  createInstance();
  if (ENABLE_DEBUG)
    setupDebugCallback();
  createSurface();
  createDevice();
  getQueue();
  createSwapchain();
  createSwapchainImageViews();
  createRenderPass();
  createCommandPool(); // Created here becase we will need to transition the layout of the depthImage
  createDepthResources();
  createFramebuffer();
  createPipeline();
  loadModel();
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffer();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();
  createDescriptorPool();
  createDescriptorSet();
  createCommandBuffers();
  recordCommandBuffers();
  createSyncObjects();
}

void VulkanTest::run()
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }

  /* Wait for device finishes what it is doing */
  vkDeviceWaitIdle(device);
}
