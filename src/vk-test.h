
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <config.h>
#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan's depth range from 0 to 1
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;
  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
  }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class VulkanTest {
 public:
  VulkanTest() {};
  ~VulkanTest() {};

  void     init();
  void     cleanup();
  void     run();

  bool             framebufferResized = false;

 private:

  /* Functions to prepare rendering */
  void     initWindow();
  void     createInstance();
  void     createDevice();
  void     createSurface();
  void     getQueue();
  void     createCommandBuffers();
  void     createCommandPool();
  void     createPipelineLayout();
  void     createSwapchain();
  void     recreateSwapchain();
  void     destroySwapchain();
  void     createSwapchainImageViews();
  void     createPipeline();
  void     createRenderPass();
  void     createFramebuffer();
  void     createSyncObjects();
  void     loadModel();
  void     createVertexBuffer();
  void     createIndexBuffer();
  void     createUniformBuffer();
  void     createTextureImage();
  void     createTextureImageView();
  void     createTextureSampler();
  void     createDepthResources();
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
  VkCommandBuffer  beginCommandBuffer();
  void             endCommandBufferAndSubmit(VkCommandBuffer commandBuffer);
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  bool     hasStencilComponent(VkFormat format);

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

  std::vector<VkSemaphore>      imageAvailableSemaphore;
  std::vector<VkSemaphore>      renderFinishedSemaphore;
  std::vector<VkFence> inFlightFences;
  size_t           currentFrame = 0;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VkBuffer         vertexBuffer;
  VkDeviceMemory   vertexBufferMemory;
  VkBuffer         indexBuffer;
  VkDeviceMemory   indexBufferMemory;
  VkBuffer         uniformBuffer;
  VkDeviceMemory   uniformBufferMemory;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet  descriptorSet;

  uint32_t         mipLevels;
  VkImage          textureImage;
  VkDeviceMemory   textureImageMemory;
  VkImageView      textureImageView;
  VkSampler        textureSampler;

  VkImage          depthImage;
  VkDeviceMemory   depthImageMemory;
  VkImageView      depthImageView;
};