#include "db_processor.hpp"
#include "gemini_client.hpp"
#include "processor.hpp"
#include "replier.hpp"
#include "worker_pool.hpp"
#include <cstdlib>
#include <dpp/dpp.h>
#include <iostream>
#include <optional>

int main() {
  // 1. Configuration
  const char *env_token = std::getenv("DISCORD_BOT_TOKEN");
  const char *env_db_url = std::getenv("DATABASE_URL");
  const char *env_gemini_key = std::getenv("GEMINI_API_KEY");
  const char *env_guild_id = std::getenv("DISCORD_GUILD_ID"); // Optional

  if (!env_token || !env_db_url || !env_gemini_key) {
    std::cerr << "CRITICAL ERROR: Missing environment variables!" << std::endl;
    std::cerr << "Ensure DISCORD_BOT_TOKEN, DATABASE_URL, and GEMINI_API_KEY are set." << std::endl;
    return 1;
  }

  constexpr std::size_t POOL_SIZE = 4;

  try {
    // 2. Initialize Infrastructure
    ragc::DatabaseProcessor db(env_db_url, POOL_SIZE);
    db.init_tables();

    // 3. Initialize D++ Bot Cluster
    dpp::cluster bot(env_token);

    ragc::GeminiClient ai(bot, env_gemini_key);
    ragc::Processor processor(db, ai);
    ragc::WorkerPool workers(POOL_SIZE);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&processor, &workers](const dpp::slashcommand_t &event) {
      const std::string cmd = event.command.get_command_name();
      
      if (cmd == "ping") {
        event.reply("Pong! ExpenseBot is operational.");
      }

      if (cmd == "expense") {
        std::string input = std::get<std::string>(event.get_parameter("query"));
        int64_t user_id = event.command.usr.id;

        event.thinking(false, [event, input, user_id, &processor, &workers](const dpp::confirmation_callback_t& auth) {
            workers.enqueue([event, input, user_id, &processor]() {
                auto result = processor.process_message(input, user_id);
                
                if (result.has_value()) {
                    dpp::message msg(event.command.channel_id, ragc::Replier::create_expense_embed(result.value()));
                    event.edit_response(msg);
                } else {
                    event.edit_response("❌ Failed to parse or save expense. Please try again with more detail.");
                }
            });
        });
      }
    });

    bot.on_ready([&bot, env_guild_id](const dpp::ready_t &event) {
      if (dpp::run_once<struct register_bot_commands>()) {
        dpp::slashcommand ping_cmd("ping", "Check bot status", bot.me.id);
        
        dpp::slashcommand exp_cmd("expense", "Log a new expense", bot.me.id);
        exp_cmd.add_option(dpp::command_option(dpp::co_string, "query", "What did you spend?", true));

        if (env_guild_id) {
            // Development Mode: Register instantly in the specific server
            dpp::snowflake guild_id(std::stoull(env_guild_id));
            bot.guild_command_create(ping_cmd, guild_id);
            bot.guild_command_create(exp_cmd, guild_id);
            std::cout << "ExpenseBot: Commands registered to guild " << guild_id << std::endl;
        } else {
            // Production Mode: Register globally (visible in ALL servers)
            bot.global_command_create(ping_cmd);
            bot.global_command_create(exp_cmd);
            std::cout << "ExpenseBot: Commands registered globally (may take 1h to propagate)" << std::endl;
        }
      }
      std::cout << "ExpenseBot is online and listening!" << std::endl;
    });

    bot.start(dpp::st_wait);

  } catch (const std::exception &e) {
    std::cerr << "FATAL RECOVERY ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}