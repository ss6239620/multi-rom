// DatabaseTypes.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "utils/utils.h"

namespace ORM
{
    enum class FieldType
    {
        INTEGER,  /**< Integer type */
        FLOAT,    /**< Floating point type */
        DOUBLE,   /**< Double precision floating point type */
        STRING,   /**< String type */
        BOOLEAN,  /**< Boolean type */
        TEXT,     /**< Text type */
        DATETIME, /**< DateTime type */
        BLOB      /**< Binary Large Object (BLOB) type */
    };

    struct FieldOptions
    {
        // Declaration order matters for designated initializers
        bool primary_key = false;    /**< Indicates if the field is a primary key */
        bool auto_increment = false; /**< Indicates if the field auto increments */
        bool nullable = false;       /**< Indicates if the field can be null */
        bool unique = false;         /**< Indicates if the field has a unique constraint */
        int max_length = 0;          /**< Maximum length for fields like strings */
        std::string default_value;   /**< Default value for the field */
    };

    // Class representing a single field in a model
    class Field
    {
    public:
        /**
         * Constructor to initialize a Field object.
         *
         * @param name The name of the field
         * @param type The type of the field (e.g., string, integer)
         * @param options Optional field options (defaults to empty options)
         */
        Field(const std::string &name, FieldType type, const FieldOptions &options = {})
            : name_(name), type_(type), options_(options) {}

        virtual ~Field() = default;

        /**
         * Get the name of the field.
         *
         * @return The field's name as a string
         */
        std::string getName() const { return name_; }
        /**
         * Get the type of the field.
         *
         * @return The field type (FieldType enum)
         */
        FieldType getType() const { return type_; }
        /**
         * Get the options (constraints) for the field.
         *
         * @return The FieldOptions struct associated with this field
         */
        FieldOptions getOptions() const { return options_; }

    private:
        std::string name_;     /**< Field name */
        FieldType type_;       /**< Field type */
        FieldOptions options_; /**< Field options (constraints, etc.) */
    };

    // Abstract base class for models, defining the required interface for any model class
    class Model
    {
    public:
        virtual ~Model() = default;

        /**
         * Get the name of the table associated with this model.
         *
         * @return The table name as a string
         */
        virtual const std::string &getTableName() const = 0;
        /**
         * Get a list of fields for this model.
         *
         * @return A vector of unique pointers to Field objects
         */
        virtual const std::vector<std::unique_ptr<Field>> &getFields() const = 0;
        /**
         * Set the value of a field.
         *
         * @param fieldName The name of the field to set
         * @param values The value to set for the field
         */
        virtual void setFieldValue(const std::string &fieldName, const std::string &value) = 0;

        /**
         * Get the value of a field.
         *
         * @param fieldName The name of the field to retrieve
         * @return The value of the field as a string
         */
        virtual std::string getFieldValue(const std::string &fieldName) const = 0;
    };

    class QueryBuilder;

    // Abstract base class for database adapters, providing the interface for interacting with databases
    class DatabaseAdapter
    {
    public:
        virtual ~DatabaseAdapter() = default;
        /**
         * Connect to a database.
         *
         * @param host The host name or IP address of the database server
         * @param user The username for the database connection
         * @param password The password for the database connection
         * @param dbname The name of the database to connect to
         * @return True if the connection was successful, false otherwise
         */
        virtual bool connect(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname) = 0;

        /**
         * Create a table in the database based on the model's fields and options.
         *
         * @param model The model containing the table schema
         * @return True if the table creation was successful, false otherwise
         */
        virtual bool createTable(const Model &model) = 0;

        virtual std::string escapeString(const std::string &input) const = 0;

        virtual std::string getLastError() const = 0;

        virtual std::string createTableSQL(const Model &model) = 0;

        virtual std::vector<std::map<std::string, std::string>> executeQuery(
            const std::string &query, const std::vector<std::string> &params) = 0;

        virtual bool executeRawSQL(
            const std::string &query, const std::vector<std::string> &params) = 0;

        /**
         * Insert a new record into a table based on the model's data.
         *
         * @param model The model containing the data to insert
         * @return True if the insertion was successful, false otherwise
         */
        virtual bool insertRecord(const Model &model) = 0;

        /**
         * Disconnect from the database.
         */
        virtual void disconnect() = 0;

        // Querybuilder for mysql
        virtual std::unique_ptr<QueryBuilder> createQueryBuilder() = 0;

        virtual std::vector<std::map<std::string, std::string>> fetchAllFromQuery(const std::string &query) = 0;
    };

    class QueryBuilder
    {
    public:
        virtual ~QueryBuilder() = default;

        // basic select
        virtual QueryBuilder &select(const std::vector<std::string> &columns = {"*"}) = 0;
        // Table name
        virtual QueryBuilder &from(const std::string &table) = 0;
        virtual QueryBuilder &alias(const std::string &table, const std::string &alias) = 0;

        // Aggregate functions
        virtual QueryBuilder &count(const std::string &column, const std::string &alias) = 0;
        virtual QueryBuilder &average(const std::string &column, const std::string &alias) = 0;
        virtual QueryBuilder &sum(const std::string &column, const std::string &alias) = 0;
        virtual QueryBuilder &min(const std::string &column, const std::string &alias) = 0;
        virtual QueryBuilder &max(const std::string &column, const std::string &alias) = 0;
        // JOIN operations
        virtual QueryBuilder &join(const std::string &table, const std::string &condition, const std::string &type = "INNER") = 0;
        virtual QueryBuilder &leftJoin(const std::string &table, const std::string &condition) = 0;
        virtual QueryBuilder &rightJoin(const std::string &table, const std::string &condition) = 0;

        // WHERE Clause
        virtual QueryBuilder &where(const std::string &condition) = 0;
        virtual QueryBuilder &where(const std::string &column, const std::string &value) = 0;

        // GROUP BY and HAVING
        virtual QueryBuilder &groupBy(const std::vector<std::string> &columns) = 0;
        virtual QueryBuilder &having(const std::string &condition) = 0;

        // ORDER BY
        virtual QueryBuilder &orderBy(const std::string &column, const std::string &direction = "ASC") = 0;

        // LIMIT and OFFSET
        virtual QueryBuilder &limit(int count) = 0;
        virtual QueryBuilder &offset(int count) = 0;

        // Execution
        virtual std::string build() = 0;
    };
}