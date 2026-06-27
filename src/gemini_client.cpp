#include "gemini_client.hpp"
#include <dpp/dpp.h>
#include <future>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

namespace ragc {

GeminiClient::GeminiClient(dpp::cluster &bot, std::string_view api_key)
    : bot_(bot), api_key_(api_key) {}

nlohmann::json GeminiClient::parse_expense(std::string_view user_input) {
  const std::string url =
      "https://generativelanguage.googleapis.com"
      "/v1beta/models/gemini-2.5-flash-lite:generateContent?key=" +
      api_key_;

  std::string body = build_request_body(user_input);

  int retries = 0;
  const int max_retries = 3;
  
  while (true) {
    std::promise<dpp::http_request_completion_t> promise;
    auto future = promise.get_future();

    bot_.request(url, dpp::m_post, [&promise](const dpp::http_request_completion_t &res) {
      promise.set_value(res);
    }, body, "application/json");

    auto response = future.get(); // Block until the HTTP response arrives

    if (response.status == 200) {
      auto json_resp = nlohmann::json::parse(response.body);
      try {
        std::string result_text =
            json_resp["candidates"][0]["content"]["parts"][0]["text"];
        return nlohmann::json::parse(result_text);
      } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse Gemini semantic JSON: " +
                                 std::string(e.what()));
      }
    }

    // Handle Rate Limiting (429)
    if (response.status == 429 && retries < max_retries) {
      retries++;
      int wait_seconds = 2 * retries * retries; // Simple backoff: 2s, 8s, 18s...
      
      // Try to parse recommended delay if available
      try {
        auto error_json = nlohmann::json::parse(response.body);
        if (error_json.contains("error") && error_json["error"].contains("details")) {
            for (auto& detail : error_json["error"]["details"]) {
                if (detail.at("@type") == "type.googleapis.com/google.rpc.RetryInfo") {
                    std::string delay_str = detail.at("retryDelay").get<std::string>();
                    // delay_str is like "50s"
                    if (delay_str.back() == 's') {
                        wait_seconds = std::stoi(delay_str.substr(0, delay_str.size()-1));
                    }
                }
            }
        }
      } catch (...) {}

      std::cout << "[GEMINI] Rate limited (429). Retrying in " << wait_seconds << "s... (Attempt " << retries << "/" << max_retries << ")" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));
      continue;
    }

    // Other errors
    throw std::runtime_error("Gemini API Error: Status " +
                             std::to_string(response.status) + " - " +
                             response.body);
  }
}

std::string GeminiClient::build_request_body(std::string_view user_input) const {
  nlohmann::json body;

  body["system_instruction"]["parts"][0]["text"] =
      "You are a financial parsing assistant. Extract structured expense data "
      "from informal text. Fields: amount (number), currency (string, ISO "
      "code), category (string, e.g., Food, Travel, Misc), description "
      "(string). Output ONLY raw JSON with no additional explanation.";

  body["contents"][0]["parts"][0]["text"] = user_input;
  body["generationConfig"]["response_mime_type"] = "application/json";

  return body.dump();
}

} // namespace ragc
