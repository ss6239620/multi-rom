#include "include/orm/MY_SQL/MySQLAdapter.h"
#include "include/orm/ModelMacros.h"
#include <iostream>
#include "utils/utils.h"
#include "serializer/jsonparser.h"

BEGIN_MODEL_DEFINITION(User, "users")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(username, STRING, .nullable = false, .unique = true, .max_length = 50)
FIELD(email, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
FIELD(is_active, BOOLEAN, .default_value = "1")
END_MODEL_DEFINITION()

BEGIN_MODEL_DEFINITION(Profile, "profile")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(user_id, INTEGER, .nullable = false)
FIELD(fullname, STRING, .nullable = false, .unique = true, .max_length = 50)
FIELD(bio, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
END_MODEL_DEFINITION()

int main()
{
    ORM::MySQLAdapter adapter;
    if (!adapter.connect("localhost", "testuser", "testpass", "testdb"))
    {
        std::cerr << "Connection failed: " << adapter.getLastError() << std::endl;
        return 1;
    }

    auto queryBuilder = adapter.createQueryBuilder();
    auto query = queryBuilder->alias("users", "u")
    .select({"email", "username",})
    .average("is_active", "active")
    .count("id", "cnt")
    .from("users")
    .alias("profile", "p")
    .select({"bio", "fullname",})
    .join("profile", "u.id = p.user_id")
    .groupBy({"u.username", "u.email", "p.bio", "p.fullname"})
    .build();

    std::cout << query << std::endl; 
    

    auto rows = adapter.fetchAllFromQuery(query);
    JSON result = serializationTOJSONNode(rows);

    std::cout << result << std::endl;

    adapter.disconnect();
    return 0;
}