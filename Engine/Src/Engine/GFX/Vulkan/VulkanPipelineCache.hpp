#pragma once
#include <memory>
#include <vector>

namespace Engine {

class Pipeline;
class Context;

struct CreatePipelineRequest {
  std::string VertexShaderFile;
  std::string FragmentShaderFile;
};

class PipelineCache {
public:
  explicit PipelineCache(Context& context);
  ~PipelineCache();

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;

  size_t CreatePipeline(const CreatePipelineRequest& request);
  Pipeline& GetPipeline(size_t pipelineID);

private:
  Context& context_;
  std::vector<std::unique_ptr<Pipeline>> pipelines_;
};

} // namespace Engine