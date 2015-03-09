#ifndef LUA_WRAPPER_HPP
#define LUA_WRAPPER_HPP

#include <stdexcept>

#include <stdlib.h>

extern "C" {
#include "lualib.h"
#include "lauxlib.h"
#include "lua.h"
}

#if( LUA_VERSION_NUM < 501 )
#error "Lua version is too old; Use 5.1 or higher"
#endif

#include "lua-type-wrapper.hpp"
#include "lua-objects.hpp"

#ifdef LUA_WRAPPER_TOP_NAMESPACE

namespace LUA_WRAPPER_TOP_NAMESPACE {

#endif

namespace lua {

    class state {
        lua_State *vm_;
        bool       own_;

        static void *def_alloc( void *ud, void *ptr,
                                size_t old_size, size_t new_size )
        {
            void *tmp = NULL;

            if ( old_size && new_size && ptr ) {
                if ( old_size < new_size ) {
                    tmp = realloc ( ptr, new_size );
                } else {
                    tmp = ptr;
                }
            } else if( new_size ) {
                tmp = malloc( new_size );
            } else if( new_size == 0 ) {
                free( ptr );
                tmp = NULL;
            }
            return tmp;
        }

        struct object_wrapper {
            const objects::base *obj_;
            object_wrapper( const objects::base *o )
                :obj_(o)
            { }
        };

        void push( object_wrapper value )
        {
            value.obj_->push( vm_ );
        }

    public:

        enum state_owning {
             NOT_OWN_STATE = 0
            ,OWN_STATE     = 1
        };

        state( lua_State *vm, state_owning os = NOT_OWN_STATE )
            :vm_(vm)
            ,own_(os == OWN_STATE)
        { }

        state( )
            :vm_(lua_newstate( def_alloc, 0 ))
            ,own_(true)
        { }

        ~state( )
        {
            if( own_ && vm_ ) {
                lua_close( vm_ );
            }
        }

        int openlib( const char *libname )
        {
            static const struct {
                std::string     name;
                lua_CFunction   func;
                int             results;
            } libs[ ] = {
                 { LUA_TABLIBNAME,  &luaopen_table,   1 }
                ,{ LUA_IOLIBNAME,   &luaopen_io,      1 }
                ,{ LUA_OSLIBNAME,   &luaopen_os,      1 }
                ,{ LUA_STRLIBNAME,  &luaopen_string,  1 }
                ,{ LUA_MATHLIBNAME, &luaopen_math,    1 }
                ,{ LUA_DBLIBNAME,   &luaopen_debug,   1 }
                ,{ LUA_LOADLIBNAME, &luaopen_package, 1 }
                ,{ "base",          &luaopen_base,    0 }
            };

            const size_t libs_count = sizeof( libs ) / sizeof( libs[0] );

            for( size_t i=0; i<libs_count; ++i ) {
                if( 0 == libs[i].name.compare( libname ) ) {

#if( LUA_VERSION_NUM < 502 )
                    push( libs[i].func );
                    lua_call( vm_, 0, 0 );
#else
                    push( libs[i].func );
                    lua_call( vm_, 0, libs[i].results );
                    if( libs[i].results > 0 ) {
                        lua_setglobal( vm_, libname );
                    }
#endif
                    return 1;
                }
            }
            return 0;
        }

        lua_State *get_state( )
        {
            return vm_;
        }

        const lua_State *get_state( ) const
        {
            return vm_;
        }

        void clean_stack( )
        {
            pop( get_top( ) );
        }

        void pop( )
        {
            lua_pop( vm_, 1 );
        }

        void pop( int n )
        {
            lua_pop( vm_, n );
        }

        std::string pop_error( )
        {
            const char *str = lua_tostring(vm_, -1);
            std::string res( str ? str : "Unknown error" );
            pop( );
            return res;
        }

        void register_call( const char *name, lua_CFunction fn )
        {
            lua_register( vm_, name, fn );
        }

        std::string error( )
        {
            std::string res( lua_tostring(vm_, -1) );
            return res;
        }

        void check_call_error( int res )
        {
            if( 0 != res ) {
                throw std::runtime_error( pop_error( ) );
            }
        }

        void push_value( int idx = -1 )
        {
            lua_pushvalue( vm_, idx );
        }

        void push( )
        {
            lua_pushnil( vm_ );
        }

        void push( bool value )
        {
            lua_pushboolean( vm_, value ? 1 : 0 );
        }

        void push( const char* value )
        {
            lua_pushstring( vm_, value );
        }

        void push( const char* value, size_t len )
        {
            lua_pushlstring( vm_, value, len );
        }

        void push( const std::string& value )
        {
            lua_pushlstring( vm_, value.c_str( ), value.size( ) );
        }

        void push( lua_CFunction value )
        {
            lua_pushcfunction( vm_, value );
        }

        template<typename T>
        void push( T * value )
        {
            lua_pushlightuserdata( vm_, reinterpret_cast<void *>( value ) );
        }

        template<typename T>
        void push( T value )
        {
            lua_pushnumber( vm_, static_cast<T>( value ) );
        }

        int get_type( int id = -1 )
        {
            return lua_type( vm_, id );
        }

        int get_top( )
        {
            return lua_gettop( vm_ );
        }

        bool none_or_nil( int id = -1 ) const
        {
            return lua_isnoneornil( vm_, -1 );
        }

        template<typename T>
        T get( int id = -1 )
        {
            typedef types::id_traits<T> traits;
            if( !traits::check( vm_, id ) ) {
                throw std::runtime_error( std::string("bad type '")
                        + types::id_to_string( traits::type_index )
                        + std::string("'. lua type is '")
                        + types::id_to_string( get_type( id ) )
                        + std::string("'") );
            }
            return traits::get( vm_, id );
        }

        template<typename T>
        T get_field( const char *key, int id = -1 )
        {
            T p = T( );
            lua_getfield( vm_, id, key );
            if( !none_or_nil(  ) ) {
                try {
                    p = get<T>( );
                } catch( ... ) {
                    pop( );
                    throw;
                }
            }
            pop( );
            return p;
        }

        objects::base_sptr get_table( int idx = -1, unsigned flags = 0 )
        {
            lua_pushvalue( vm_, idx );
            lua_pushnil( vm_ );

            objects::table_sptr new_table( objects::new_table( ) );

            while ( lua_next( vm_, -2 ) ) {
                lua_pushvalue( vm_, -2 );
                objects::pair_sptr new_pair
                        ( objects::new_pair( get_object( -1, flags ),
                                             get_object( -2, flags ) ) );
                new_table->push_back( new_pair );
                lua_pop( vm_, 2 );
            }

            lua_pop( vm_, 1 );
            return new_table;
        }

        objects::base_sptr get_object( int idx = -1, unsigned flags = 0 )
        {

            typedef objects::base_sptr base_sptr;

            int t = lua_type( vm_, idx );
            switch( t ) {
            case LUA_TBOOLEAN:
                return base_sptr(
                    new objects::boolean( lua_toboolean( vm_, idx ) ));
            case LUA_TLIGHTUSERDATA:
                return base_sptr(
                    new objects::light_userdata( lua_touserdata( vm_, idx ) ));
            case LUA_TNUMBER:
                return flags
                  ? base_sptr(new objects::integer( lua_tointeger( vm_, idx ) ))
                  : base_sptr(new objects::number( lua_tonumber( vm_, idx ) )) ;
            case LUA_TSTRING: {
                    size_t length = 0;
                    const char *ptr = lua_tolstring( vm_, idx, &length );
                    return base_sptr(new objects::string( ptr, length ));
                }
            case LUA_TFUNCTION:
                return base_sptr(
                    new objects::function( lua_tocfunction( vm_, idx ) ));
            case LUA_TTABLE:
                return get_table( idx, flags );
            case LUA_TTHREAD:
                return base_sptr(
                    new objects::thread( vm_, lua_tothread( vm_, idx ) ));

        //    case LUA_TUSERDATA:
        //        return "userdata";
            }
            return base_sptr(new objects::nil);
        }

    private:

        void get_global( const char *val )
        {
            lua_getglobal( vm_, val );
        }

        void set_global( const char *val )
        {
            lua_setglobal( vm_, val );
        }

        void set_table( int id = -3 )
        {
            lua_settable( vm_, id );
        }

        static size_t path_root( const char *path )
        {
            const char *p = path;
            for( ; *p && (*p != '.'); ++p );
            return p - path;
        }

        static const char *path_leaf( const char *path )
        {
            const char *p  = path;
            const char *sp = path;
            for( ; *p; ++p ) {
                if( *p == '.' ) {
                    sp = p;
                }
            }
            return sp;
        }

        template <typename T>
        void create_or_push( const char *path, T value )
        {
            if( *path ) {

                std::string p( path, path_root( path ) );
                const char *tail = path + p.size( );

                lua_newtable( vm_ );
                push( p.c_str( ) );
                create_or_push( !*tail ? "" : tail + 1, value );
                set_table( );
            } else {
                push( value );
            }
        }

        template <typename T>
        void set_to_stack( const char *path, T value )
        {
            std::string p( path, path_root( path ) );
            const char *tail = path + p.size( );

            if( *tail ) {

                lua_getfield( vm_, -1, p.c_str( ) );

                if( !lua_istable( vm_, -1 ) ) {
                    pop( 1 );
                    push( p.c_str( ) );
                    create_or_push( tail + 1, value );
                    set_table( );
                } else {
                    set_to_stack( tail + 1, value );
                    pop( 1 );
                }
            } else {
                push( p.c_str( ) );
                push( value );
                set_table( );
            }
        }

    public:

        template <typename T>
        void set( const char *path, T value )
        {
            std::string p( path, path_root( path ) );
            const char *tail = path + p.size( );

            get_global( p.c_str( ) );

            if( !lua_istable( vm_, -1 ) ) {
                pop( 1 );
                if( !*tail ) {
                    push( value );
                } else {
                    create_or_push( tail + 1, value );
                }
            } else {
                if( !*tail ) {
                    pop( 1 );
                    push( value );
                } else {
                    set_to_stack( tail + 1, value );
                }
            }
            set_global( p.c_str( ) );
        }

        void set( const char *path )
        {
            static const objects::nil nil_value;
            set_object( path, &nil_value );
        }

        void set_object( const char *path, const objects::base *obj )
        {
            set<object_wrapper>( path, object_wrapper( obj ) );
        }

        int get_table( const char *path )
        {
            int level = 1;
            std::string r(path, path_root( path ));
            const char *tail = path + r.size( );

            lua_getglobal( vm_, r.c_str( ) );

            while( 1 ) {
                if( !lua_istable( vm_, -1 ) ) {
                    lua_pop( vm_, level );
                    level = 0;
                    break;
                } else {
                    if( *tail ) {
                        tail++;
                        level++;
                        r.assign( tail, path_root( tail ) );
                        tail = tail + r.size( );
                        lua_getfield( vm_, -1, r.c_str( ) );
                    } else {
                        break;
                    }
                }
            }
            return level;
        }

        template <typename T>
        T get( const char *path )
        {
            const char *pl = path_leaf( path );
            if( pl == path ) {
                get_global( pl );
                T val = T( );
                if( !none_or_nil(  ) ) try {
                    val = get<T>( );
                    pop( );
                } catch( ... ) {
                    pop( );
                    throw;
                }
                return val;
            } else {
                std::string tpath( path, (pl - path) );
                int level = get_table( tpath.c_str( ) );
                if( level ) {
                    T val = T( );
                    try {
                        val = get_field<T>( pl + 1 );
                        pop( level );
                    } catch( ... ) {
                        pop( level );
                        throw;
                    }
                    return val;
                }
            }
            return T( );
        }

        bool exists( const char *path )
        {
            const char *pl = path_leaf( path );
            bool res = false;
            if( pl == path ) {
                get_global( pl );
                if( !none_or_nil(  ) ) {
                    res = true;
                    pop( );
                }
            } else {
                std::string tpath( path, (pl - path) );
                int level = get_table( tpath.c_str( ) );
                if( level ) {
                    lua_getfield( vm_, -1, pl + 1 );
                    if( !none_or_nil(  ) ) {
                        res = true;
                        pop( );
                    }
                    pop( level );
                }
            }
            return res;
        }

        int exec_function( const char* func )
        {
            lua_getglobal( vm_, func );
            int rc = lua_pcall( vm_, 0, LUA_MULTRET, 0 );
            return rc;
        }

        int exec_function( const char* func, const objects::base &bo )
        {
            lua_getglobal( vm_, func );
            bo.push( vm_ );
            int rc = lua_pcall( vm_, 1, LUA_MULTRET, 0 );
            return rc;
        }

        void push_object_list( const std::vector<objects::base_sptr> &bo )
        {
            typedef std::vector<objects::base_sptr>::const_iterator citr;
            for( citr b(bo.begin( )), e(bo.end( )); b!=e; ++b ) {
                (*b)->push( vm_ );
            }
        }

        void set_value( const char *path, int idx = -1 )
        {
            //// crutch ... WILL FIX IT LATER
            set( path, 0 );
            ////////////////////

            const char *pl = path_leaf( path );

            if( pl == path ) {

                push_value( idx );
                set_global( pl );

            } else {

                std::string tpath( path, (pl - path) );

                int level = get_table( tpath.c_str( ) ); // set table level

                if( level ) {

                    lua_getfield( vm_, -1, pl + 1 );
                    pop( );             // old value
                    push( pl + 1 );     // name
                    push_value( idx );  // value by index
                    set_table( );
                    pop( level );       // clean table level
                }

            }
        }

        lua_State *create_thread( const char *path )
        {
            lua_State *res = lua_newthread( vm_ ); // push new value 'state'
            int index = get_top( );                // table index in parameters
            set_value( path, index );              // copy here
            pop( 1 );                              // pop state from stack
            return res;
#if 0
            /// COPY-PASTE-COPY-PASTE-COPY-PASTE
            //// crutch ... WILL FIX IT LATER
            set( path, 0 );
            ////////////////////

            const char *pl = path_leaf( path );

            lua_State *res = NULL;

            if( pl == path ) {

                res = lua_newthread( vm_ );
                set_global( pl );

            } else {

                std::string tpath( path, (pl - path) );

                int level = get_table( tpath.c_str( ) );

                if( level ) {
                    lua_getfield( vm_, -1, pl + 1 );
                    pop( );
                    push( pl + 1 );
                    res = lua_newthread( vm_ );
                    set_table( );
                    pop( level );
                }
            }
            return res;
#endif
        }

        int exec_function( const char* func,
                           const std::vector<objects::base_sptr> &bo )
        {
            lua_getglobal( vm_, func );
            push_object_list( bo );
            int rc = lua_pcall( vm_, bo.size( ), LUA_MULTRET, 0 );
            return rc;
        }

        template <typename P0>
        int exec_function( const char* func, P0 p0 )
        {

            lua_getglobal( vm_, func );
            push( p0 );
            int rc = lua_pcall( vm_, 1, LUA_MULTRET, 0 );
            return rc;
        }

        template <typename P0>
        int exec_function( const char* func, P0 p0,
                           const std::vector<objects::base_sptr> &bo )
        {

            lua_getglobal( vm_, func );
            push( p0 );
            push_object_list( bo );
            int rc = lua_pcall( vm_, 1 + bo.size( ), LUA_MULTRET, 0 );
            return rc;
        }

        template <typename P0, typename P1>
        int exec_function( const char* func, P0 p0, P1 p1 )
        {
            lua_getglobal( vm_, func );
            push( p0 );
            push( p1 );
            int rc = lua_pcall( vm_, 2, LUA_MULTRET, 0 );
            return rc;
        }

        template <typename P0, typename P1>
        int exec_function( const char* func, P0 p0, P1 p1,
                           const std::vector<objects::base_sptr> &bo )
        {
            lua_getglobal( vm_, func );
            push( p0 );
            push( p1 );
            push_object_list( bo );
            int rc = lua_pcall( vm_, 2 + bo.size( ), LUA_MULTRET, 0 );
            return rc;
        }

        int load_file( const char *path )
        {
            int res = luaL_loadfile( vm_, path );
            if( 0 == res ) {
                res = lua_pcall( vm_, 0, LUA_MULTRET, 0);
            }
            return res;
        }

        int load_buffer( const char *buf, size_t length,
                         const char *name = NULL )
        {
            int res = luaL_loadbuffer ( vm_, buf, length, name ? name : "" );
            if( 0 == res ) {
                res = lua_pcall( vm_, 0, LUA_MULTRET, 0);
            }
            return res;
        }
    };
    typedef std::shared_ptr<state> state_sptr;
}

#ifdef LUA_WRAPPER_TOP_NAMESPACE

} // LUA_WRAPPER_TOP_NAMESPACE

#endif


#endif // LUAWRAPPER_HPP
