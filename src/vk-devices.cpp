#include <stdio.h>
#include <stdexcept>

#include "vk-def.h"

VkResult TestMain::init()
{
  VkResult res = VK_SUCCESS;

  initWindow();

  VkApplicationInfo applicationInfo = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO, // VkStructureType    sType;
    0,                                  // const void*        pNext;
    PACKAGE_STRING,                     // const char*        pApplicationName;
    VK_MAKE_VERSION(1, 0, 0),           // uint32_t           applicationVersion;
    VK_NULL_HANDLE,                     // const char*        pEngineName;
    0,                                  // uint32_t           engineVersion;
    VK_API_VERSION_1_0                  // uint32_t           apiVersion;
  };

  std::vector<const char*> instanceExtensions;

  unsigned requiredInstanceExtensionsCount = 0;
  const char **instanceExtensionsGLFW;
  instanceExtensionsGLFW = glfwGetRequiredInstanceExtensions(&requiredInstanceExtensionsCount);
  printf("Required instance extensions for GLFW: \n");
  for (int i = 0; i < requiredInstanceExtensionsCount; i++) {
    instanceExtensions.push_back(instanceExtensionsGLFW[i]);
    printf("\t%s\n", instanceExtensionsGLFW[i]);
  }

  VkInstanceCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType;
    VK_NULL_HANDLE,                         // const void*                pNext;
    0,                                      // VkInstanceCreateFlags      flags;
    &applicationInfo,                       // const VkApplicationInfo*   pApplicationInfo;
    0,                                      // uint32_t                   enabledLayerCount;
    VK_NULL_HANDLE,                         // const char* const*         ppEnabledLayerNames;
    (unsigned)instanceExtensions.size(),    // uint32_t                   enabledExtensionCount;
    &instanceExtensions[0]               // const char* const*         ppEnabledExtensionNames;
  };

  /* Creating the instance */
  res = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating the instance\n");

  printf("Instance created\n");

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

  /* Create Surface */
  createSurface();

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
  VkDeviceQueueCreateInfo queueCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // VkStructureType             sType;
    VK_NULL_HANDLE,                             // const void*                 pNext;
    0,                                          // VkDeviceQueueCreateFlags    flags;
    (unsigned)queueGraphicsFamilyIndex,         // uint32_t                    queueFamilyIndex;
    1,                                          // uint32_t                    queueCount;
    VK_NULL_HANDLE                              // const float*                pQueuePriorities;
  };

  std::vector<const char*> deviceExtensions;
  instanceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo deviceCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // VkStructureType                    sType;
    VK_NULL_HANDLE,                       // const void*                        pNext;
    0,                                    // VkDeviceCreateFlags                flags;
    1,                                    // uint32_t                           queueCreateInfoCount;
    &queueCreateInfo,                     // const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
    0,                                    // uint32_t                           enabledLayerCount;
    VK_NULL_HANDLE,                       // const char* const*                 ppEnabledLayerNames;
    (unsigned) deviceExtensions.size(),   // uint32_t                           enabledExtensionCount;
    &deviceExtensions[0],              // const char* const*                 ppEnabledExtensionNames;
    VK_NULL_HANDLE                        // const VkPhysicalDeviceFeatures*    pEnabledFeatures;
  };

  res = vkCreateDevice(phyDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
  if (res != VK_SUCCESS)
    throw std::runtime_error("Error creating device\n");

  printf("Created logical device\n");

  /* Get queue */
  vkGetDeviceQueue(device, queueGraphicsFamilyIndex, 0, &queue);

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

  /* Create Descriptor Set Layout */

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
  return res;
}

void TestMain::initWindow()
{
  glfwInit();

  /* No need to create a context because I am not going to share it with OpenGL */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  /* Window is not resizable */
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

void TestMain::createSurface()
{
  VkResult res = VK_SUCCESS;
  res = glfwCreateWindowSurface(instance, window, NULL, &surface);

  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
}

void TestMain::cleanup()
{
  vkDestroyPipelineLayout(device, pipelineLayout, VK_NULL_HANDLE);
  vkDestroyDescriptorSetLayout(device, setLayout, VK_NULL_HANDLE);
  vkDestroyCommandPool(device, cmdPool, VK_NULL_HANDLE);
  vkDestroyDevice(device, VK_NULL_HANDLE);
  vkDestroyInstance(instance, VK_NULL_HANDLE);

  glfwDestroyWindow(window);
  glfwTerminate();
}

void TestMain::run()
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}
