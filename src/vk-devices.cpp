#include <stdio.h>

#include "vk-def.h"

VkResult TestMain::init()
{
  VkResult res = VK_SUCCESS;

  VkApplicationInfo applicationInfo = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO, // VkStructureType    sType;
    0,                                  // const void*        pNext;
    PACKAGE_STRING,                     // const char*        pApplicationName;
    1,                                  // uint32_t           applicationVersion;
    VK_NULL_HANDLE,                     // const char*        pEngineName;
    0,                                  // uint32_t           engineVersion;
    VK_API_VERSION_1_0                  // uint32_t           apiVersion;
  };
  
  VkInstanceCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType            sType;
    VK_NULL_HANDLE,                         // const void*                pNext;
    0,                                      // VkInstanceCreateFlags      flags;
    &applicationInfo,                       // const VkApplicationInfo*   pApplicationInfo;
    0,                                      // uint32_t                   enabledLayerCount;
    VK_NULL_HANDLE,                         // const char* const*         ppEnabledLayerNames;
    0,                                      // uint32_t                   enabledExtensionCount;
    VK_NULL_HANDLE                          // const char* const*         ppEnabledExtensionNames;
  };
  
  /* Creating the instance */
  res = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Error creating the instance\n");
    return res;
  }

  /* Enumerate physical devices */
  std::vector<VkPhysicalDevice> physicalDevices;
  unsigned count = 0;

  res = vkEnumeratePhysicalDevices(instance, &count, VK_NULL_HANDLE);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Error enumerating devices\n");
    return res;
  }

  physicalDevices.resize(count);

  res = vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Error enumerating devices\n");
    return res;
  }

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
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Error getting device queue family properties\n");
    return res;
  }

  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  queueFamilyProperties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &count, queueFamilyProperties.data());

  for (queueFamilyIndex = 0; queueFamilyIndex < count; queueFamilyIndex++) {
    if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
	queueFamilyProperties[queueFamilyIndex].queueCount >= 1)
      break;
  }

  if (queueFamilyIndex == count) {
    printf("Device doesn't have a queue useful for us\n");
    return VK_ERROR_DEVICE_LOST;
  }
					   
  VkDeviceQueueCreateInfo queueCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // VkStructureType             sType;
    VK_NULL_HANDLE,                             // const void*                 pNext;
    0,                                          // VkDeviceQueueCreateFlags    flags;
    queueFamilyIndex,                           // uint32_t                    queueFamilyIndex;
    1,                                          // uint32_t                    queueCount;
    VK_NULL_HANDLE                              // const float*                pQueuePriorities;
  };

  VkDeviceCreateInfo deviceCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // VkStructureType                    sType;
    VK_NULL_HANDLE,                       // const void*                        pNext;
    0,                                    // VkDeviceCreateFlags                flags;
    1,                                    // uint32_t                           queueCreateInfoCount;
    &queueCreateInfo,                     // const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
    0,                                    // uint32_t                           enabledLayerCount;
    VK_NULL_HANDLE,                       // const char* const*                 ppEnabledLayerNames;
    0,                                    // uint32_t                           enabledExtensionCount;
    VK_NULL_HANDLE,                       // const char* const*                 ppEnabledExtensionNames;
    VK_NULL_HANDLE                        // const VkPhysicalDeviceFeatures*    pEnabledFeatures;
  };
  
  res = vkCreateDevice(phyDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Error creating device\n");
    return res;
  }

  // Get queue
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
  
  return res;
}
