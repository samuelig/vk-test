
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

  /* Functions to prepare rendering */
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
  void     createIndexBuffer();
  void     createUniformBuffer();
  void     createTextureImage();
  void     createDescriptorPool();
  void     createDescriptorSet();
  void     recordCommandBuffers();
  void     updateUniformBuffer();
  void     drawFrame();

  /* Auxiliary functions */
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  void     setupDebugCallback();
  bool     checkValidationLayerSupport();
  void     createBuffer(VkDeviceSize bufferSize, unsigned bufferUsage, unsigned memoryProperties,
                        VkBuffer &buffer, VkDeviceMemory &bufferMemory);
  void     fillBuffer(VkDeviceMemory bufferMemory, VkDeviceSize size, const void *data);
  VkShaderModule   createShaderModule(const std::vector<char>& code);

  /* Class members */
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

  VkSemaphore      imageAvailableSemaphore;
  VkSemaphore      renderFinishedSemaphore;

  VkBuffer         vertexBuffer;
  VkDeviceMemory   vertexBufferMemory;
  VkBuffer         indexBuffer;
  VkDeviceMemory   indexBufferMemory;
  VkBuffer         uniformBuffer;
  VkDeviceMemory   uniformBufferMemory;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet  descriptorSet;

  VkImage          textureImage;
  VkDeviceMemory   textureImageMemory;
};
