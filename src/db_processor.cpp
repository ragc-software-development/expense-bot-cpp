#include "db_processor.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ragc {

DatabaseProcessor::DatabaseProcessor(std::string_view db_url,
                                     std::size_t pool_size)
    : pool_(db_url, pool_size) {}

void DatabaseProcessor::init_tables() {
  try {
    // Migrations run once at startup — acquire a dedicated connection
    auto guard = pool_.acquire();
    pqxx::work tx(guard.get());

    // 1. Ensure migration tracking table exists
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )");

    // 2. Sequential migration definitions
    const std::vector<std::pair<int, std::string>> migrations = {
        {1, R"(
            CREATE TABLE IF NOT EXISTS expenses (
                id SERIAL PRIMARY KEY,
                amount NUMERIC(12, 2) NOT NULL,
                currency VARCHAR(10) NOT NULL,
                category VARCHAR(50) NOT NULL,
                description TEXT,
                user_id BIGINT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"},
        // Add future migrations here: {2, "ALTER TABLE ..."}
    };

    // 3. Determine current schema version
    pqxx::result res = tx.exec("SELECT MAX(version) FROM schema_migrations");
    int current_version = 0;
    if (!res.empty() && !res[0][0].is_null()) {
      current_version = res[0][0].as<int>();
    }

    // 4. Apply pending migrations
    for (const auto &[version, sql] : migrations) {
      if (version > current_version) {
        std::cout << "[DB] Applying migration v" << version << "..." << std::endl;
        tx.exec(sql);
        tx.exec("INSERT INTO schema_migrations (version) VALUES ($1)",
                pqxx::params{version});
      }
    }

    tx.commit();
    std::cout << "[DB] Schema up to date." << std::endl;

  } catch (const pqxx::sql_error &e) {
    std::cerr << "[DB] Migration Error: " << e.what() << std::endl;
    std::cerr << "[DB] Failed Query: " << e.query() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "[DB] Error during schema initialization: " << e.what() << std::endl;
  }
}

bool DatabaseProcessor::save_expense(const Expense &expense) {
  try {
    // Each call gets its own connection — fully thread-safe
    auto guard = pool_.acquire();
    pqxx::work tx(guard.get());

    tx.exec(
        "INSERT INTO expenses (amount, currency, category, description, user_id) "
        "VALUES ($1, $2, $3, $4, $5)",
        pqxx::params{expense.amount, expense.currency, expense.category,
                     expense.description, expense.user_id});

    tx.commit();
    return true;

  } catch (const pqxx::sql_error &e) {
    std::cerr << "[DB] SQL Error: " << e.what() << std::endl;
    std::cerr << "[DB] Failed Query: " << e.query() << std::endl;
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[DB] Standard Error: " << e.what() << std::endl;
    return false;
  }
}

} // namespace ragc
