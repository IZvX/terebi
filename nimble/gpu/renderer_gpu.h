#pragma once

// ============================================================================
// SDL3 GPU RENDERER
// ============================================================================
// Provides GPU-accelerated rendering using SDL3's GPU API (SPIR-V shaders).
//
// ARCHITECTURE NOTE:
//   This renderer owns the window's GPU device exclusively. It cannot coexist
//   with an SDL_Renderer on the same window — SDL_ClaimWindowForGPUDevice is
//   mutually exclusive with SDL_CreateRenderer.
//
// Integration path into main.cpp:
//   1. Remove SDL_CreateRenderer / SDL_Renderer and the SDLRenderer3 ImGui
//      backend; replace with ImGui_ImplSDLGPU3.
//   2. After SDL_CreateWindow, call:
//        GPURenderer::Instance()->Initialize(window);
//   3. Each frame:
//        gpu->AcquireCommandBuffer();
//        gpu->BeginFrame();          // begins render pass, clears
//        ... your draw calls ...
//        gpu->EndFrame();            // ends pass, submits CB → presents
//
//   See the "INTEGRATION GUIDE" comment block at the bottom of renderer_gpu.cpp
//   for a minimal main.cpp skeleton.
// ============================================================================

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>

// ============================================================================
// Vertex layout (must match the vertex shader attribute locations)
// ============================================================================

struct GPUVertex {
    float x, y;       // location 0 — position
    float u, v;       // location 1 — uv
    float r, g, b, a; // location 2 — color
};

// ============================================================================
// Push-constant structures (sizes must match shader layout(push_constant) blocks)
// ============================================================================

struct RoundedBoxParams {
    float sizeX, sizeY;
    float radius;
    float feather;
};

struct ShadowParams {
    float sizeX, sizeY;
    float radius;
    float spread;
    float blur;
};

struct BlurParams {
    float directionX, directionY;
    float radius;
    float resolutionX, resolutionY;
};

// ============================================================================
// Shader type tag
// ============================================================================

enum class ShaderType {
    BasicQuad,
    RoundedBox,
    BoxShadow,
    Blur,
    Triangle
};

// ============================================================================
// Opaque handles for GPU resources
// ============================================================================

struct ShaderHandle {
    SDL_GPUShader*           vertexShader   = nullptr;
    SDL_GPUShader*           fragmentShader = nullptr;
    SDL_GPUGraphicsPipeline* pipeline       = nullptr;
    ShaderType               type           = ShaderType::BasicQuad;
    bool                     isValid        = false;
};

struct GPUBufferHandle {
    SDL_GPUBuffer*         buffer         = nullptr;
    SDL_GPUTransferBuffer* transferBuffer = nullptr; // persistent staging buffer
    size_t                 size           = 0;
};

struct GPUTextureHandle {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    int width  = 0;
    int height = 0;
};

// ============================================================================
// GPURenderer
// ============================================================================

class GPURenderer {
public:
    GPURenderer();
    ~GPURenderer();

    // ----- Lifecycle --------------------------------------------------------
    bool Initialize(SDL_Window* window);
    void Shutdown();

    // ----- Per-frame API ----------------------------------------------------
    // Must be called in this order each frame:
    //   AcquireCommandBuffer → BeginFrame → [draws] → EndFrame
    bool AcquireCommandBuffer();
    void BeginFrame();
    void EndFrame();

    // ----- Shader management ---------------------------------// ----- Shader management ------------------------------------------------
    bool          LoadShader(ShaderType type, const std::string& vertSpirvPath, const std::string& fragSpirvPath);
     ShaderHandle* GetShader(ShaderType type);
     void          BindShader(ShaderType type);
 


    // ----- Buffer management ------------------------------------------------
    // usage should be SDL_GPU_BUFFERUSAGE_VERTEX or SDL_GPU_BUFFERUSAGE_INDEX.
    // Do NOT OR in GRAPHICS_STORAGE_READ — that causes validation errors on
    // some backends when the buffer is bound as a vertex/index source.
    GPUBufferHandle* CreateBuffer(size_t size, SDL_GPUBufferUsageFlags usage);
    void             UploadBuffer(GPUBufferHandle* handle, const void* data, size_t size);
    void             DestroyBuffer(GPUBufferHandle* handle);

    // ----- Texture management -----------------------------------------------
    GPUTextureHandle* CreateTexture(int width, int height, SDL_GPUTextureFormat format);
    void              UploadTexture(GPUTextureHandle* handle,
                                    const void* data, size_t dataSize);
    void              DestroyTexture(GPUTextureHandle* handle);

    // ----- Draw calls -------------------------------------------------------
    void Clear(float r, float g, float b, float a); // sets clear colour for next BeginFrame
    void DrawTexturedQuad(GPUTextureHandle* texture,
                          float x, float y, float w, float h);
    void DrawRoundedBox(float x, float y, float w, float h,
                        float radius, float feather, const float color[4]);
    void DrawBoxShadow(float x, float y, float w, float h,
                       float radius, float spread, float blur, const float color[4]);
    void DrawBlur(float x, float y, float w, float h,
                  GPUTextureHandle* texture, float blurRadius);
    void DrawTriangle(float x, float y, float w, float h, const float color[4]);

    // ----- Accessors --------------------------------------------------------
    SDL_GPUDevice*        GetDevice()        const { return device; }
    SDL_GPUCommandBuffer* GetCommandBuffer() const { return commandBuffer; }
    SDL_GPURenderPass*    GetRenderPass()    const { return renderPass; }
    int                   GetWindowWidth()   const { return windowWidth; }
    int                   GetWindowHeight()  const { return windowHeight; }
    bool                  IsFrameActive()    const { return frameActive; }

    // ----- Singleton --------------------------------------------------------
    static GPURenderer* Instance();

private:
    // Core objects
    SDL_GPUDevice*        device        = nullptr;
    SDL_Window*           window        = nullptr;
    SDL_GPUCommandBuffer* commandBuffer = nullptr;
    SDL_GPURenderPass*    renderPass    = nullptr;

    // Resource registries
    std::unordered_map<ShaderType, ShaderHandle> shaders;
    std::vector<GPUBufferHandle*>                buffers;
    std::vector<GPUTextureHandle*>               textures;

    // Persistent geometry buffers
    GPUBufferHandle* vertexBuffer = nullptr;
    GPUBufferHandle* indexBuffer  = nullptr;

    // Current pipeline
    ShaderHandle* currentShader = nullptr;

    // Swapchain texture acquired this frame
    SDL_GPUTexture* swapchainTexture = nullptr;
    Uint32          swapchainW       = 0;
    Uint32          swapchainH       = 0;

    // Clear colour (updated by Clear())
    SDL_FColor clearColor = { 0.12f, 0.12f, 0.14f, 1.0f };

    int  windowWidth  = 0;
    int  windowHeight = 0;
    bool frameActive  = false;

    static GPURenderer* instance;

    // ---- Private helpers ---------------------------------------------------

    // Returns the actual swapchain format from the driver (replaces hardcoded value).
    SDL_GPUTextureFormat GetSwapchainFormat() const;

    // Read a binary file (e.g. compiled .spv) into a byte vector.
    std::vector<char> LoadShaderCode(const std::string& path);

    // Create an SDL_GPUShader from raw SPIR-V bytes.
    // samplerCount / uniformBufferCount must match the shader's resource
    // declarations or validation layers will reject it.
    SDL_GPUShader* CompileShader(const std::vector<char>& code,
                                 SDL_GPUShaderStage       stage,
                                 Uint32 samplerCount       = 0,
                                 Uint32 uniformBufferCount = 0);

    // Build the graphics pipeline for a fully populated ShaderHandle.
    bool BuildPipeline(ShaderHandle& handle);

    // Upload arbitrary data to a GPU buffer using a temporary transfer buffer
    // (one-shot: creates, maps, copies, submits, waits, destroys).
    void UploadBufferImmediate(GPUBufferHandle* dst, const void* data, size_t size);

    // Emit a full-screen quad into the active render pass using the given
    // pixel-space rect. Handles vertex/index buffer population internally.
    void EmitQuad(float x, float y, float w, float h, const float color[4],
                  float u0 = 0.f, float v0 = 0.f,
                  float u1 = 1.f, float v1 = 1.f);
};

// ============================================================================
// Convenience free functions
// ============================================================================

inline GPURenderer* GetGPURenderer() { return GPURenderer::Instance(); }

inline bool IsGPUAvailable() {
    GPURenderer* gpu = GetGPURenderer();
    return gpu && gpu->GetDevice();
}

// ============================================================================
// Widget helpers
// ============================================================================

namespace GPUWidgets {

void RenderRoundedBox(float x, float y, float w, float h,
                      float radius, float feather, const float color[4]);

void RenderBoxShadow(float x, float y, float w, float h,
                     float radius, float spread, float blur, const float color[4]);

void RenderBlur(float x, float y, float w, float h,
                GPUTextureHandle* texture, float blurRadius);

} // namespace GPUWidgets