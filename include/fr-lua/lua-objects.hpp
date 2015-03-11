#ifndef LUA_OBJECTS_HPP
#define LUA_OBJECTS_HPP

#include <memory>
#include <sstream>
#include <vector>
#include <string>

extern "C" {
#include "lualib.h"
#include "lauxlib.h"
#include "lua.h"
}

#ifdef LUA_WRAPPER_TOP_NAMESPACE

namespace LUA_WRAPPER_TOP_NAMESPACE {

#endif

namespace lua { namespace objects {

    struct base {

        static const char *type2string( unsigned value )
        {
            switch( value ) {
            case TYPE_NONE:
                return "none";
            case TYPE_NIL:
                return "nil";
            case TYPE_BOOL:
                return "bool";
            case TYPE_LUSERDATA:
                return "lightuserdata";
            case TYPE_NUMBER:
                return "number";
            case TYPE_STRING:
                return "string";
            case TYPE_TABLE:
                return "table";
            case TYPE_FUNCTION:
                return "function";
            case TYPE_USERDATA:
                return "userdata";
            case TYPE_THREAD:
                return "thread";
            case TYPE_LOCAL_INDEX:
                return "localindex";
            case TYPE_PAIR:
                return "pair";
            case TYPE_INTEGER:
                return "integer";
#if LUA_VERSION_NUM >=503
            case TYPE_UINTEGER:
                return "uinteger";
#endif
            case TYPE_REFERENCE:
                return "reference";
            }
            return "unknown";
        }

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
#if LUA_VERSION_NUM >=503
            ,TYPE_UINTEGER      = TYPE_LOCAL_INDEX + 3
#endif
            ,TYPE_REFERENCE     = 0x80000000
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

        virtual const base * at( size_t /*index*/ ) const
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

        const bool none_or_nil( ) const
        {
            int ti = type_id( );
            return ( ti == LUA_TNONE ) || ( ti == LUA_TNIL );
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
            static const char * vals[2] = { "false", "true" };
            return vals[value_ ? 1 : 0];
        }

        lua_Number num( ) const
        {
            return static_cast<lua_Number>(value_ ? 1 : 0);
        }

    };

    class nil: public base {

    public:

        nil( )
        { }

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

#if LUA_VERSION_NUM >=503
    class uinteger: public base {

        lua_Unsigned num_;

    public:

        explicit uinteger( lua_Unsigned num )
            :num_(num)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_UINTEGER;
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
#endif

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

        size_t count( ) const
        {
            return 2;
        }

        void push( lua_State *L ) const
        {
            if( !pair_.first->none_or_nil( ) )  {
                pair_.first->push( L );
            }
            if( !pair_.second->none_or_nil( ) )  {
                pair_.second->push( L );
            }
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

        size_t nil_size( ) const
        {
            return  ( pair_.first->none_or_nil( )  ? 1 : 0 )
                  + ( pair_.second->none_or_nil( ) ? 1 : 0 );
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

        size_t count( ) const
        {
            return list_.size( );
        }

        const base * at( size_t index ) const
        {
            return list_[index].get( );
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

        table * add( const base_sptr &k, const base_sptr &v )
        {
            push_back( pair_sptr(new pair( k, v ) ) );
            return this;
        }

        table * add( base *k, base *v )
        {
            add( base_sptr(k), base_sptr(v) );
            return this;
        }

        table * add( base *k, const base_sptr &v )
        {
            add( base_sptr(k), v );
            return this;
        }

        table * add( const base_sptr &k, base *v )
        {
            add( k, base_sptr(v) );
            return this;
        }

        table * add( base_sptr v )
        {
            static std::shared_ptr<base> const_nil( new nil );
            push_back( pair_sptr( new pair( const_nil, v ) ) );
            return this;
        }

        table * add( base *v )
        {
            add( base_sptr(v) );
            return this;
        }

        table * add( const char *fld, base *v )
        {
            add( new string(fld), base_sptr(v) );
            return this;
        }

        table * add( const std::string fld, base *v )
        {
            add( new string(fld), base_sptr(v) );
            return this;
        }

        table * add( const char *fld, base_sptr v )
        {
            add( new string(fld), v );
            return this;
        }

        table * add( const std::string fld,  base_sptr v )
        {
            add( new string(fld), v );
            return this;
        }

        base *clone( ) const
        {
            return new table( *this );
        }

        void push( lua_State *L ) const
        {
            typedef pair_vector::const_iterator citr;
            lua_newtable( L );
            size_t len = 1;
            for( citr b(list_.begin( )), e(list_.end( )); b!=e; ++b, ++len ) {
                size_t n((*b)->nil_size( ));
                switch (n) {
                case 1:
                    lua_pushinteger( L, len );
                    (*b)->push( L );
                    lua_settable( L, -3 );
                    break;
                case 0:
                    (*b)->push( L );
                    lua_settable( L, -3 );
                    break;
                default: // nothing to do here
                    break;
                }
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

    class reference: public base {

        lua_State *state_;
        int ref_;
        int type_;

    public:

        int create_ref( lua_State *state, int index )
        {
            lua_pushvalue( state, index );
            int ref = luaL_ref( state, LUA_REGISTRYINDEX );

            return ref;
        }

        reference( lua_State *state, int index )
            :state_(state)
            ,ref_(create_ref( state, index ))
            ,type_(lua_type( state, index ) | base::TYPE_REFERENCE)
        { }

        reference( lua_State *state, int /*index*/, int ref )
            :state_(state)
            ,ref_(ref)
        { }

        ~reference( )
        {
            luaL_unref( state_, LUA_REGISTRYINDEX, ref_ );
        }

        virtual int type_id( ) const
        {
            return type_;
        }

        virtual base *clone( ) const
        {
            push( state_ );
            int new_ref = luaL_ref( state_, LUA_REGISTRYINDEX );
            return new reference( state_, 0, new_ref );
        }

        void push( lua_State *L ) const
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref_);
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << "ref[" << base::type2string( type_ & ~base::TYPE_REFERENCE )
                << "]@"
                << std::hex << state_ << ":" << ref_;
            return oss.str( );
        }
    };

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
            oss << "function@" << std::hex << func_;
            return oss.str( );
        }
    };

    class thread: public base {

        lua_State *p_;
        lua_State *s_;

    public:

        thread( lua_State *parent,  lua_State *s )
            :p_(parent)
            ,s_(s)
        { }

        virtual int type_id( ) const
        {
            return base::TYPE_THREAD;
        }

        virtual base *clone( ) const
        {
            return new thread( p_, s_ );
        }

        void push( lua_State *L ) const
        {
            if( L != p_ ) {
                throw std::logic_error( "Invalid stack for thread" );
            }
            lua_pushthread( s_ );
            lua_xmove( s_, L, 1 );
        }

        std::string str( ) const
        {
            std::ostringstream oss;
            oss << "thread@" << std::hex << s_;
            return oss.str( );
        }
    };

    inline pair * new_pair( base *k, base *v )
    {
        return new pair( base_sptr(k), base_sptr(v) );
    }

    inline pair * new_pair( const base_sptr &k, const base_sptr &v )
    {
        return new pair( k, v );
    }

    inline table * new_table(  )
    {
        return new table;
    }

    inline thread * new_thread( lua_State *parent, lua_State *child )
    {
        return new thread( parent, child );
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

    inline integer * new_integer( lua_Integer value )
    {
        return new integer( value );
    }

    inline boolean * new_boolean( bool value )
    {
        return new boolean( value );
    }

    inline number * new_number( lua_Number value )
    {
        return new number( value );
    }

    inline function * new_function( lua_CFunction func )
    {
        return new function( func );
    }

    inline reference * new_reference( lua_State *L, int id )
    {
        return new reference( L, id );
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
