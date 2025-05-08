# Multi-Database ORM System for C++

---

<p align="center">
  <img src="/asset/multi-orm-logo.png" width="300" height="150" alt="Logo" />
</p>

---

A lightweight, header-only ORM system for C++ that supports multiple database backends with a unified API. Currently features full MySQL support with architecture designed for easy extension to other databases.


# Features ✨

- Database Agnostic Interface - Same API for different database backends
- Expressive Model Definition - Simple macros for model creation
- Fluent Query Builder - Chainable methods for complex queries
- Automatic Migrations - Schema versioning and change detection
- Type Safety - Strongly typed fields and relationships
- Connection Pooling - Efficient database connection management
- Cross-platform - Works on Windows, Linux, and macOS


# Installation 📦
### Requirements
- C++17 compatible compiler
- MySQL client libraries (for MySQL adapter)
- CMake 3.12+ (for building examples)

### Clone the repository:
```bash
  git clone https://github.com/ss6239620/multi-orm.git
  cd multi-orm
```

### Include the headers in your project:
```bash
#include "orm/MySQLAdapter.h"
#include "orm/ModelMacros.h"
```

### Building with CMake
```bash
mkdir build && cd build
cmake ..
make
```
    
# Quick Start 🚀
###  Define Your Model
```bash
#include "orm/ModelMacros.h"

BEGIN_MODEL_DEFINITION(User, "users")
    FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
    FIELD(username, STRING, .max_length = 50, .unique = true)
    FIELD(email, STRING, .max_length = 100, .unique = true)
    FIELD(created_at, DATETIME, .default_value = "CURRENT_TIMESTAMP")
END_MODEL_DEFINITION()
```

### Initialize Database Connection
```bash
ORM::MySQLAdapter adapter;
if (!adapter.connect("localhost", "user", "password", "mydatabase")) {
    std::cerr << "Connection failed: " << adapter.getLastError() << std::endl;
    return 1;
}
```

### Run Migrations
```bash
ORM::MigrationManager::initialize(adapter);
ORM::MigrationManager::migrateModel(adapter, User{});
```

### Basic CRUD Operations
```bash
// Create
adapter.insert<User>({
    {"username", "johndoe"},
    {"email", "john@example.com"}
});

// Read
auto results = adapter.createQueryBuilder()
    ->select({"id", "username"})
    ->from("users")
    ->where("username", "johndoe")
    ->execute();

// Update
adapter.update<User>()
    .set({{"email", "newemail@example.com"}})
    .where("id = 1")
    .execute();

// Delete
adapter.delete_<User>()
    .where("id = 1")
    .execute();
```






# Documentation 📚

# Core Components

| Component         | Description                                 |
|------------------|---------------------------------------------|
| `DatabaseAdapter` | Base interface for all database adapters     |
| `Model`           | Abstract base class for database models      |
| `Field`           | Type-safe field definitions                  |
| `QueryBuilder`    | Fluent interface for building queries        |
| `MigrationManager`| Handles schema versioning and migrations     |

# Supported Field Types

| Type     | Description            |
|----------|------------------------|
| `INTEGER` | Integer values         |
| `FLOAT`   | Floating point numbers |
| `STRING`  | Character data         |
| `BOOLEAN` | True/false values      |
| `DATETIME`| Date and time values   |
| `TEXT`    | Long text data         |
| `BLOB`    | Binary data            |



# Query Builder Examples

###  Complex Query:
```bash
auto query = adapter.createQueryBuilder()
    ->select({"u.username", "COUNT(p.id) as post_count"})
    ->from("users", "u")
    ->leftJoin("posts", "p.user_id = u.id")
    ->groupBy({"u.id"})
    ->having("post_count > 5")
    ->orderBy("post_count", "DESC")
    ->limit(10)
    ->build();
```

###  Aggregate Functions:
```bash
auto query = adapter.createQueryBuilder()
    ->count("id", "total_users")
    ->from("users")
    ->build();
```

### Advanced Usage 🛠️
```bash
adapter.beginTransaction();
try {
    adapter.insert<User>({...});
    adapter.update<User>(...).execute();
    adapter.commitTransaction();
} catch (const std::exception& e) {
    adapter.rollbackTransaction();
}
```

### Custom Migrations
```bash
class CustomMigration : public ORM::MigrationInterface {
public:
    void up(DatabaseAdapter &adapter) override {
        adapter.executeRawSQL("ALTER TABLE users ADD COLUMN last_login DATETIME", {});
    }
    
    void down(DatabaseAdapter &adapter) override {
        adapter.executeRawSQL("ALTER TABLE users DROP COLUMN last_login", {});
    }
};

// Register migration
ORM::MigrationManager::registerMigration("custom_migration", []{
    return std::make_unique<CustomMigration>();
});
```
# Best Practices
- Always use migrations for schema changes
- Use the query builder for complex queries
- Handle database errors appropriately
- Manage connections carefully (RAII recommended)
- Use transactions for multiple related operations

# Extending the System
### To add support for another database:
- Create a new adapter class inheriting from DatabaseAdapter
- Implement all pure virtual methods
- Create a corresponding query builder if needed
- Update the factory methods to support your new adapter

# Limitations
- Currently only MySQL is fully implemented
- Complex relationships require manual join handling
- Bulk operations could be more optimized

