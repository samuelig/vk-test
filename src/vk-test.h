
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <config.h>
#include <vector>
#include <string>

const int WIDTH = 800;
const int HEIGHT = 600;

class VulkanTest {
 public:
  VulkanTest() {};
  ~VulkanTest() {};

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
  void     createSemaphores();
  void     createVertexBuffer();

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  void     recordCommandBuffers();
  void     drawFrame();
  void     setupDebugCallback();
  bool     checkValidationLayerSupport();

  VkShaderModule   createShaderModule(const std::vector<char>& code);

  GLFWwindow       *window;
  VkSurfaceKHR     surface;

  VkInstance       instance;
  VkDebugReportCallbackEXT callback;
  std::vector<const char*> instanceExtensions;
  std::vector<const char*> deviceExtensions;

  VkPhysicalDevice phyDevice;
  VkDevice         device;

  int              queueGraphicsFamilyIndex;
  int              queuePresentationFamilyIndex;
  VkQueue          graphicsQueue;
  VkQueue          presentQueue;

  VkCommandPool                 cmdPool;
  std::vector<VkCommandBuffer>  commandBuffers;

  VkDescriptorSetLayout setLayout;
  VkPipelineLayout      pipelineLayout;
  VkPipeline            graphicsPipeline;
  VkRenderPass          renderPass;

  VkSwapchainKHR        swapChain;
  VkFormat              swapChainImageFormat;
  VkExtent2D            swapChainExtent;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkSemaphore    imageAvailableSemaphore;
  VkSemaphore    renderFinishedSemaphore;
  VkBuffer       vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
};