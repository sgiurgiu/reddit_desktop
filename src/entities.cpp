#include "entities.h"
#include <fmt/format.h>

std::string make_user_agent(const user& u)
{
    return fmt::format("{}:{}:v1.0 (by /u/{}",u.website,u.app_name,u.username);
}
