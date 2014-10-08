#ifndef LUA_OBJECTS_HPP
#define LUA_OBJECTS_HPP

#include <memory>
#include <sstream>
#include <vector>
#include <string>

extern "C" {
#include "lua.h"
}

#ifdef LUA_WRAPPER_TOP_NAMESPACE

namespace LUA_WRAPPER_TOP_NAMESPACE {

#endif

namespace lua { namespace objects {

    struct base {

        enum {
             TYPE_NONE          = LUA_TNONE
            ,TYPE_NIL           = LUA_TNIL
            ,TYPE_BOOL          = LUA_TBOOLEAN
            ,TYPE_LUSERDATA     = LUA_TLIGHTUSERDATA
            ,TYPE_NUMBER        = LUA_TNUMBER
            ,TYPE_STRING        = LUA_TSTRING
            ,TYPE_TABLE         = LUA_TTABLE
            ,TYPE_FUNCTION      = LUA_TFUNCTION
            ,TYPE_USERDATA      = LUA_TUSERDATA
            ,TYPE_THREAD        = LUA_TTHREAD
            ,TYPE_LOCAL_INDEX   = 1000
            ,TYPE_PAIR          = TYPE_LOCAL_INDEX + 1
            ,TYPE_INTEGER       = TYPE_LOCAL_INDEX + 2
        };

        virtual ~base( ) { }

        virtual int type_id( ) const
        {
            return TYPE_NONE;
        }

        virtual bool is_container( ) const
        {
            return false;
        }

        virtual base * clone( ) const
        {
            return new base;
        }

        virtual size_t count( ) const
        {
            return 0;
        }

        virtual void push( lua_State *L ) const
        {
            throw std::runtime_error( "push for 'none' is not available" );
        }

        const base * at( size_t /*index*/ ) const
        {
            throw std::out_of_range( "bad index" );
        }

        virtual std::string str( ) const
        {
            return std::string( );
        }

        virtual lua_Number num( ) const
        {
            return 0;
        }

    };

    typedef std::shared_ptr<base> base_sptr;
    typedef std::unique_ptr<base> base_uptr;

    class boolean: public base {

        bool value_;

    public:

        boolean( bool val )
            :value_(val)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_BOOL;
        }

        virtual base *clone( ) const
        {
            return new boolean( value_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushboolean( L, value_ ? 1 : 0 );
        }

        std::string str( ) const
        {
            static const char * vals[2] = { "0", "1" };
            return vals[value_ ? 1 : 0];
        }

        lua_Number num( ) const
        {
            return static_cast<lua_Number>(value_ ? 1 : 0);
        }

    };

    class nil: public base {

    public:
        virtual int type_id( ) const
        {
            return base::TYPE_NIL;
        }

        virtual base *clone( ) const
        {
            return new nil;
        }

        void push( lua_State *L ) const
        {
            lua_pushnil( L );
        }

        std::string str( ) const
        {
            return "nil";
        }
    };

    class light_userdata: public base {

        void *ptr_;

    public:

        light_userdata( void *ptr )
            :ptr_(ptr)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_LUSERDATA;
        }

        virtual base *clone( ) const
        {
            return new light_userdata( ptr_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushlightuserdata( L, ptr_ );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << std::hex << ptr_;
            return oss.str( );
        }
    };

    class number: public base {

        lua_Number num_;

    public:

        explicit number( lua_Number num )
            :num_(num)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_NUMBER;
        }

        virtual base *clone( ) const
        {
            return new number( num_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushnumber( L, num_ );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << num_;
            return oss.str( );
        }

        lua_Number num( ) const
        {
            return num_;
        }

    };

    class integer: public base {

        lua_Integer num_;

    public:

        explicit integer( lua_Integer num )
            :num_(num)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_INTEGER;
        }

        virtual base *clone( ) const
        {
            return new integer( num_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushinteger( L, num_ );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << num_;
            return oss.str( );
        }

        lua_Number num( ) const
        {
            return static_cast<lua_Number>( num_ );
        }

    };

    class string: public base {

        std::string cont_;

    public:

        string( const std::string &cont )
            :cont_(cont)
        { }

        string( const char * cont )
            :cont_(cont)
        { }

        string( const char * cont, size_t len )
            :cont_(cont, len)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_STRING;
        }

        virtual base *clone( ) const
        {
            return new string( cont_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushstring( L, cont_.c_str( ) );
        }

        virtual size_t count( ) const
        {
            return cont_.size( );
        }

        std::string str( ) const
        {
            return cont_ ;
        }

        lua_Number num( ) const
        {
            return atof(cont_.c_str( ));
        }

    };

    class pair: public base {

        std::pair<base_sptr, base_sptr> pair_;

    public:

        virtual int type_id( ) const
        {
            return base::TYPE_PAIR;
        }

        pair( const base_sptr f, const base_sptr s )
            :pair_(std::make_pair(f, s))
        { }

        pair( const base *f, const base *s )
        {
            pair_.first.reset( f->clone( ) );
            pair_.second.reset( s->clone( ) );
        }

        pair( const pair &other )
        {
            pair_.first.reset( other.pair_.first->clone( ) );
            pair_.second.reset( other.pair_.second->clone( ) );
        }

        ~pair( )
        { }

        virtual bool is_container( ) const
        {
            return true;
        }

        virtual base *clone( ) const
        {
            return new pair( *this );
        }

        void push( lua_State *L ) const
        {
            pair_.first->push( L );
            pair_.second->push( L );
        }

        const base * at( size_t index ) const
        {
            if( index < 2 ) {
                return index ? pair_.second.get( ) : pair_.first.get( );
            }
            throw std::out_of_range( "bad index" );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << pair_.first->str( )
                << "="
                << pair_.second->str( );

            return oss.str( );
        }
    };

    typedef std::shared_ptr<pair> pair_sptr;

    class table: public base {


        typedef std::vector<pair_sptr> pair_vector;
        pair_vector list_;

    public:

        table( const table &o )
            :list_(o.list_)
        { }

        table( )
        { }

        int type_id( ) const
        {
            return base::TYPE_TABLE;
        }

        bool is_container( ) const
        {
            return true;
        }

        const base * at( size_t index ) const
        {
            return list_.at(index).get( );
        }

        void push_back( const pair_sptr &val )
        {
            list_.push_back( val );
        }

        table * add( pair *p )
        {
            push_back( pair_sptr( p ) );
            return this;
        }

        table * add( base *k, base *v )
        {
            push_back( pair_sptr(new pair( base_sptr(k), base_sptr(v) ) ) );
            return this;
        }

        virtual base *clone( ) const
        {
            return new table( *this );
        }

        void push( lua_State *L ) const
        {
            typedef pair_vector::const_iterator citr;
            lua_newtable( L );
            for( citr b(list_.begin( )), e(list_.end( )); b!=e; ++b ) {
                (*b)->push( L );
                lua_settable(L, -3);
            }
        }

        std::string str( size_t shift, const pair &pair ) const
        {
            std::string spaces( shift << 2, ' ' );
            std::ostringstream oss;
            oss << spaces << pair.str( );
            return oss.str( );
        }

        std::string str( ) const
        {
            std::ostringstream oss;

            typedef pair_vector::const_iterator citer;

            oss << "{ ";
            bool fst = true;
            for( citer b(list_.begin( )), e(list_.end( )); b!=e; ++b  ) {
                std::string res(str( 0, *b->get( ) ));
                if( !fst ) {
                    oss << ", ";
                } else {
                    fst = false;
                }
                oss << res;
            }
            oss << " }";

            return oss.str( );
        }
    };

    typedef std::shared_ptr<table> table_sptr;

    class function: public base {

        lua_CFunction func_;

    public:

        function( lua_CFunction func )
            :func_(func)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_FUNCTION;
        }

        virtual base *clone( ) const
        {
            return new function( func_ );
        }

        void push( lua_State *L ) const
        {
            lua_pushcfunction( L, func_ );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << "call@" << reinterpret_cast<const void *>(func_);
            return oss.str( );
        }
    };

    inline pair * new_pair( base *k, base *v )
    {
        return new pair( base_sptr(k), base_sptr(v) );
    }

    inline table * new_table(  )
    {
        return new table;
    }

    inline string * new_string( const char *str )
    {
        return new string( str );
    }

    inline string * new_string( const char *str, size_t length )
    {
        return new string( str, length );
    }

    inline string * new_string( const std::string &str )
    {
        return new string( str );
    }

    inline function * new_function( lua_CFunction func )
    {
        return new function( func );
    }

    inline light_userdata * new_light_userdata( void *data )
    {
        return new light_userdata( data );
    }

}}

#ifdef LUA_WRAPPER_TOP_NAMESPACE
}
#endif


#endif // LUAOBJECTS_HPP
