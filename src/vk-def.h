
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

  void     init();
  void     cleanup();
  void     run();

 private:
  void     initWindow();
  void     createInstance();
  void     createDevice();
  void     createSurface();
  void     getQueue();
  void     createCommandBuffer();
  void     createPipelineLayout();
  void     createSwapchain();

  GLFWwindow       *window;
  VkSurfaceKHR     surface;

  VkInstance       instance;
  std::vector<const char*> instanceExtensions;
  std::vector<const char*> deviceExtensions;

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
