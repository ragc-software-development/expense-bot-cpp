#include "config.hpp"

#include <cstdlib>
#include <stdexcept>

namespace ragc {

Config Config::load()
{
    const char* raw_token      = std::getenv("DISCORD_BOT_TOKEN");
    const char* raw_db_url     = std::getenv("DATABASE_URL");
    const char* raw_gemini_key = std::getenv("GEMINI_API_KEY");
    const char* raw_guild_id   = std::getenv("DISCORD_GUILD_ID");

    // OpenAI-compatible AI backend vars (required for OllamaClient)
    const char* raw_ai_url   = std::getenv("AI_API_URL");
    const char* raw_ai_key   = std::getenv("AI_API_KEY");
    const char* raw_ai_model = std::getenv("AI_MODEL_NAME");

    if (!raw_token) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN environment variable is missing.");
    }
    std::string token(raw_token);
    if (token.empty()) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN is empty.");
    }

    if (!raw_db_url) {
        throw std::runtime_error("Configuration error: DATABASE_URL environment variable is missing.");
    }
    std::string db_url(raw_db_url);
    if (db_url.empty() || (db_url.rfind("postgresql://", 0) != 0 && db_url.rfind("postgres://", 0) != 0)) {
        throw std::runtime_error("Configuration error: DATABASE_URL must start with 'postgresql://' or 'postgres://'.");
    }

    // ---------------------------------------------------------------------------
    // Optional legacy Gemini key — allowed to be empty when using OllamaClient
    // ---------------------------------------------------------------------------
    std::string gemini_key;
    if (raw_gemini_key) {
        gemini_key = raw_gemini_key;
    }

    // ---------------------------------------------------------------------------
    // OpenAI-compatible AI backend
    // ---------------------------------------------------------------------------
    if (!raw_ai_url || std::string(raw_ai_url).empty()) {
        throw std::runtime_error(
            "Configuration error: AI_API_URL environment variable is missing.");
    }
    if (!raw_ai_key || std::string(raw_ai_key).empty()) {
        throw std::runtime_error(
            "Configuration error: AI_API_KEY environment variable is missing.");
    }
    if (!raw_ai_model || std::string(raw_ai_model).empty()) {
        throw std::runtime_error(
            "Configuration error: AI_MODEL_NAME environment variable is missing.");
    }

    std::string ai_api_url(raw_ai_url);
    std::string ai_api_key(raw_ai_key);
    std::string ai_model_name(raw_ai_model);

    return {std::move(token),
            std::move(db_url),
            std::move(gemini_key),
            std::move(ai_api_url),
            std::move(ai_api_key),
            std::move(ai_model_name),
            std::move(guild_id)};
}

} // namespace ragc
