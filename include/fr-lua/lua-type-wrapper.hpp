#ifndef LUA_TYPE_WRAPPER_HPP
#define LUA_TYPE_WRAPPER_HPP

extern "C" {
#include "lua.h"
}

#include <stdint.h>

#ifdef LUA_WRAPPER_TOP_NAMESPACE

namespace LUA_WRAPPER_TOP_NAMESPACE {

#endif

namespace lua { namespace types {

    static inline const char * id_to_string( int t )
    {
        switch( t ) {
        case LUA_TNIL:
            return "nil";
        case LUA_TBOOLEAN:
            return "boolean";
        case LUA_TLIGHTUSERDATA:
            return "lightuserdata";
        case LUA_TNUMBER:
            return "number";
        case LUA_TSTRING:
            return "string";
        case LUA_TTABLE:
            return "table";
        case LUA_TFUNCTION:
            return "function";
        case LUA_TUSERDATA:
            return "userdata";
        case LUA_TTHREAD:
            return "thread";
        }
        return "none";
    }

    template <int LuaTypeID>
    struct base_id {
        enum { type_index = LuaTypeID };
        static bool check( lua_State *L, int idx )
        {
            int r = lua_type( L, idx );
            return r == type_index;
        }
    };

    template <typename T>
    struct id_integer: public base_id<LUA_TNUMBER> {

        static bool check( lua_State *L, int idx )
        {
            int r = lua_type( L, idx );
            return r == LUA_TNUMBER  ||
                   r == LUA_TBOOLEAN ||
                   r == LUA_TNIL     ||
                   r == LUA_TLIGHTUSERDATA;
        }

        static T get( lua_State *L, int idx )
        {
            int r = lua_type( L, idx );
            switch (r) {
            case LUA_TNUMBER:
                return static_cast<T>(lua_tointeger( L, idx ));
            case LUA_TBOOLEAN:
                return static_cast<T>(lua_toboolean( L, idx ) ? 1 : 0);
            case LUA_TNIL:
                return static_cast<T>( 0 );
            case LUA_TLIGHTUSERDATA:
                return static_cast<T>((LUA_INTEGER)(lua_topointer( L, idx )));
            default:
                break;
            }
            return T( );
        }
    };

    struct id_boolean: public base_id<LUA_TBOOLEAN> {

        static bool check( lua_State *L, int idx )
        {
            int r = lua_type( L, idx );
            return r == LUA_TNUMBER  ||
                   r == LUA_TBOOLEAN ||
                   r == LUA_TNIL     ||
                   r == LUA_TLIGHTUSERDATA;
        }

        static bool get( lua_State *L, int idx )
        {
            int r = lua_type( L, idx );
            switch (r) {
            case LUA_TNUMBER:
                return !!lua_tointeger( L, idx );
            case LUA_TBOOLEAN:
                return !!lua_toboolean( L, idx );
            case LUA_TNIL:
                return false;
            case LUA_TLIGHTUSERDATA:
                return lua_topointer( L, idx ) != NULL;
            default:
                break;
            }
            return false;
        }
    };

    template <typename T>
    struct id_numeric: public base_id<LUA_TNUMBER> {
        static T get( lua_State *L, int idx )
        {
            return static_cast<T>(lua_tonumber( L, idx ));
        }
    };

    template <typename T>
    struct id_user_data: public base_id<LUA_TLIGHTUSERDATA> {
        static T get( lua_State *L, int idx )
        {
            return static_cast<T>(lua_touserdata( L, idx ));
        }
    };

    template <typename T>
    struct id_pointer: public base_id<LUA_TLIGHTUSERDATA> {
        static T get( lua_State *L, int idx )
        {
            return static_cast<T>(lua_topointer( L, idx ));
        }
    };

    template <typename T>
    struct id_function: public base_id<LUA_TFUNCTION> {
        static T get( lua_State *L, int idx )
        {
            return static_cast<T>(lua_tocfunction( L, idx ));
        }
    };

    template <typename T>
    struct id_string {

        enum { type_index = LUA_TSTRING };
        static bool check( lua_State * /*L*/, int /*idx*/ )
        {
            return true;
        }

        static T get( lua_State *L, int idx )
        {
            size_t length = 0;
            const char *t = lua_tolstring( L, idx, &length );
            return t ? T( t, t + length ) : T( );
        }
    };

    template <typename T>
    struct id_string_ptr {

        enum { type_index = LUA_TSTRING };
        static bool check( lua_State * /*L*/, int /*idx*/ )
        {
            return true;
        }

        static T get( lua_State *L, int idx )
        {
            const char *t = lua_tostring( L, idx );
            return T( t ? t : "<nil>" );
        }
    };

    template <typename CT>
    struct id_traits {
        enum { type_index = LUA_TNONE };
    };

    template <>
    struct id_traits<lua_CFunction> : public
           id_function<lua_CFunction> { };

    template <>
    struct id_traits<signed int> : public
           id_integer<signed int> { };

    template <>
    struct id_traits<unsigned int> : public
           id_integer<unsigned int> { };

    template <>
    struct id_traits<signed long> : public
           id_integer<signed long> { };

    template <>
    struct id_traits<unsigned long> : public
           id_integer<unsigned long> { };

#if LUA_VERSION_NUM >= 503
    template <>
    struct id_traits<lua_Integer> : public
           id_integer<lua_Integer> { };

    template <>
    struct id_traits<lua_Unsigned> : public
           id_integer<lua_Unsigned> { };
#else
//    template <>
//    struct id_traits<uint64_t> : public
//           id_integer<uint64_t> { };
#endif

    template <>
    struct id_traits<signed short> : public
           id_integer<signed short> { };

    template <>
    struct id_traits<unsigned short> : public
           id_integer<unsigned short> { };

    template <>
    struct id_traits<signed char> : public
           id_integer<signed char> { };

    template <>
    struct id_traits<unsigned char> : public
           id_integer<unsigned char> { };

    template <>
    struct id_traits<std::string> : public
           id_string<std::string> { };

    template <>
    struct id_traits<const char *> : public
           id_string_ptr<const char *> { };

    template <>
    struct id_traits<float> : public
           id_numeric<float> { };

    template <>
    struct id_traits<double> : public
           id_numeric<double> { };

    template <>
    struct id_traits<long double> : public
           id_numeric<long double> { };

    template <>
    struct id_traits<void *> : public
           id_user_data<void *> { };

    template <>
    struct id_traits<const void *> : public
           id_pointer<const void *> { };

    template <>
    struct id_traits<bool> : public
           id_boolean { };

}}

#ifdef LUA_WRAPPER_TOP_NAMESPACE
}
#endif


#endif // LUATYPEWRAPPER_HPP
