#include <MySQLAdapter.h>
#include <ModelMacros.h>
#include <iostream>
#include <MigrationManager.h>

BEGIN_MODEL_DEFINITION(User, "users")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(username, STRING, .nullable = false, .max_length = 50, .default_value = "sharvesh")
FIELD(email, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
FIELD(is_active, BOOLEAN, .default_value = "1")
FIELD(is_deleted, BOOLEAN, .default_value = "0")
END_MODEL_DEFINITION()

BEGIN_MODEL_DEFINITION(Profile, "profile")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(user_id, INTEGER, .nullable = false)
FIELD(fullname, STRING, .nullable = false, .unique = true, .max_length = 50)
FIELD(bio, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
END_MODEL_DEFINITION()

BEGIN_MODEL_DEFINITION(Account, "account")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(user_id, INTEGER, .nullable = false)
FIELD(price, INTEGER, .nullable = false, .unique = true, .max_length = 50)
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

    adapter.softDelete<User>({{"id", "4"}});

    std::cout << "deleted" << std::endl;

    adapter.disconnect();

    return 0;
}