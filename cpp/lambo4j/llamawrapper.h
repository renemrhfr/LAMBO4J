#pragma once
#include "llama.h"
#include <vector>
#include <string>
#include <functional>

class llamawrapper {
public:
    using TokenCallback = std::function<void(const std::string&)>;

    llamawrapper(const char* model_path, int n_ctx, int n_gpu_layers);
    ~llamawrapper();

    std::string generateResponse(const std::vector<llama_chat_message>& messages, TokenCallback callback = nullptr);

private:
    std::string generate(const std::string& prompt, TokenCallback callback);
    void cleanup();

    llama_model* model;
    llama_context* ctx;
    llama_sampler* smpl;
    const llama_vocab* vocab;
    std::vector<char> formatted;
};