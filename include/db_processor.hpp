#pragma once

#include "connection_pool.hpp"
#include "models/expense.hpp"
#include <string_view>

namespace ragc {

class DatabaseProcessor {
public:
  /**
   * @brief Construct DatabaseProcessor backed by a thread-safe connection pool.
   *
   * @param db_url    PostgreSQL connection string.
   * @param pool_size Number of concurrent DB connections (default: 4).
   */
  explicit DatabaseProcessor(std::string_view db_url, std::size_t pool_size = 4);

  /** Run sequential schema migrations on startup. */
  void init_tables();

  /** Thread-safe: each call acquires its own connection from the pool. */
  bool save_expense(const Expense &expense);

private:
  ConnectionPool pool_;
};

} // namespace ragc
