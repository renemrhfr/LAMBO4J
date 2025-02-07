#include "llamawrapper.h"
#include <stdexcept>
#include <cstring>

std::string llamawrapper::generateResponse(const std::vector<llama_chat_message>& messages, TokenCallback callback) {
    const char* tmpl = llama_model_chat_template(model, nullptr);
    if (!tmpl) {
        throw std::runtime_error("Failed to get chat template");
    }

    int new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(),
                                            formatted.size());
    if (new_len > (int)formatted.size()) {
        formatted.resize(new_len);
        new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(),
                                            formatted.size());
    }
    if (new_len < 0) {
        throw std::runtime_error("Failed to apply chat template");
    }

    std::string prompt(formatted.data(), new_len);
    return generate(prompt, callback);
}

std::string llamawrapper::generate(const std::string& prompt, TokenCallback callback) {
    std::string response;
    const bool is_first = llama_get_kv_cache_used_cells(ctx) == 0;

    // Tokenize the prompt
    const int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token> prompt_tokens(n_prompt_tokens);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first,
                       true) < 0) {
        throw std::runtime_error("Failed to tokenize the prompt");
    }

    // Check context size
    int n_ctx = llama_n_ctx(ctx);
    int n_ctx_used = llama_get_kv_cache_used_cells(ctx);
    if (n_ctx_used + n_prompt_tokens > n_ctx) {
        throw std::runtime_error("Context size exceeded");
    }

    // Initial batch with prompt tokens
    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    while (true) {
        if (llama_decode(ctx, batch)) {
            throw std::runtime_error("Failed to decode");
        }

        // Sample next token
        llama_token new_token_id = llama_sampler_sample(smpl, ctx, -1);

        // Check for end of generation
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }

        // Convert token to text
        char buf[256];
        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            throw std::runtime_error("Failed to convert token to piece");
        }

        std::string piece(buf, n);
        if (callback) {
            callback(piece);
        }
        response += piece;

        // Prepare next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    return response;
}

llamawrapper::llamawrapper(const char* model_path, int n_ctx, int n_gpu_layers) :
        model(nullptr), ctx(nullptr), smpl(nullptr), vocab(nullptr) {
    try {
        // Initialize logging
        llama_log_set([](enum ggml_log_level level, const char* text, void*) {
            if (level >= GGML_LOG_LEVEL_ERROR) {
                fprintf(stderr, "%s", text);
            }
        }, nullptr);

        // Load backends
        ggml_backend_load_all();

        // Initialize model
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = n_gpu_layers;

        model = llama_model_load_from_file(model_path, model_params);
        if (!model) {
            throw std::runtime_error("Unable to load model");
        }

        vocab = llama_model_get_vocab(model);

        // Initialize context
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = n_ctx;
        ctx_params.n_batch = n_ctx;

        ctx = llama_init_from_model(model, ctx_params);
        if (!ctx) {
            throw std::runtime_error("Failed to create the llama_context");
        }

        // Initialize sampler - matching the example's parameters
        smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
        if (!smpl) {
            throw std::runtime_error("Failed to create sampler");
        }
        llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
        llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
        llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

        // Initialize formatted buffer
        formatted.resize(n_ctx);
    } catch (...) {
        cleanup();
        throw;
    }
}

void llamawrapper::cleanup() {
    if (smpl) {
        llama_sampler_free(smpl);
        smpl = nullptr;
    }
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_model_free(model);
        model = nullptr;
    }
    vocab = nullptr;  // This is owned by model, no need to free
}

llamawrapper::~llamawrapper() {
    cleanup();
}