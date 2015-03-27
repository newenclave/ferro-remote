#include <map>

#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/II2C.h"

#include "../utils.h"

namespace fr { namespace lua { namespace m { namespace smbus {

namespace {

    using namespace objects;
    namespace siface = fr::client::interfaces::i2c;

    typedef std::shared_ptr<siface::iface> smbus_sptr;
    typedef std::map<size_t, smbus_sptr>   dev_map;

    static const unsigned slave_invalid = siface::I2C_SLAVE_INVALID_ADDRESS;

    const std::string     module_name("smbus");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".smbus.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_bus_avail     ( lua_State *L );
    int lcall_bus_open      ( lua_State *L );
    int lcall_bus_set_addr  ( lua_State *L );
    int lcall_bus_close     ( lua_State *L );
    int lcall_bus_functions ( lua_State *L );

    int lcall_bus_read  ( lua_State *L );
    int lcall_bus_write ( lua_State *L );

#define CODE_NAME_VALUE( name ) { #name, siface::FUNC_##name }

    struct funcs_names_codes_type {
        const char * name_;
        uint64_t     code_;
    } const funcs_names_codes[ ] = {
         CODE_NAME_VALUE( I2C                    )
        ,CODE_NAME_VALUE( 10BIT_ADDR             )
        ,CODE_NAME_VALUE( PROTOCOL_MANGLING      )
        ,CODE_NAME_VALUE( SMBUS_PEC              )
        ,CODE_NAME_VALUE( NOSTART                )
        ,CODE_NAME_VALUE( SMBUS_BLOCK_PROC_CALL  )
        ,CODE_NAME_VALUE( SMBUS_QUICK            )
        ,CODE_NAME_VALUE( SMBUS_READ_BYTE        )
        ,CODE_NAME_VALUE( SMBUS_WRITE_BYTE       )
        ,CODE_NAME_VALUE( SMBUS_READ_BYTE_DATA   )
        ,CODE_NAME_VALUE( SMBUS_WRITE_BYTE_DATA  )
        ,CODE_NAME_VALUE( SMBUS_READ_WORD_DATA   )
        ,CODE_NAME_VALUE( SMBUS_WRITE_WORD_DATA  )
        ,CODE_NAME_VALUE( SMBUS_PROC_CALL        )
        ,CODE_NAME_VALUE( SMBUS_READ_BLOCK_DATA  )
        ,CODE_NAME_VALUE( SMBUS_WRITE_BLOCK_DATA )
        ,CODE_NAME_VALUE( SMBUS_READ_I2C_BLOCK   )
        ,CODE_NAME_VALUE( SMBUS_WRITE_I2C_BLOCK  )
        ,{ nullptr, 0 }
    };

#undef CODE_NAME_VALUE

    struct module: public iface {

        client::general_info  &info_;
        dev_map                buses_;

        module( client::general_info &info )
            :info_(info)
        { }

        size_t next_handle(  )
        {
            return info_.eventor_->next_index( );
        }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void reinit( )
        {

        }

        void deinit( )
        {
            buses_.clear( );
        }

        utils::handle new_bus( unsigned bus_id,
                               unsigned slave = slave_invalid,
                               bool slave_force = false)
        {
            size_t nh = next_handle( );
            smbus_sptr r(siface::open( *info_.client_core_,
                                       bus_id, slave, slave_force ));
            buses_[nh] = r;
            return utils::to_handle( nh );
        }

        smbus_sptr get_bus( utils::handle hdl )
        {
            auto f(buses_.find( utils::from_handle<size_t>(hdl) ));
            if( f == buses_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second;
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        static
        objects::table_sptr functions_table( )
        {
            objects::table_sptr t(std::make_shared<objects::table>( ));

            const funcs_names_codes_type *p = funcs_names_codes;

            do {
                t->add( p->name_, new_integer( p->code_ ) );
                t->add( new_integer( p->code_ ), new_string( p->name_ ) );
            } while( (++p)->name_ );

            return t;
        }

        objects::table_sptr table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "available",    new_function( &lcall_bus_avail ) );
            res->add( "open",         new_function( &lcall_bus_open ) );
            res->add( "set_address",  new_function( &lcall_bus_set_addr ) );
            res->add( "functions",    new_function( &lcall_bus_functions ) );
            res->add( "close",        new_function( &lcall_bus_close ) );

            res->add( "read",   new_function( &lcall_bus_read ) );
            res->add( "write",  new_function( &lcall_bus_write ) );

            res->add( "fcodes", functions_table( ) );

            return res;
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_bus_avail( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        try {
            ls.push( siface::bus_available(
                         *m->info_.client_core_,
                         ls.get_opt<unsigned>( 1, 0xFFFFFFFF )) );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_open ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

            unsigned busid = ls.get_opt<unsigned>( 1 );
            unsigned slave = ls.get_opt<unsigned>( 2, slave_invalid );
            bool     force = ls.get_opt<bool>( 3, false );

            utils::handle nh( m->new_bus( busid, slave, force ) );
            ls.push( nh );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    int lcall_bus_set_addr( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

            utils::handle hdl = ls.get_opt<utils::handle>( 1 );
            unsigned slave = ls.get_opt<unsigned>( 2, slave_invalid );

            m->get_bus ( hdl )->set_address( slave );
            ls.push( true );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_close( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        int n = ls.get_top( );
        for( int i=1; i<=n; i++ ) {
            utils::handle hdl = ls.get_opt<utils::handle>( i );
            m->buses_.erase( utils::from_handle<size_t>( hdl ) );
        }
        ls.push( true );
        return 1;
    }

    int lcall_bus_functions ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

            utils::handle hdl = ls.get_opt<utils::handle>( 1 );

            uint64_t val = m->get_bus( hdl )->function_mask( );
            objects::table res;
            res.add( "value", new_integer( val ) );

            const funcs_names_codes_type *p = funcs_names_codes;

            do {
                if( val & p->code_ ) {
                    res.add( new_string( p->name_ ) );
                }
            } while ( (++p)->name_ );

            res.push( L );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_read ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = ls.get_opt<utils::handle>( 1 );
        unsigned max = ls.get_opt<unsigned>( 2, 44000 );

        if( max > 44000 ) { // fkn mgk!
            max = 44000;
        }

        try {
            std::vector<char> data( max + 1 );
            size_t res = m->get_bus( h )->read( &data[0], max );
            ls.push( &data[0], res );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;

    }

    int lcall_bus_write ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = ls.get_opt<utils::handle>( 1 );
        std::string   d = ls.get_opt<std::string>( 2 );

        try {
            size_t res = m->get_bus( h )
                          ->write( d.empty( ) ? "" : &d[0], d.size( ) );
            ls.push( res );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

