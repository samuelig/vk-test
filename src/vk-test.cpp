#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
// For rotating MVP matrices
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "vk-test.h"
#include "vk-util.h"

std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
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

void VulkanTest::initWindow()
{
  glfwInit();

  /* No need to create a context because I am not going to share it with OpenGL */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  /* Window is not resizable */
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
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
  uniqueQueueFamilies.push_back(queuePresentationFamilyIndex);
  for (unsigned i = 0; i < uniqueQueueFamilies.size(); i++) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = VK_NULL_HANDLE;
    queueCreateInfo.flags = 0;
    queueCreateInfo.queueFamilyIndex = (unsigned)uniqueQueueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = VK_NULL_HANDLE;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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
  deviceCreateInfo.pEnabledFeatures = VK_NULL_HANDLE;

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

void VulkanTest::createCommandBuffer()
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
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
  descriptorSetLayoutBinding.binding = 0;
  descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  /* Uniform buffer only used in vertex shader */
  descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;
  descriptorSetLayoutBinding.descriptorCount = 1;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.pNext = VK_NULL_HANDLE;
  descriptorSetLayoutInfo.flags = 0;
  descriptorSetLayoutInfo.bindingCount = 1;
  descriptorSetLayoutInfo.pBindings = &descriptorSetLayoutBinding;

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

void VulkanTest::createSwapchain()
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
    throw std::runtime_error("VK_FORMAT_B8G8R8A8_UNORM is not supported");

  /* This presentation mode is always supported according to the spec */
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

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

  res = vkCreateSwapchainKHR(device, &createInfo, VK_NULL_HANDLE, &swapChain);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating swapchain");


  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, VK_NULL_HANDLE);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
}

void VulkanTest::createImageViews()
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

  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, color);

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

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
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
    VkImageView attachments[] = {
      swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    res = vkCreateFramebuffer(device, &framebufferInfo, VK_NULL_HANDLE, &swapChainFramebuffers[i]);
    if (res != VK_SUCCESS)
      throw std::runtime_error("Error creating framebuffer");
  }
}

void VulkanTest::createSemaphores()
{
  /* Create semaphores to know when an swapchain image is ready and when the rendering has finished */
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (vkCreateSemaphore(device, &semaphoreInfo, VK_NULL_HANDLE, &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(device, &semaphoreInfo, VK_NULL_HANDLE, &renderFinishedSemaphore) != VK_SUCCESS)
    throw std::runtime_error("Error creating semaphores");

  printf("Created the semaphores used for getting swapchain images and know when it finished rendering\n");
}

void VulkanTest::drawFrame()
{
  VkResult res = VK_SUCCESS;
  /* Acquire next image to draw into */
  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  /* Submit work to the queue */
  res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
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

  vkQueuePresentKHR(presentQueue, &presentInfo);

  vkQueueWaitIdle(presentQueue);
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

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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

  /* For testing, I am going to use a staging buffer, even when for Intel GPU
   * it is not needed (the memory is shared between host and device). However
   * I am following https://vulkan-tutorial.com/Vertex_buffers/Index_buffer
   * tutorial and I want to learn how to do it for other GPUs.
   */
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

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

  VkBufferCopy copyRegion = {};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);
  vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);

  printf("Transfer data from staging buffer to index buffer done\n");

  /* Destroy staging buffer, it is no longer used */
  vkDestroyBuffer(device, stagingBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, stagingBufferMemory, VK_NULL_HANDLE);
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
  ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);

  fillBuffer(uniformBufferMemory, sizeof(ubo), &ubo);
}

void VulkanTest::createDescriptorPool()
{
  VkResult res = VK_SUCCESS;

  VkDescriptorPoolSize descriptorPoolSize = {};
  descriptorPoolSize.descriptorCount = 1;
  descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
  descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.flags = 0; // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  descriptorPoolCreateInfo.poolSizeCount = 1;
  descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
  descriptorPoolCreateInfo.maxSets = 1;

  res = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &descriptorPool);
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

  VkWriteDescriptorSet descriptorWrite = {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
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

void VulkanTest::cleanup()
{
  vkDestroySemaphore(device, renderFinishedSemaphore, VK_NULL_HANDLE);
  vkDestroySemaphore(device, imageAvailableSemaphore, VK_NULL_HANDLE);

  vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);

  vkDestroyBuffer(device, uniformBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, uniformBufferMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(device, indexBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, indexBufferMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(device, vertexBuffer, VK_NULL_HANDLE);
  vkFreeMemory(device, vertexBufferMemory, VK_NULL_HANDLE);

  vkDestroyPipeline(device, graphicsPipeline, VK_NULL_HANDLE);
  vkDestroyPipelineLayout(device, pipelineLayout, VK_NULL_HANDLE);

  vkDestroyDescriptorSetLayout(device, setLayout, VK_NULL_HANDLE);
  vkDestroyCommandPool(device, cmdPool, VK_NULL_HANDLE);

  for (unsigned i = 0; i < swapChainFramebuffers.size(); i++)
    vkDestroyFramebuffer(device, swapChainFramebuffers[i], VK_NULL_HANDLE);

  vkDestroyRenderPass(device, renderPass, VK_NULL_HANDLE);

  for (unsigned i = 0; i < swapChainImageViews.size(); i++)
    vkDestroyImageView(device, swapChainImageViews[i], VK_NULL_HANDLE);

  vkDestroySwapchainKHR(device, swapChain, VK_NULL_HANDLE);
  vkDestroyDevice(device, VK_NULL_HANDLE);
  DestroyDebugReportCallbackEXT(instance, callback, VK_NULL_HANDLE);
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
  createImageViews();
  createRenderPass();
  createFramebuffer();
  createCommandBuffer();
  createPipeline();
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffer();
  createDescriptorPool();
  createDescriptorSet();
  recordCommandBuffers();
  createSemaphores();
}

void VulkanTest::run()
{
  while (!glfwWindowShouldClose(window)) {
    updateUniformBuffer();
    drawFrame();
    glfwPollEvents();
  }

  /* Wait for device finishes what it is doing */
  vkDeviceWaitIdle(device);
}
