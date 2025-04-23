#pragma once
#include "DatabaseTypes.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <typeinfo>
#include <typeindex>
#include <map>

namespace ORM
{
    /**
     * @class ModelRegistry
     * @brief Registry class for managing fields of different model types.
     *
     * This class maintains a static map from type indices to their associated
     * fields. It allows retrieval and management of fields for each registered model.
     */
    class ModelRegistry
    {
    private:
        /**
         * @brief Get the static field registry.
         *
         * @return Reference to the internal static registry map.
         *         The map associates a type index with a vector of unique pointers to Field objects.
         */
        static std::map<std::type_index, std::vector<std::unique_ptr<Field>>> &getFieldRegistry()
        {
            static std::map<std::type_index, std::vector<std::unique_ptr<Field>>> registry;
            return registry;
        }

    public:
        /**
         * @brief Retrieve or create a field list for a given model type.
         *
         * This template function provides access to the list of fields registered
         * for a specific model type `T`. If the type is not yet in the registry,
         * an empty list will be created and returned.
         *
         * @tparam T The model type for which to retrieve the field list.
         * @return Reference to the vector of unique pointers to Field objects.
         */
        template <typename T>
        static std::vector<std::unique_ptr<Field>> &getFields()
        {
            auto &registry = getFieldRegistry();
            auto type = std::type_index(typeid(T));
            return registry[type];
        }
    };
}

#define BEGIN_MODEL_DEFINITION(className, tableName)                                        \
    class className : public ORM::Model                                                     \
    {                                                                                       \
    public:                                                                                 \
        className()                                                                         \
        {                                                                                   \
            static bool registered = []() { \
            registerFields(); \
            return true; }();                                            \
        }                                                                                   \
        const std::string &getTableName() const override                                    \
        {                                                                                   \
            static std::string name = tableName;                                            \
            return name;                                                                    \
        }                                                                                   \
        const std::vector<std::unique_ptr<ORM::Field>> &getFields() const override          \
        {                                                                                   \
            return ORM::ModelRegistry::getFields<className>();                              \
        }                                                                                   \
        void setFieldValue(const std::string &fieldName, const std::string &value) override \
        {                                                                                   \
            bool fieldExists = false;                                                       \
            for (const auto &field : getFields())                                           \
            {                                                                               \
                if (field->getName() == fieldName)                                          \
                {                                                                           \
                    fieldExists = true;                                                     \
                    break;                                                                  \
                }                                                                           \
            }                                                                               \
            if (!fieldExists)                                                               \
            {                                                                               \
                throw std::runtime_error("Field '" + fieldName + "' does not exist");       \
            }                                                                               \
            fieldValues_[fieldName] = value;                                                \
        }                                                                                   \
        std::string getFieldValue(const std::string &fieldName) const override              \
        {                                                                                   \
            auto it = fieldValues_.find(fieldName);                                         \
            return it != fieldValues_.end() ? it->second : "";                              \
        }                                                                                   \
                                                                                            \
    private:                                                                                \
        std::unordered_map<std::string, std::string> fieldValues_;                          \
        static void registerFields()                                                        \
        {                                                                                   \
            auto &fields = ORM::ModelRegistry::getFields<className>();

#define FIELD(name, type, ...)                                                    \
    fields.emplace_back(std::make_unique<ORM::Field>(#name, ORM::FieldType::type, \
                                                     ORM::FieldOptions{__VA_ARGS__}));

#define END_MODEL_DEFINITION() \
    }                          \
    }                          \
    ;