
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <config.h>
#include <vector>

class TestMain {
 public:
  TestMain() {};
  ~TestMain() {};

  VkResult init();

  VkInstance       instance;
  VkPhysicalDevice phyDevice;
  VkDevice         device;

  unsigned         queueFamilyIndex;
  VkQueue          queue;

};
