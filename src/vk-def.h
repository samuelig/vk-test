
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <config.h>
#include <vector>
#include <string>

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
  void     createImageViews();
  void     createPipeline();
  void     createRenderPass();
  void     createFramebuffer();

  VkShaderModule   createShaderModule(const std::vector<char>& code);

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
  VkPipeline            graphicsPipeline;
  VkRenderPass          renderPass;

  VkSwapchainKHR swapChain;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

};
