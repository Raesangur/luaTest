#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

/* This code was inspired from OneLoneCoder (javidx9)'s video on Embedding Lua into C++
 * https://www.youtube.com/watch?v=4l5HdmPoynw
 */

extern "C"
{
#include "vendor/lua540/include/lua.h"

#include "vendor/lua540/include/lauxlib.h"
#include "vendor/lua540/include/lualib.h"
}

#ifdef _WIN32
#pragma comment(lib, "vendor/lua540/lua54.lib")
#endif


class LuaObject
{
#define LUAOBJECT_ASSERT(condition, ...)                                                           \
    do                                                                                             \
    {                                                                                              \
        if(!(condition))                                                                           \
        {                                                                                          \
            throw std::runtime_error(std::string{#__VA_ARGS__});                                    \
        }                                                                                          \
    } while(false)

    template<typename T, typename U>
    using check_type = std::is_same<std::remove_cvref<T>, std::remove_cvref<U>>;

private:
    constexpr static int TOP_OF_STACK = -1;

    lua_State* m_luaState;

public:
    LuaObject()
    {
        m_luaState = luaL_newstate();
    }
    ~LuaObject()
    {
        lua_close(m_luaState);
    }
#pragma region Execution
    int
    RunString(const std::string& command)
    {
        int returnCode = luaL_dostring(m_luaState, command.c_str());
        LUAOBJECT_ASSERT(returnCode == LUA_OK);

        return returnCode;
    }

    int
    RunFile(const std::string& file)
    {
        int returnCode = luaL_dofile(m_luaState, file.c_str());
        LUAOBJECT_ASSERT(returnCode == LUA_OK);

        return returnCode;
    }
#pragma endregion

    template<typename T>
    [[nodiscard]] T
    GetValue(const std::string& variableName)
    {
        /* Put the requested variable on top of the LUA stack */
        lua_getglobal(m_luaState, variableName.c_str());

        T var;

        /* According to the requested type, check its compatibility and return it */
        if constexpr(std::is_integral_v<T> == true)
        {
            /* Check compatibility */
            LUAOBJECT_ASSERT(lua_isinteger(m_luaState, TOP_OF_STACK)
                               || lua_isnumber(m_luaState, TOP_OF_STACK),
                             "Conversion impossible");

            /* Extract variable */
            var = static_cast<T>(lua_tointeger(m_luaState, TOP_OF_STACK));
        }
        else if constexpr(std::is_floating_point_v<T> == true)
        {
            /* Check compatibility */
            LUAOBJECT_ASSERT(lua_isnumber(m_luaState, TOP_OF_STACK), "Conversion impossible");

            /* Extract variable */
            var = static_cast<T>(lua_tonumber(m_luaState, TOP_OF_STACK));
        }
        else if constexpr(check_type<T, std::string>::value == true)
        {
            /* Check compatibility */
            LUAOBJECT_ASSERT(lua_isstring(m_luaState, TOP_OF_STACK), "Conversion impossible");

            /* Extract variable */
            var = T{lua_tostring(m_luaState, TOP_OF_STACK)};
        }
        else
        {
            /* clang-format off */
            static_assert(std::is_integral_v<T> ||
                          std::is_floating_point_v<T> ||
                          check_type<T, std::string>::value,
                          "Requested type is not supported");
            /* clang-format on */
        }

        return var;
    }

    [[nodiscard]]
    operator lua_State*()
    {
        return m_luaState;
    }

#undef LUAOBJECT_ASSERT
};

int
main()
{
    LuaObject L{};

    L.RunFile("test1.lua");

    std::string a = L.GetValue<std::string>("a");
    std::cout << a;

    return 0;
}
