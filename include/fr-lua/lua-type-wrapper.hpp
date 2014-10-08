#ifndef LUA_TYPE_WRAPPER_HPP
#define LUA_TYPE_WRAPPER_HPP

extern "C" {
#include "lua.h"
}

#ifdef LUA_WRAPPER_TOP_NAMESPACE

namespace LUA_WRAPPER_TOP_NAMESPACE {

#endif

namespace lua { namespace types {

    static const char * id_to_string( int t )
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
    struct id_pointer: public base_id<LUA_TLIGHTUSERDATA> {
        static T get( lua_State *L, int idx )
        {
            return static_cast<T>(lua_topointer( L, idx ));
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
            const char *t = lua_tolstring( L, idx, length );
            return t ? T( t, length ) : T( );
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
    struct id_traits<const void *> : public
           id_pointer<const void *> { };

}}

#ifdef LUA_WRAPPER_TOP_NAMESPACE
}
#endif


#endif // LUATYPEWRAPPER_HPP
