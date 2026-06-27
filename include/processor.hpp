#pragma once

#include "db_processor.hpp"
#include "gemini_client.hpp"
#include "models/expense.hpp"
#include <optional>

namespace ragc {

/**
 * @brief Coordinates between the LLM parser and the Database persistence.
 */
class Processor {
public:
    /**
     * @brief Construct a new Processor
     * 
     * @param db Reference to the database processor (Dependency Injection).
     * @param ai Reference to the Gemini AI client (Dependency Injection).
     */
    Processor(DatabaseProcessor& db, GeminiClient& ai);

    /**
     * @brief orchestrates the end-to-end task: AI extraction -> DB persistence.
     * 
     * @param raw_content The text sent by the user in Discord.
     * @param user_id The Discord User ID.
     * @return std::optional<Expense> The processed expense if successful, nullopt otherwise.
     */
    std::optional<Expense> process_message(std::string_view raw_content, int64_t user_id);

private:
    DatabaseProcessor& db_;
    GeminiClient& ai_;
};

} // namespace ragc
