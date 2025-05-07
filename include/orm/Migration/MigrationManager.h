#pragma once

#include "jsonparser.h"
#include "ModelMacros.h"
#include "DatabaseTypes.h"

#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <ctime>
#include <functional>

namespace ORM
{
    class MigrationInterface
    {
    public:
        virtual ~MigrationInterface() = default;
        virtual void up(DatabaseAdapter &adapter) = 0;
        virtual void down(DatabaseAdapter &adapter) = 0;
    };

    class MigrationManager
    {
    public:
        // Intialize the migration system
        static void intialize(DatabaseAdapter &adapter);

        static void registerMigration(const std::string &version, std::function<std::unique_ptr<MigrationInterface>()> creator);

        // main migration method
        static void migrateModel(DatabaseAdapter &adapter, const Model &model);

        // Migration file operation
        static void createMigrationFile(const std::string &name, const std::vector<std::string> &upSql, const std::vector<std::string> &downSql);

        // version control
        static std::string getCurrentVersion(DatabaseAdapter &adapter, const std::string &modelName);
        // migrating to specfic version
        static bool migrateToVersion(DatabaseAdapter &adater, const std::string &modelName, const std::string &targetVersion);

    private:
        static std::unordered_map<std::string, std::function<std::unique_ptr<MigrationInterface>()>> migrationRegistry;

        // schema opertaions
        static std::string claculateSchemaHash(const Model &model);
        static JSON generateSchemaJSON(const Model &model);
        static JSON parseSchemaJSON(const std::string &json);

        // Migration Tracking
        static void ensureMigrationTable(DatabaseAdapter &adapter);
        static bool migrationExists(DatabaseAdapter &adapter, const std::string &tableName, const std::string &hash);
        static void createMigrationRecord(DatabaseAdapter &adapter, const std::string &tableName, const std::string &hash, const JSON &schemaJson, const std::string &version);
        static JSON getLastMigration(DatabaseAdapter &adapter, const std::string &tableName);

        // Schema Compariasion and alteration
        static void compareAndUpdateSchema(DatabaseAdapter &adapter, const Model &model, const JSON &oldSchema, std::vector<std::string> &upSql, std::vector<std::string> &downSql);
        static void alterTable(DatabaseAdapter &adapter, const Model &model, const JSON &oldSchema);
        static void handleFieldChanges(DatabaseAdapter &adapter, const std::string &tableaName, const Field &newField, const JSON &oldField);
        static void handleDroppedColumn(DatabaseAdapter &adapter, const std::string &tableName, const Model &model, const JSON &oldSchema, std::vector<std::string> &upSql, std::vector<std::string> &downSql);

        // Version generation
        static std::string generateVersionNumber();

        // SQL generation helpers
        static std::string generateColumnDefination(const Field &field);
        static std::string generateAlterAddColumn(const std::string &tableName, const Field &field);
        static std::string generateAlterModifyColumn(const std::string &tableName, const Field &field);
        static std::string generateAlterDropColumn(const std::string &tableName, const std::string &columnName);

        // migrating to specfic helper function
        static std::vector<std::pair<std::string, JSON>> getAllMigration(DatabaseAdapter &adapter, const std::string &modelName);
        static bool applyMigration(DatabaseAdapter &adapter, const std::string &modelName, const std::string &version, bool up);
    };
}
