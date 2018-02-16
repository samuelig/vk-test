
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <config.h>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;

class TestMain {
 public:
  TestMain() {};
  ~TestMain() {};

  VkResult init();
  void     cleanup();
  void     run();

 private:
  void     initWindow();
  void     createSurface();

  GLFWwindow       *window;
  VkSurfaceKHR     surface;

  VkInstance       instance;
  VkPhysicalDevice phyDevice;
  VkDevice         device;

  int              queueGraphicsFamilyIndex;
  VkQueue          queue;

  VkCommandPool    cmdPool;
  VkCommandBuffer  cmdBuffer;

  VkDescriptorSetLayout setLayout;
  VkPipelineLayout      pipelineLayout;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
};
