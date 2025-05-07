#include "MigrationManager.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <chrono>

namespace ORM
{
    namespace fs = std::filesystem;

    std::unordered_map<std::string, std::function<std::unique_ptr<MigrationInterface>()>> MigrationManager::migrationRegistry;

    void MigrationManager::intialize(DatabaseAdapter &adapter)
    {
        ensureMigrationTable(adapter);

        // create migrations/ fireactory if it dosent exists
        if (!fs::exists("migrations"))
            fs::create_directory("migrations");
    }

    void MigrationManager::registerMigration(const std::string &version, std::function<std::unique_ptr<MigrationInterface>()> creator)
    {
        migrationRegistry[version] = creator;
    }

    void MigrationManager::migrateModel(DatabaseAdapter &adapter, const Model &model)
    {
        std::string tableName = model.getTableName();
        std::string schemaHash = claculateSchemaHash(model);
        JSON schemaJSON = generateSchemaJSON(model);

        JSON lastMigration = getLastMigration(adapter, tableName);

        if (lastMigration.isNULL()) // first migration of the model
        {
            std::string version = "001_intial";
            std::vector<std::string> upSql = {adapter.createTableSQL(model)};
            std::vector<std::string> downSql = {"DROP TABLE " + tableName};

            createMigrationFile(version + "_create_" + tableName, upSql, downSql);
            adapter.createTable(model);
            createMigrationRecord(adapter, tableName, schemaHash, schemaJSON, version);
        }
        else if (lastMigration["schema_hash"].get<std::string>() != schemaHash) // schema has changed need migration
        {
            JSON old_schema = parseSchemaJSON(lastMigration["schema_json"].get<std::string>());

            std::string version = generateVersionNumber();
            std::string migrationName = version + "_after_" + tableName;

            // already generated upsql and downsql when create schema
            std::vector<std::string> upsql;
            std::vector<std::string> downsql;

            // compare schema and genearet alter statements
            compareAndUpdateSchema(adapter, model, old_schema, upsql, downsql);

            createMigrationFile(migrationName, upsql, downsql);

            createMigrationRecord(adapter, tableName, schemaHash, schemaJSON, version);
        }
    }

    // Create migration file
    void MigrationManager::createMigrationFile(const std::string &name, const std::vector<std::string> &upSql, const std::vector<std::string> &downSql)
    {
        std::ofstream file("migrations/" + name + ".cpp");

        file << "#include \"MigrationManager.h\"\n";
        file << "#include \"DatabaseTypes.h\"\n\n";

        file << "using namespace ORM;\n\n";

        file << "class Migration_" << name << " : public ORM::MigrationInterface {\n";
        file << "public:\n";

        file << "     void up(ORM::DatabaseAdapter &adapter) override { \n";
        for (const auto &sql : upSql)
        {
            file << "         adapter.executeRawSQL(\"" << sql << "\",{});\n";
        }
        file << "     }\n\n";

        file << "      void down(ORM::DatabaseAdapter &adapter) override { \n";
        for (const auto &sql : downSql)
        {
            file << "         adapter.executeRawSQL(\"" << sql << "\",{});\n";
        }
        file << "     }\n";

        file << "};\n\n";

        // auto-register in registry
        file << "static bool registered = [] {\n";
        file << "     MigrationManager::registerMigration(\"" << name << "\",[]{\n";
        file << "           return std::make_unique<Migration_" << name << ">();\n";
        file << "     });\n";
        file << "     return true;\n";
        file << "}();\n";
    }

    std::string MigrationManager::getCurrentVersion(DatabaseAdapter &adapter, const std::string &modelName)
    {
        auto result = adapter.executeQuery(
            "SELECT version FROM migrations WHERE model_name = ? AND is_applied = 1 ORDER BY applied_at DESC LIMIT 1",
            {modelName});

        if (result.empty())
        {
            return ""; // No migrations applied yet
        }

        return result[0]["version"];
    }

    std::string MigrationManager::claculateSchemaHash(const Model &model)
    {
        std::ostringstream schemaStream;
        schemaStream << JSON::stringify(generateSchemaJSON(model));

        std::string schemaString = schemaStream.str();
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char *>(schemaString.c_str()), schemaString.length(), hash);

        std::ostringstream hashStream;
        for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++)
            hashStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        return hashStream.str();
    }

    JSON MigrationManager::generateSchemaJSON(const Model &model)
    {
        JSON schema(JSONType::ARRAY);

        for (const auto &field : model.getFields())
        {
            if (!field)
            {
                std::cerr << "Warning: Null field encountered, skipping" << std::endl;
                continue;
            }
            auto opt = field->getOptions();
            JSON fieldJson(JSONType::OBJECT);

            fieldJson["name"] = JSON(field->getName());
            // Use static_cast<int> for enum types
            fieldJson["type"] = JSON(static_cast<int>(field->getType()));
            fieldJson["primary_key"] = JSON(opt.primary_key);
            fieldJson["auto_increment"] = JSON(opt.auto_increment);
            fieldJson["default_value"] = JSON(opt.default_value);
            // Use static_cast<int> for max_length if it's meant to be integer
            fieldJson["max_length"] = JSON(static_cast<int>(opt.max_length));
            fieldJson["nullable"] = JSON(opt.nullable);
            fieldJson["unique"] = JSON(opt.unique);

            schema.appendArray(fieldJson);
        }
        return schema;
    }

    JSON MigrationManager::parseSchemaJSON(const std::string &jsonStr)
    {
        try
        {
            JSON parsed = JSON::parse(jsonStr);
            if (!parsed.isArray())
            {
                throw std::runtime_error("Schema JSON should be an array of field definitions");
            }
            return parsed;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
            std::cerr << "Error: " << e.what() << std::endl;
            throw;
        }
    }

    void MigrationManager::ensureMigrationTable(DatabaseAdapter &adapter)
    {
        try
        {
            std::string query = R"(
                CREATE TABLE IF NOT EXISTS migrations (
                    id INT PRIMARY KEY AUTO_INCREMENT,
                    model_name TEXT NOT NULL,
                    version TEXT NOT NULL,
                    schema_hash TEXT NOT NULL,
                    schema_json TEXT NOT NULL,
                    is_applied BOOLEAN NOT NULL DEFAULT 1,
                    applied_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            )";
            if (!adapter.executeRawSQL(query, {}))
            {
                throw std::runtime_error("Failed to execute migrations table creation SQL");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error ensuring migrations table exists: " << e.what() << std::endl;
            throw;
        }
    }

    bool MigrationManager::migrationExists(DatabaseAdapter &adapter, const std::string &tableName, const std::string &hash)
    {
        auto result = adapter.executeQuery(
            "SELECT 1 FROM migrations WHERE model_name = ? AND schema_hash = ?",
            {tableName, hash});
        return !result.empty();
    }

    void MigrationManager::createMigrationRecord(DatabaseAdapter &adapter, const std::string &tableName, const std::string &hash, const JSON &schemaJSON, const std::string &version)
    {

        // Validate schemaJSON before storing
        if (!schemaJSON.isArray())
            throw std::runtime_error("Schema JSON must be an array");

        for (size_t i = 0; i < schemaJSON.size(); i++)
        {
            JSON field = schemaJSON[i];
            if (field.isNULL() || (field["name"].get<std::string>().empty()))
            {
                throw std::runtime_error("Invalid field in schema - cannot store empty fields");
            }
        }

        std::string escapedModelName = adapter.escapeString(tableName);
        std::string escapedVersion = adapter.escapeString(version);
        std::string escapedHash = adapter.escapeString(hash);
        std::string escapedSchema = adapter.escapeString(JSON::stringify(schemaJSON));

        std::string query = "INSERT INTO migrations (model_name,version,schema_hash,schema_json) VALUES ('" +
                            escapedModelName + "','" + escapedVersion + "','" + escapedHash + "','" + escapedSchema + "')";

        if (!adapter.executeRawSQL(query, {}))
        {
            throw std::runtime_error("Failed to insert record in migrations table.");
        }
    }

    JSON MigrationManager::getLastMigration(DatabaseAdapter &adapter, const std::string &tableName)
    {
        auto result = adapter.executeQuery(
            "SELECT schema_hash,schema_json FROM migrations where model_name = ? ORDER BY applied_at DESC LIMIT 1", {tableName});

        if (result.empty())
            return JSON();

        JSON lastMigration(JSONType::OBJECT);
        lastMigration["schema_hash"] = JSON(result[0]["schema_hash"]);
        lastMigration["schema_json"] = JSON(result[0]["schema_json"]);
        return lastMigration;
    }

    void MigrationManager::compareAndUpdateSchema(DatabaseAdapter &adapter, const Model &model, const JSON &oldSchema, std::vector<std::string> &upSql, std::vector<std::string> &downSql)
    {
        std::string tableName = model.getTableName();

        // create hashmap for fast lookup
        std::unordered_map<std::string, JSON> oldFields;
        for (size_t i = 0; i < oldSchema.size(); i++)
        {
            JSON field = oldSchema[i];
            // Skip empty or invalid fields
            if (field.isNULL())
            {
                std::cerr << "Warning: Skipping empty/invalid field at index " << i << std::endl;
                continue;
            }
            try
            {
                std::string fieldName = field["name"].get<std::string>();
                oldFields[fieldName] = field;
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error("Invalid field structure: " + std::string(e.what()));
            }
        }
        // check for modified field
        for (const auto &field : model.getFields())
        {
            std::string fieldName = field->getName();
            auto it = oldFields.find(fieldName);

            if (it == oldFields.end()) // new field found
            {
                std::string alterSql = generateAlterAddColumn(tableName, *field);
                upSql.push_back(alterSql);
                downSql.push_back(generateAlterDropColumn(tableName, fieldName));
                adapter.executeRawSQL(alterSql, {});
            }

            else
            {
                // existing field possibly its renamed
                const JSON &oldField = it->second;

                if (static_cast<int>(field->getType()) != oldField["type"].get<int>() ||
                    static_cast<int>(field->getOptions().nullable) != oldField["nullable"].get<bool>() ||
                    static_cast<int>(field->getOptions().max_length) != oldField["max_length"].get<int>())
                {
                    std::string modifySql = generateAlterModifyColumn(tableName, *field);
                    upSql.push_back(modifySql);
                    // Down would revert to old field definition
                    downSql.push_back(generateAlterModifyColumn(tableName,
                                                                Field(oldField["name"].get<std::string>(),
                                                                      static_cast<FieldType>(oldField["type"].get<int>()),
                                                                      FieldOptions{
                                                                          oldField["primary_key"].get<bool>(),
                                                                          oldField["auto_increment"].get<bool>(),
                                                                          oldField["nullable"].get<bool>(),
                                                                          oldField["unique"].get<bool>(),
                                                                          oldField["max_length"].get<int>(),
                                                                          oldField["default_value"].get<std::string>()})));
                    adapter.executeRawSQL(modifySql, {});
                }
            }
        }
        // check for droped column
        handleDroppedColumn(adapter, tableName, model, oldSchema, upSql, downSql);
    }

    void MigrationManager::alterTable(DatabaseAdapter &adapter, const Model &model, const JSON &oldSchema)
    {
        // compareAndUpdateSchema(adapter, model, oldSchema);
    }

    void MigrationManager::handleDroppedColumn(DatabaseAdapter &adapter, const std::string &tableName, const Model &model, const JSON &oldSchema, std::vector<std::string> &upSql, std::vector<std::string> &downSql)
    {
        std::unordered_set<std::string> currentFields;
        for (const auto &field : model.getFields())
        {
            currentFields.insert(field->getName());
        }

        for (size_t i = 0; i < oldSchema.size(); i++)
        {
            JSON oldField = oldSchema[i];
            std::string fieldName = oldField["name"].get<std::string>();

            if (currentFields.find(fieldName) == currentFields.end())
            {
                std::string dropSql = generateAlterDropColumn(tableName, fieldName);
                upSql.push_back(dropSql);

                // down would recreate the column
                downSql.push_back(generateAlterAddColumn(tableName,
                                                         Field(oldField["name"].get<std::string>(),
                                                               static_cast<FieldType>(oldField["type"].get<int>()),
                                                               FieldOptions{
                                                                   oldField["primary_key"].get<bool>(),
                                                                   oldField["auto_increment"].get<bool>(),
                                                                   oldField["nullable"].get<bool>(),
                                                                   oldField["unique"].get<bool>(),
                                                                   oldField["max_length"].get<int>(), // Fix typo: max_lenght -> max_length
                                                                   oldField["default_value"].get<std::string>()})));

                adapter.executeRawSQL(dropSql, {});
            }
        }
    }

    std::string MigrationManager::generateVersionNumber()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm tm;
        localtime_r(&in_time_t, &tm);

        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return ss.str();
    }

    std::string MigrationManager::generateColumnDefination(const Field &field)
    {
        std::ostringstream ss;

        switch (field.getType())
        {
        case FieldType::INTEGER:
            ss << "INTEGER";
            break;
        case FieldType::FLOAT:
            ss << "FLOAT";
            break;
        case FieldType::DOUBLE:
            ss << "DOUBLE";
            break;
        case FieldType::BOOLEAN:
            ss << "BOOLEAN";
            break;
        case FieldType::STRING:
            ss << "VARCHAR(" << (field.getOptions().max_length > 0 ? std::to_string(field.getOptions().max_length) : "255") << ")";
            break;
        case FieldType::TEXT:
            ss << "TEXT";
            break;
        case FieldType::DATETIME:
            ss << "DATETIME";
            break;
        }

        if (field.getOptions().primary_key)
        {
            ss << " PRIMARY KEY";
            if (field.getOptions().auto_increment)
            {
                ss << " AUTOINCREMENT";
            }
        }

        if (!field.getOptions().nullable)
        {
            ss << " NOT NULL";
        }

        if (field.getOptions().unique)
        {
            ss << " UNIQUE";
        }

        if (!field.getOptions().default_value.empty())
        {
            ss << " DEFAULT '" << field.getOptions().default_value << "'";
        }
        return ss.str();
    }

    std::string MigrationManager::generateAlterAddColumn(const std::string &tableName, const Field &field)
    {
        return "ALTER TABLE " + tableName + " ADD COLUMN " + field.getName() + " " + generateColumnDefination(field);
    }

    std::string MigrationManager::generateAlterModifyColumn(const std::string &tableName, const Field &field)
    {
        return "ALTER TABLE " + tableName + " MODIFY COLUMN " + field.getName() + " " + generateColumnDefination(field);
    }

    std::string MigrationManager::generateAlterDropColumn(const std::string &tableName, const std::string &columnName)
    {
        return "ALTER TABLE " + tableName + " DROP COLUMN " + columnName;
    }

    std::vector<std::pair<std::string, JSON>> MigrationManager::getAllMigration(DatabaseAdapter &adapter, const std::string &modelName)
    {
        auto result = adapter.executeQuery("SELECT version, schema_json FROM migrations WHERE model_name = ? ORDER BY applied_at ASC", {modelName});

        std::vector<std::pair<std::string, JSON>> migrations;
        for (const auto &row : result)
            migrations.emplace_back(row.at("version"), parseSchemaJSON(row.at("schema_json")));
        return migrations;
    }

    bool MigrationManager::applyMigration(DatabaseAdapter &adapter, const std::string &modelName, const std::string &version, bool up)
    {
        // Try both naming patterns
        std::string fullVersionName1 = version + "_after_" + modelName;
        std::string fullVersionName2 = version + "_create_" + modelName;

        auto it = migrationRegistry.find(fullVersionName1);
        if (it == migrationRegistry.end())
        {
            it = migrationRegistry.find(fullVersionName2);
        }

        if (it == migrationRegistry.end())
        {
            std::cerr << "Migration not registered for version: " << version
                      << " (tried: " << fullVersionName1 << " and " << fullVersionName2 << ")" << std::endl;
            return false;
        }

        try
        {
            // Update status first (mark as in progress)
            std::string statusUpdate = up
                                           ? "UPDATE migrations SET is_applied = 0 WHERE model_name = ? AND version = ?"
                                           : "UPDATE migrations SET is_applied = 1 WHERE model_name = ? AND version = ?";

            if (!adapter.executeRawSQL(statusUpdate, {modelName, version}))
            {
                throw std::runtime_error("Failed to update migration status");
            }

            // Execute migration
            auto migration = it->second();
            if (up)
            {
                migration->up(adapter);
            }
            else
            {
                migration->down(adapter);
            }

            // Finalize status
            std::string finalizeUpdate = up
                                             ? "UPDATE migrations SET is_applied = 1, applied_at = CURRENT_TIMESTAMP WHERE model_name = ? AND version = ?"
                                             : "UPDATE migrations SET is_applied = 0, applied_at = CURRENT_TIMESTAMP WHERE model_name = ? AND version = ?";

            return adapter.executeRawSQL(finalizeUpdate, {modelName, version});
        }
        catch (const std::exception &e)
        {
            std::cerr << "Migration failed: " << e.what() << std::endl;
            // Revert status on failure
            adapter.executeRawSQL(
                "UPDATE migrations SET is_applied = ? WHERE model_name = ? AND version = ?",
                {up ? "0" : "1", modelName, version});
            return false;
        }
    }

    bool MigrationManager::migrateToVersion(DatabaseAdapter &adapter, const std::string &modelName, const std::string &targetVersion)
    {
        try
        {
            std::string currentVersion = getCurrentVersion(adapter, modelName);
            auto allMigrations = getAllMigration(adapter, modelName);

            if (currentVersion == targetVersion)
            {
                std::cout << "Already at target version: " << targetVersion << std::endl;
                return true;
            }

            // Find positions in the migration sequence
            auto currentIt = allMigrations.begin();
            if (!currentVersion.empty())
            {
                currentIt = std::find_if(allMigrations.begin(), allMigrations.end(),
                                         [&currentVersion](const auto &m)
                                         { return m.first == currentVersion; });
                if (currentIt == allMigrations.end())
                {
                    throw std::runtime_error("Current version " + currentVersion + " not found in migration history");
                }
            }

            auto targetIt = std::find_if(allMigrations.begin(), allMigrations.end(),
                                         [&targetVersion](const auto &m)
                                         { return m.first == targetVersion; });
            if (targetIt == allMigrations.end())
            {
                throw std::runtime_error("Target version " + targetVersion + " not found in migration history");
            }

            // Migrate forward
            if (currentIt < targetIt)
            {
                std::cout << "Migrating forward from " << currentVersion << " to " << targetVersion << std::endl;
                for (auto it = currentIt; it != targetIt; ++it)
                {
                    std::cout << "Applying migration: " << it->first << std::endl;
                    if (!applyMigration(adapter, modelName, it->first, true))
                    {
                        throw std::runtime_error("Failed to apply migration: " + it->first);
                    }
                }
            }
            // Migrate backward
            else if (currentIt > targetIt)
            {
                std::cout << "Migrating backward from " << currentVersion << " to " << targetVersion << std::endl;
                // Need to start from currentIt-1 because we want to roll back the current migration first
                for (auto it = std::prev(currentIt); it >= targetIt; --it)
                {
                    std::cout << "Reverting migration: " << it->first << std::endl;
                    if (!applyMigration(adapter, modelName, it->first, false))
                    {
                        throw std::runtime_error("Failed to revert migration: " + it->first);
                    }
                }
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Migration failed: " << e.what() << std::endl;
            return false;
        }
    }
}