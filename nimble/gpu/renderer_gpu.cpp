// ============================================================================
// SDL3 GPU RENDERER IMPLEMENTATION
// ============================================================================

#include "renderer_gpu.h"
#include <algorithm> 


// ============================================================================
// Singleton
// ============================================================================

GPURenderer* GPURenderer::instance = nullptr;

GPURenderer::GPURenderer()  { instance = this; }
GPURenderer::~GPURenderer() { Shutdown(); instance = nullptr; }

GPURenderer* GPURenderer::Instance() {
    if (!instance) instance = new GPURenderer();
    return instance;
}
// ============================================================================
// Initialize
// ============================================================================

bool GPURenderer::Initialize(SDL_Window* window) {
    this->window = window;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true,    // enable validation / debug
        nullptr  // let SDL pick the best driver
    );
    if (!device) {
        SDL_Log("[GPU] SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("[GPU] SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyGPUDevice(device);
        device = nullptr;
        return false;
    }

    // SDL_SetGPUSwapchainComposition is now unified into SDL_SetGPUSwapchainParameters
    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // Pre-allocate geometry buffers.
    vertexBuffer = CreateBuffer(1024 * 1024, SDL_GPU_BUFFERUSAGE_VERTEX);
    indexBuffer  = CreateBuffer(512  * 1024, SDL_GPU_BUFFERUSAGE_INDEX);
    if (!vertexBuffer || !indexBuffer) {
        SDL_Log("[GPU] Failed to create geometry buffers");
        return false;
    }

    SDL_Log("[GPU] Initialized — backend: %s", SDL_GetGPUDeviceDriver(device));
    return true;
}

// ============================================================================
// Shutdown
// ============================================================================

void GPURenderer::Shutdown() {
    if (!device) return;

    SDL_WaitForGPUIdle(device);
    
    // SDL_UnclaimWindowForGPUDevice was renamed to SDL_ReleaseWindowFromGPUDevice
    SDL_ReleaseWindowFromGPUDevice(device, window);

    for (auto& [type, sh] : shaders) {
        if (sh.vertexShader)   SDL_ReleaseGPUShader(device, sh.vertexShader);
        if (sh.fragmentShader) SDL_ReleaseGPUShader(device, sh.fragmentShader);
        if (sh.pipeline)       SDL_ReleaseGPUGraphicsPipeline(device, sh.pipeline);
    }
    shaders.clear();

    for (auto* b : buffers) {
        if (b->buffer)         SDL_ReleaseGPUBuffer(device, b->buffer);
        if (b->transferBuffer) SDL_ReleaseGPUTransferBuffer(device, b->transferBuffer);
        delete b;
    }
    buffers.clear();

    for (auto* t : textures) {
        if (t->texture) SDL_ReleaseGPUTexture(device, t->texture);
        if (t->sampler) SDL_ReleaseGPUSampler(device, t->sampler);
        delete t;
    }
    textures.clear();

    vertexBuffer = nullptr;
    indexBuffer  = nullptr;

    SDL_DestroyGPUDevice(device);
    device = nullptr;
    window = nullptr;

    SDL_Log("[GPU] Shutdown complete");
}

// ============================================================================
// Per-frame API
// ============================================================================

bool GPURenderer::AcquireCommandBuffer() {
    if (!device) return false;
    commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer) {
        SDL_Log("[GPU] SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void GPURenderer::BeginFrame() {
    if (!device || frameActive) return;

    if (!commandBuffer) {
        SDL_Log("[GPU] BeginFrame: no command buffer — call AcquireCommandBuffer() first");
        return;
    }

    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window,
                                        &swapchainTexture,
                                        &swapchainW, &swapchainH)) {
        SDL_Log("[GPU] SDL_AcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return;
    }

    if (!swapchainTexture) {
        return;
    }

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture     = swapchainTexture;
    colorTarget.load_op     = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op    = SDL_GPU_STOREOP_STORE;
    colorTarget.clear_color = clearColor;

    renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        SDL_Log("[GPU] SDL_BeginGPURenderPass failed: %s", SDL_GetError());
        return;
    }

    frameActive = true;
}

void GPURenderer::EndFrame() {
    if (!frameActive) return;

    SDL_EndGPURenderPass(renderPass);
    renderPass    = nullptr;
    currentShader = nullptr;

    SDL_SubmitGPUCommandBuffer(commandBuffer);
    commandBuffer = nullptr;

    frameActive = false;
}

void GPURenderer::Clear(float r, float g, float b, float a) {
    clearColor = { r, g, b, a };
}

// ============================================================================
// Shader loading
// ============================================================================

std::vector<char> GPURenderer::LoadShaderCode(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        SDL_Log("[GPU] Cannot open shader: %s", path.c_str());
        return {};
    }
    size_t size = (size_t)file.tellg();
    file.seekg(0);
    std::vector<char> buf(size);
    file.read(buf.data(), (std::streamsize)size);
    return buf;
}

SDL_GPUShader* GPURenderer::CompileShader(const std::vector<char>& code,
                                           SDL_GPUShaderStage       stage,
                                           Uint32 samplerCount,
                                           Uint32 uniformBufferCount) {
    if (code.empty()) return nullptr;

    SDL_GPUShaderCreateInfo info = {};
    info.code                = (const Uint8*)code.data();
    info.code_size           = code.size();
    info.entrypoint          = "main";
    info.format              = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage               = stage;
    info.num_samplers        = samplerCount;
    info.num_uniform_buffers = uniformBufferCount;

    SDL_GPUShader* sh = SDL_CreateGPUShader(device, &info);
    if (!sh) SDL_Log("[GPU] SDL_CreateGPUShader failed: %s", SDL_GetError());
    return sh;
}

bool GPURenderer::LoadShader(ShaderType type, const std::string& vertPath, const std::string& fragPath) {
    std::vector<char> vertCode = LoadShaderCode(vertPath);
    std::vector<char> fragCode = LoadShaderCode(fragPath);

    if (vertCode.empty()) {
        SDL_Log("[GPU] Failed to load vertex shader: %s", vertPath.c_str());
        return false;
    }
    if (fragCode.empty()) {
        SDL_Log("[GPU] Failed to load fragment shader: %s", fragPath.c_str());
        return false;
    }

    // Textured shaders need 1 sampler; SDF/Geometry shaders need 0.
    Uint32 samplers = (type == ShaderType::BasicQuad || type == ShaderType::Blur) ? 1 : 0;
    
    SDL_GPUShader* vert = CompileShader(vertCode, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    if (!vert) return false;

    SDL_GPUShader* frag = CompileShader(fragCode, SDL_GPU_SHADERSTAGE_FRAGMENT, samplers, 0);
    if (!frag) { 
        SDL_ReleaseGPUShader(device, vert); 
        return false; 
    }

    ShaderHandle handle;
    handle.vertexShader   = vert;
    handle.fragmentShader = frag;
    handle.type           = type;

    if (!BuildPipeline(handle)) {
        SDL_ReleaseGPUShader(device, vert);
        SDL_ReleaseGPUShader(device, frag);
        return false;
    }

    shaders[type] = handle;
    SDL_Log("[GPU] Loaded shader pipeline: %s & %s", vertPath.c_str(), fragPath.c_str());
    return true;
}

bool GPURenderer::BuildPipeline(ShaderHandle& handle) {
    SDL_GPUVertexAttribute attrs[] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, (Uint32)offsetof(GPUVertex, x) },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, (Uint32)offsetof(GPUVertex, u) },
        { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, (Uint32)offsetof(GPUVertex, r) },
    };
    SDL_GPUVertexBufferDescription vbDesc = {
        0, sizeof(GPUVertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0
    };
    SDL_GPUVertexInputState vertexInput = {};
    vertexInput.vertex_attributes          = attrs;
    vertexInput.num_vertex_attributes      = 3;
    vertexInput.vertex_buffer_descriptions = &vbDesc;
    vertexInput.num_vertex_buffers         = 1;

    SDL_GPUColorTargetDescription colorTarget = {};
    colorTarget.format = GetSwapchainFormat();
    colorTarget.blend_state.enable_blend          = true;
    colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pci = {};
    pci.vertex_shader                     = handle.vertexShader;
    pci.fragment_shader                   = handle.fragmentShader;
    pci.primitive_type                    = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.vertex_input_state                = vertexInput;
    pci.rasterizer_state.cull_mode        = SDL_GPU_CULLMODE_NONE;
    pci.multisample_state.sample_count    = SDL_GPU_SAMPLECOUNT_1;
    pci.depth_stencil_state.enable_depth_test  = false;
    pci.depth_stencil_state.enable_depth_write = false;
    pci.target_info.color_target_descriptions  = &colorTarget;
    pci.target_info.num_color_targets          = 1;

    handle.pipeline = SDL_CreateGPUGraphicsPipeline(device, &pci);
    if (!handle.pipeline) {
        SDL_Log("[GPU] SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        return false;
    }
    handle.isValid = true;
    return true;
}

SDL_GPUTextureFormat GPURenderer::GetSwapchainFormat() const {
    if (device && window)
        return SDL_GetGPUSwapchainTextureFormat(device, window);
    return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
}

ShaderHandle* GPURenderer::GetShader(ShaderType type) {
    auto it = shaders.find(type);
    return (it != shaders.end() && it->second.isValid) ? &it->second : nullptr;
}

void GPURenderer::BindShader(ShaderType type) {
    ShaderHandle* sh = GetShader(type);
    if (sh && renderPass) {
        SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
        currentShader = sh;
    }
}

// ============================================================================
// Buffer management
// ============================================================================

GPUBufferHandle* GPURenderer::CreateBuffer(size_t size, SDL_GPUBufferUsageFlags usage) {
    SDL_GPUBufferCreateInfo bci = {};
    bci.usage = usage;
    bci.size  = (Uint32)size;

    SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(device, &bci);
    if (!buf) {
        SDL_Log("[GPU] SDL_CreateGPUBuffer failed: %s", SDL_GetError());
        return nullptr;
    }

    auto* handle    = new GPUBufferHandle();
    handle->buffer  = buf;
    handle->size    = size;
    buffers.push_back(handle);
    return handle;
}

void GPURenderer::UploadBufferImmediate(GPUBufferHandle* dst, const void* data, size_t size) {
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = (Uint32)size;

    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!tb) { SDL_Log("[GPU] CreateGPUTransferBuffer failed"); return; }

    void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
    if (!mapped) { SDL_ReleaseGPUTransferBuffer(device, tb); return; }
    SDL_memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(device, tb);

    SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      cp = SDL_BeginGPUCopyPass(cb);

    SDL_GPUTransferBufferLocation src = { tb, 0 };
    SDL_GPUBufferRegion            dr  = { dst->buffer, 0, (Uint32)size };
    SDL_UploadToGPUBuffer(cp, &src, &dr, false);

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cb);
    SDL_ReleaseGPUTransferBuffer(device, tb);
}

void GPURenderer::UploadBuffer(GPUBufferHandle* handle, const void* data, size_t size) {
    if (!handle || !data || size == 0) return;
    UploadBufferImmediate(handle, data, size);
}

void GPURenderer::DestroyBuffer(GPUBufferHandle* handle) {
    if (!handle) return;
    if (handle->buffer)         SDL_ReleaseGPUBuffer(device, handle->buffer);
    if (handle->transferBuffer) SDL_ReleaseGPUTransferBuffer(device, handle->transferBuffer);
    buffers.erase(std::remove(buffers.begin(), buffers.end(), handle), buffers.end());
    delete handle;
}

// ============================================================================
// Texture management
// ============================================================================

GPUTextureHandle* GPURenderer::CreateTexture(int width, int height, SDL_GPUTextureFormat format) {
    SDL_GPUTextureCreateInfo tci = {};
    tci.format               = format;
    tci.width                = (Uint32)width;
    tci.height               = (Uint32)height;
    tci.layer_count_or_depth = 1;
    tci.num_levels           = 1;
    tci.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.sample_count         = SDL_GPU_SAMPLECOUNT_1;

    SDL_GPUTexture* tex = SDL_CreateGPUTexture(device, &tci);
    if (!tex) { SDL_Log("[GPU] CreateGPUTexture failed: %s", SDL_GetError()); return nullptr; }

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter     = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter     = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &sci);

    auto* handle    = new GPUTextureHandle();
    handle->texture = tex;
    handle->sampler = sampler;
    handle->width   = width;
    handle->height  = height;
    textures.push_back(handle);
    return handle;
}

void GPURenderer::UploadTexture(GPUTextureHandle* handle, const void* data, size_t dataSize) {
    if (!handle || !data || dataSize == 0) return;

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = (Uint32)dataSize;

    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!tb) return;

    void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
    if (!mapped) { SDL_ReleaseGPUTransferBuffer(device, tb); return; }
    SDL_memcpy(mapped, data, dataSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      cp = SDL_BeginGPUCopyPass(cb);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tb;
    src.offset          = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = handle->texture;
    dst.w       = (Uint32)handle->width;
    dst.h       = (Uint32)handle->height;
    dst.d       = 1;

    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cb);
    SDL_ReleaseGPUTransferBuffer(device, tb);
}

void GPURenderer::DestroyTexture(GPUTextureHandle* handle) {
    if (!handle) return;
    if (handle->texture) SDL_ReleaseGPUTexture(device, handle->texture);
    if (handle->sampler) SDL_ReleaseGPUSampler(device, handle->sampler);
    textures.erase(std::remove(textures.begin(), textures.end(), handle), textures.end());
    delete handle;
}

// ============================================================================
// Internal quad emitter
// ============================================================================

void GPURenderer::EmitQuad(float x, float y, float w, float h,
                            const float color[4],
                            float u0, float v0, float u1, float v1) {
    if (!renderPass || !vertexBuffer || !indexBuffer) return;

    auto toNDCX = [&](float px) { return (px / (float)windowWidth)  * 2.f - 1.f; };
    auto toNDCY = [&](float py) { return 1.f - (py / (float)windowHeight) * 2.f; };

    GPUVertex verts[4] = {
        { toNDCX(x),     toNDCY(y),     u0, v0, color[0], color[1], color[2], color[3] },
        { toNDCX(x + w), toNDCY(y),     u1, v0, color[0], color[1], color[2], color[3] },
        { toNDCX(x + w), toNDCY(y + h), u1, v1, color[0], color[1], color[2], color[3] },
        { toNDCX(x),     toNDCY(y + h), u0, v1, color[0], color[1], color[2], color[3] },
    };
    Uint16 indices[6] = { 0, 1, 2, 0, 2, 3 };

    UploadBuffer(vertexBuffer, verts,   sizeof(verts));
    UploadBuffer(indexBuffer,  indices, sizeof(indices));

    SDL_GPUBufferBinding vbBind = { vertexBuffer->buffer, 0 };
    SDL_GPUBufferBinding ibBind = { indexBuffer->buffer,  0 };
    SDL_BindGPUVertexBuffers(renderPass, 0, &vbBind, 1);
    SDL_BindGPUIndexBuffer(renderPass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
}

// ============================================================================
// Public draw calls
// ============================================================================

void GPURenderer::DrawTexturedQuad(GPUTextureHandle* texture,
                                    float x, float y, float w, float h) {
    ShaderHandle* sh = GetShader(ShaderType::BasicQuad);
    if (!sh || !renderPass || !texture) return;
    SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
    SDL_GPUTextureSamplerBinding tsb = { texture->texture, texture->sampler };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &tsb, 1);
    const float white[4] = { 1.f, 1.f, 1.f, 1.f };
    EmitQuad(x, y, w, h, white);
}

void GPURenderer::DrawRoundedBox(float x, float y, float w, float h,
                                  float radius, float feather, const float color[4]) {
    ShaderHandle* sh = GetShader(ShaderType::RoundedBox);
    if (!sh || !renderPass) return;
    SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
    EmitQuad(x, y, w, h, color);
}

void GPURenderer::DrawBoxShadow(float x, float y, float w, float h,
                                 float radius, float spread, float blur,
                                 const float color[4]) {
    ShaderHandle* sh = GetShader(ShaderType::BoxShadow);
    if (!sh || !renderPass) return;
    SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
    EmitQuad(x, y, w, h, color);
}

void GPURenderer::DrawBlur(float x, float y, float w, float h,
                            GPUTextureHandle* texture, float /*blurRadius*/) {
    ShaderHandle* sh = GetShader(ShaderType::Blur);
    if (!sh || !renderPass || !texture) return;
    SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
    SDL_GPUTextureSamplerBinding tsb = { texture->texture, texture->sampler };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &tsb, 1);
    const float white[4] = { 1.f, 1.f, 1.f, 1.f };
    EmitQuad(x, y, w, h, white);
}

void GPURenderer::DrawTriangle(float x, float y, float w, float h, const float color[4]) {
    ShaderHandle* sh = GetShader(ShaderType::Triangle);
    if (!sh || !renderPass) return;
    SDL_BindGPUGraphicsPipeline(renderPass, sh->pipeline);
    EmitQuad(x, y, w, h, color);
}

namespace GPUWidgets {
void RenderRoundedBox(float x, float y, float w, float h,
                      float radius, float feather, const float color[4]) {
    if (auto* gpu = GetGPURenderer())
        gpu->DrawRoundedBox(x, y, w, h, radius, feather, color);
}

void RenderBoxShadow(float x, float y, float w, float h,
                     float radius, float spread, float blur, const float color[4]) {
    if (auto* gpu = GetGPURenderer())
        gpu->DrawBoxShadow(x, y, w, h, radius, spread, blur, color);
}

void RenderBlur(float x, float y, float w, float h,
                GPUTextureHandle* texture, float blurRadius) {
    if (auto* gpu = GetGPURenderer())
        gpu->DrawBlur(x, y, w, h, texture, blurRadius);
}
} // namespace GPUWidgets