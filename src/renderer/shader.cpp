#include "fate/shader.h"
#include "fate/log.h"
#include <bgfx/bgfx.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <fstream>

namespace Fate {

  class Program {
  public:
    std::string concatPath;
    bgfx::ProgramHandle programHandle;
  };

  bgfx::ShaderHandle GetShader(const std::string &fileName) {
    std::ifstream ifs(fileName);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                         (std::istreambuf_iterator<char>()    ) );

    const bgfx::Memory* mem = bgfx::copy(content.c_str(),content.length() + 1);
    mem->data[mem->size-1] = '\0';
    bgfx::ShaderHandle handle = bgfx::createShader(mem);
    return handle;
  }

  uint8_t ShaderManager::LoadProgram(const std::string &vertexPath, const std::string &fragmentPath) {

    auto concatPath = vertexPath + fragmentPath;

    for(auto const& [key, val] : programs) {
      if(val->concatPath == concatPath) {
        return key;
      }
    }

    auto vertexHandle = GetShader(vertexPath);
    auto fragmentHandle = GetShader(fragmentPath);

    keyVal = keyVal + 1;

    auto program = std::make_shared<Program>();
    program->concatPath = concatPath;
    program->programHandle = bgfx::createProgram(vertexHandle, fragmentHandle, true);

    programs.insert(std::make_pair(keyVal, std::move(program)));

    return keyVal;
  };
}
