#include "fate/renderer.h"
#include "fate/core.h"
#include "fate/gamestate.h"
#include "fate/log.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <bx/allocator.h>
#include <fstream>
#include <bimg/decode.h>

namespace Fate {

  struct PosTexcoordVertex
  {
    float m_x;
    float m_y;
    float m_u;
    float m_v;

    static void init()
    {
      ms_decl
        .begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, true, true)
        .end();
    }

    static bgfx::VertexDecl ms_decl;
  };

  bgfx::VertexDecl PosTexcoordVertex::ms_decl;
  bgfx::VertexBufferHandle m_vbh;
  bgfx::IndexBufferHandle m_ibh;
  bgfx::UniformHandle s_texColor;
  bgfx::TextureHandle m_textureColor;
  bx::DefaultAllocator s_allocator;

  static PosTexcoordVertex s_cubeVertices[] =
    {
     {-0.5f, -0.5f, 1, 1 },
     {-0.5f, 0.5f, 1, 0 },
     {0.5f, 0.5f, 0, 0 },
     {0.5f, -0.5f, 0, 1 }
    };

  static const uint16_t s_cubeIndices[] =
    {
     0, 1, 2,
     2, 3, 0
    };

  static void imageReleaseCb(void* _ptr, void* _userData)
  {
    BX_UNUSED(_ptr);
    bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
    bimg::imageFree(imageContainer);
  }


  bgfx::TextureHandle loadTexture(std::string fileName) {
    std::ifstream ifs(fileName);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                         (std::istreambuf_iterator<char>()) );

    if(!ifs.is_open()) {
      LogMessage("Could not open texture. Probably crashing");
    }

    bimg::ImageContainer* imageContainer = bimg::imageParse(&s_allocator, (void*)content.c_str(), (uint32_t)content.length());

    const bgfx::Memory* mem = bgfx::makeRef(
                      imageContainer->m_data
                    , imageContainer->m_size
                    , imageReleaseCb
                    , imageContainer
                    );

    return bgfx::createTexture2D(
                      uint16_t(imageContainer->m_width)
                    , uint16_t(imageContainer->m_height)
                    , 1 < imageContainer->m_numMips
                    , imageContainer->m_numLayers
                    , bgfx::TextureFormat::Enum(imageContainer->m_format)
                    , 0
                    , mem
                    );
  };

  void Renderer::InitializeRenderer(WindowState &windowState, RenderState &renderState) {
    LogMessage("Initializing renderer...");

    bgfx::PlatformData pd;
    pd.ndt = GetDisplayType(windowState);
    pd.nwh = GetWindowHandle(windowState);

    bgfx::Init init;
    init.vendorId = BGFX_PCI_ID_NONE;
    init.deviceId = 0;
    init.platformData = pd;
    init.resolution.width  = windowState.width;
    init.resolution.height = windowState.height;

    bgfx::renderFrame();
    bgfx::init(init);

    shaderManager.LoadProgram("shaders/cubes.vshader.bin", "shaders/cubes.fshader.bin", 0);
    shaderManager.LoadProgram("shaders/sprite.vshader.bin", "shaders/sprite.fshader.bin", 1);

    PosTexcoordVertex::init();

    bgfx::reset(windowState.width, windowState.height, BGFX_RESET_VSYNC);
     // Enable debug text.
    bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);


    // Set view rectangle for 0th view
    bgfx::setViewRect(0, 0, 0, uint16_t(windowState.width), uint16_t(windowState.height));

    // Clear the view rect
    bgfx::setViewClear(0,
                       BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                       0x443355FF, 1.0f, 0);

    bgfx::touch(0);

    // Create static vertex buffer.
    m_vbh = bgfx::createVertexBuffer
      (
       bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) ),
       PosTexcoordVertex::ms_decl
       );

    m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) ) );

    s_texColor  = bgfx::createUniform("s_texColor",  bgfx::UniformType::Sampler);

    m_textureColor = loadTexture("textures/FATE.png");
  }

  void Renderer::Render(WindowState& windowState, RenderState &renderState) {
    const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };
    const bx::Vec3 eye = { 0.0f, 0.0f, 1.0f };

    // Set view and projection matrix for view 0.
    float view[16];
    bx::mtxLookAt(view, eye, at);

    float proj[16];
    bx::mtxProj(proj,
                60.0f,
                float(windowState.width)/float(windowState.height),
                0.1f, 100.0f,
                bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewTransform(0, view, proj);

    // Set view 0 default viewport.
    bgfx::setViewRect(0, 0, 0,
                      windowState.width,
                      windowState.height);

    bgfx::touch(0);

    float mtx[16];
    bx::mtxRotateY(mtx, 0.0f);

    // position x,y,z
    mtx[12] = 0.0f;
    mtx[13] = 0.0f;
    mtx[14] = 0.0f;

    // Set model matrix for rendering.
    bgfx::setTransform(mtx);

    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);
    bgfx::setTexture(0, s_texColor,  m_textureColor);

    bgfx::setState(BGFX_STATE_DEFAULT);

    shaderManager.UseProgram(1);

    bgfx::frame();
  }

  void Renderer::ShutdownRenderer(WindowState &windowState, RenderState &renderState) {
    LogMessage("Shutting down renderer");
    bgfx::destroy(m_ibh);
    bgfx::destroy(m_vbh);
    bgfx::destroy(m_textureColor);
    bgfx::destroy(s_texColor);
    bgfx::shutdown();
  }

  Renderer::~Renderer() {
  }
};
