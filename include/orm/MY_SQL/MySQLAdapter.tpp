// include/orm/MySQLAdapter.tpp
#ifndef MYSQL_ADAPTER_TPP
#define MYSQL_ADAPTER_TPP

#include <map>
#include <stdexcept>
#include <iostream>

namespace ORM
{

    template <typename ModelType>
    bool MySQLAdapter::insert(const std::map<std::string, std::string> &fields)
    {
        if (!connection_)
        {
            lastError_ = "Not connected to database";
            return false;
        }

        ModelType model;
        for (const auto &[fieldName, value] : fields)
        {
            try
            {
                model.setFieldValue(fieldName, value);
            }
            catch (const std::runtime_error &e)
            {
                lastError_ = e.what();
                return false;
            }
        }
        return insertRecord(model);
    }

    template <typename ModelType>
    bool MySQLAdapter::bulkInsert(const std::vector<std::map<std::string, std::string>> &entities)
    {
        ModelType model;
        for (size_t i = 0; i < entities.size(); i++)
        {
            for (const auto &[fieldName, value] : entities[i])
            {
                try
                {
                    model.setFieldValue(fieldName, value);
                }
                catch (const std::exception &e)
                {
                    lastError_ = e.what();
                    return false;
                }
            }
            insertRecord(model);
        }
        return true;
    }

    template <typename ModelType>
    std::vector<std::map<std::string, std::string>> MySQLAdapter::find()
    {
        ModelType model;
        queryBuilder_.select({"*"}).from(model.getTableName());
        return fetchAllFromQuery(queryBuilder_.build());
    }

    template <typename ModelType>
    std::map<std::string, std::string> MySQLAdapter::findOne(const std::string &condition)
    {
        ModelType model;
        queryBuilder_.select({"*"}).from(model.getTableName()).where(condition).limit(1);
        auto result = fetchAllFromQuery(queryBuilder_.build());
        return result.empty() ? std::map<std::string, std::string>() : result[0];
    }

    template <typename ModelType>
    std::map<std::string, std::string> MySQLAdapter::findById(const std::string &id)
    {
        ModelType model;
        std::string pkField;

        for (const auto &field : model.getFields())
        {
            if (field->getOptions().primary_key)
            {
                pkField = field->getName();
                break;
            }
        }
        if (pkField.empty())
        {
            lastError_ = "No primary key found for table " + model.getTableName();
            return {};
        }

        queryBuilder_.select({"*"}).from(model.getTableName()).where(pkField + " = " + id).limit(1);
        auto result = fetchAllFromQuery(queryBuilder_.build());
        return result.empty() ? std::map<std::string, std::string>() : result[0];
    }

    template <typename ModelType>
    std::vector<std::map<std::string, std::string>> MySQLAdapter::findBy(const std::string &condition)
    {
        ModelType model;
        queryBuilder_.select({"*"}).from(model.getTableName());

        if (!condition.empty())
        {
            queryBuilder_.where(condition);
        }
        return fetchAllFromQuery(queryBuilder_.build());
    }

    template <typename ModelType>
    std::pair<std::vector<std::map<std::string, std::string>>, int> MySQLAdapter::findAndCount()
    {
        ModelType model;
        int total = 0;

        queryBuilder_.count("*", "total").from(model.getTableName());
        auto totalData = fetchAllFromQuery(queryBuilder_.build());
        if (!totalData.empty())
        {
            total = std::stoi(totalData[0]["total"]);
        }
        queryBuilder_.select({"*"}).from(model.getTableName());
        auto data = fetchAllFromQuery(queryBuilder_.build());

        return {data, total};
    }

    template <typename ModelType>
    bool MySQLAdapter::exists(const std::string &condition)
    {
        ModelType model;
        queryBuilder_.select({"1"}).from(model.getTableName()).where(condition).limit(1);
        auto result = fetchAllFromQuery(queryBuilder_.build());
        return !result.empty();
    }

    template <typename ModelType>
    int MySQLAdapter::count(const std::string &condition)
    {
        ModelType model;
        queryBuilder_.count("*", "total").from(model.getTableName()).where(condition);
        auto result = fetchAllFromQuery(queryBuilder_.build());
        return result.empty() ? 0 : std::stoi(result[0]["total"]);
    }

    template <typename ModelType>
    bool MySQLAdapter::update(const std::map<std::string, std::string> &critria, const std::map<std::string, std::string> &partialEntity)
    {
        ModelType model;
        std::string query = "UPDATE " + model.getTableName() + " SET ";

        // build set clause
        bool first = true;
        for (const auto &[field, value] : partialEntity)
        {
            if (!first)
                query += ", ";
            first = false;
            // check if expression is literal
            if (value.find("(") != std::string::npos || // function call like now()
                value.find("+") != std::string::npos || // math operation
                isdigit(value[0]))                      // numeric literal
            {
                query += field + " = " + value;
            }
            else
            {
                query += field + " = '" + escapeString(value) + "'";
            }
        }
        // Build where clause
        if (!critria.empty())
        {
            query += " WHERE ";
            first = true;
            for (const auto &[field, value] : critria)
            {
                if (!first)
                    query += " AND ";
                first = false;
                query += field + " = '" + escapeString(value) + "'";
            }
        }

        MYSQL_RES *temp = nullptr;
        bool success = executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }

    template <typename ModelType>
    bool MySQLAdapter::updateById(const std::string &id, const std::map<std::string, std::string> &updates)
    {
        ModelType model;
        std::string pkField;

        for (const auto &field : model.getFields())
        {
            if (field->getOptions().primary_key)
            {
                pkField = field->getName();
                break;
            }
        }
        if (pkField.empty())
        {
            lastError_ = "No primary key found for table " + model.getTableName();
            return false;
        }
        return update<ModelType>({{pkField, id}}, updates);
    }

    template <typename ModelType>
    ModelType MySQLAdapter::create(const std::initializer_list<std::pair<std::string, std::string>> &fields)
    {
        ModelType model;
        for (const auto &[field, value] : fields)
        {
            model.setFieldValue(field, value);
        }
        return model;
    }

    template <typename ModelType>
    bool MySQLAdapter::save(const ModelType &entity)
    {
        std::string pkField;
        std::string pkValue;

        for (const auto &field : entity.getFields())
        {
            if (field->getOptions().primary_key)
            {
                pkField = field->getName();
                pkValue = entity.getFieldValue(pkField);
                break;
            }
        }
        if (pkField.empty())
        {
            lastError_ = "No primary key found for table " + entity.getTableName();
            return false;
        }
        if (!pkField.empty())
        {
            auto existing = findById<ModelType>(pkValue);
            if (!existing.empty())
            {
                std::map<std::string, std::string> updates;
                for (const auto &field : entity.getFields())
                {
                    if (field->getName() != pkField && !entity.getFieldValue(field->getName()).empty()) // Don't update primary key
                    {
                        updates[field->getName()] = entity.getFieldValue(field->getName());
                    }
                }
                return updateById<ModelType>(pkValue, updates);
            }
        }
        // otherwise insert record
        return insertRecord(entity);
    }

    template <typename ModelType>
    bool MySQLAdapter::bulkUpddate(const std::map<std::string, std::string> &critria, const std::map<std::string, std::string> &updates)
    {
        // For MySQL, bulk update is same as regular update with criteria
        return update<ModelType>(critria, updates);
    }

    template <typename ModelType>
    bool MySQLAdapter::increment(const std::string &field, int value, const std::string &condition)
    {
        ModelType model;
        std::string query = "UPDATE " + model.getTableName() + " SET " + field + " = " + field + " + " + std::to_string(value);

        if (!condition.empty())
        {
            query += " WHERE " + condition + ";";
        }

        MYSQL_RES *temp;
        bool success = executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }

    template <typename ModelType>
    bool MySQLAdapter::decrement(const std::string &field, int value, const std::string &condition)
    {
        ModelType model;
        std::string query = "UPDATE " + model.getTableName() + " SET " + field + " = " + field + " - " + std::to_string(value);

        if (!condition.empty())
        {
            query += " WHERE " + condition + ";";
        }

        MYSQL_RES *temp;
        bool success = executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }

    template <typename ModelType>
    bool MySQLAdapter::delete_(const std::map<std::string, std::string> &critria)
    {
        ModelType model;
        std::string query = "DELETE FROM " + model.getTableName();

        if (!critria.empty())
        {
            query += " WHERE ";
            bool first = true;
            for (const auto &[field, value] : critria)
            {
                if (!first)
                    query += " AND ";
                first = false;
                query += field + " = '" + escapeString(value) + "'";
            }
        }
        MYSQL_RES *temp;
        bool success = executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }

    template <typename ModelType>
    bool MySQLAdapter::deleteById(const std::string &id)
    {
        ModelType model;
        std::string pkField;

        for (const auto &field : model.getFields())
        {
            if (field->getOptions().primary_key)
            {
                pkField = field->getName();
                break;
            }
        }
        if (pkField.empty())
        {
            lastError_ = "No primary key found for table " + model.getTableName();
            return false;
        }
        return delete_<ModelType>({{pkField, id}});
    }

    template <typename ModelType>
    bool MySQLAdapter::softDelete(const std::map<std::string, std::string> &critria, const std::string &deleteColumn)
    {
        ModelType model;

        bool hasIsDeleted = false;
        for (const auto &field : model.getFields())
        {
            if (field->getName() == "is_deleted")
            {
                hasIsDeleted = true;
                break;
            }
        }
        if (!hasIsDeleted)
        {
            lastError_ = "Table " + model.getTableName() + " doesn't support soft delete (missing is_deleted field)";
            return false;
        }
        std::string query = "UPDATE " + model.getTableName() + " SET " + deleteColumn + " = 1";
        if (!critria.empty())
        {
            bool first = true;
            query += " WHERE ";
            for (const auto &[field, value] : critria)
            {
                if (!first)
                    query += " AND ";
                first = false;
                query += field + " = '" + escapeString(value) + "'";
            }
        }

        MYSQL_RES *temp;
        bool success = executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }

    template <typename ModelType>
    bool MySQLAdapter::remove(const ModelType &entity)
    {
        std::string pkField;
        std::string pkValue;

        
    }
}
#endif // MYSQL_ADAPTER_TPP