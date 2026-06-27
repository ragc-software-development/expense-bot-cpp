#include "processor.hpp"
#include <iostream>

namespace ragc {

Processor::Processor(DatabaseProcessor &db, GeminiClient &ai) : db_(db), ai_(ai) {}

std::optional<Expense> Processor::process_message(std::string_view raw_content,
                                                  int64_t user_id) {
  try {
    // 1. Send to Gemini for semantic parsing
    nlohmann::json extracted = ai_.parse_expense(raw_content);

    // Gemini may return a JSON array instead of a plain object — normalise it
    if (extracted.is_array()) {
      if (extracted.empty()) {
        throw std::runtime_error("Gemini returned an empty JSON array");
      }
      extracted = extracted[0];
    }

    // 2. Map JSON fields to Expense struct
    Expense expense{};
    expense.amount = extracted.value("amount", 0.0);
    expense.currency = extracted.value("currency", "USD");
    expense.category = extracted.value("category", "Misc");
    expense.description =
        extracted.value("description", std::string(raw_content));
    expense.user_id = user_id;

    // 3. Persist to PostgreSQL
    if (db_.save_expense(expense)) {
      return expense;
    }
    return std::nullopt;

  } catch (const std::exception &e) {
    std::cerr << "Processor Error for user " << user_id << ": " << e.what()
              << std::endl;
    return std::nullopt;
  }
}

} // namespace ragc
