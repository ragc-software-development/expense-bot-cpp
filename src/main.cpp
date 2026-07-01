#include "config.hpp"
#include "db_processor.hpp"
#include "gemini_client.hpp"
#include "processor.hpp"
#include "receiver.hpp"
#include "worker_pool.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

int main()
{
    try {
        const auto config = ragc::Config::load();
        constexpr std::size_t POOL_SIZE = 4;

        // 1. Infrastructure
        ragc::DatabaseProcessor db(config.db_url, POOL_SIZE);
        db.init_tables();

        dpp::cluster bot(config.token);
        ragc::GeminiClient ai(bot, config.gemini_key);
        ragc::Processor processor(db, ai);
        ragc::WorkerPool workers(POOL_SIZE);

        // AI Reliability: Background thread to retry failed Gemini 503 requests
        std::thread retry_thread([&processor]() {
            std::cout << "[WORKER] Background retry thread started." << std::endl;
            while (true) {
                std::this_thread::sleep_for(std::chrono::minutes(1));
                try {
                    processor.retry_background_tasks();
                } catch (...) {
                }
            }
        });
        retry_thread.detach();

        // 2. Receiver (Event orchestration)
        ragc::Receiver receiver(bot, processor, workers, config.guild_id);
        receiver.wire_events();

        // 3. Start
        std::cout << "ExpenseBot Booting..." << std::endl;
        bot.start(dpp::st_wait);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}