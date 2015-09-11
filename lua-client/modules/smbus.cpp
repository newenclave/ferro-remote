#include <map>
#include <iostream>

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
    typedef siface::iface siface_type;

    using dev_sptr = std::shared_ptr<siface::iface>;
    using dev_map = std::map<size_t, dev_sptr>;

    template <typename T>
    using pair_vector8 = std::vector <std::pair<uint8_t, T> >;

    static const unsigned slave_invalid = siface::I2C_SLAVE_INVALID_ADDRESS;

    const std::string     module_name("smbus");
    const char *id_path     = FR_CLIENT_LUA_HIDE_TABLE ".smbus.__i";
    const char *meta_name   = FR_CLIENT_LUA_HIDE_TABLE ".smbus.meta";

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
    int lcall_bus_ioctl ( lua_State *L );

    int lcall_bus_read_byte( lua_State *L );
    int lcall_bus_read_word( lua_State *L );

    int lcall_bus_write_byte( lua_State *L );
    int lcall_bus_write_word( lua_State *L );

    int lcall_bus_read_block ( lua_State *L );
    int lcall_bus_write_block( lua_State *L );

    int lcall_bus_proc_call( lua_State *L );

    struct meta_object {
        utils::handle hdl_;
    };

    int lcall_meta_string( lua_State *L )
    {
        lua::state ls(L);
        void *ud = luaL_testudata( L, 1, meta_name );
        if( ud ) {
            std::ostringstream oss;
            oss << "smbus@"
                << std::hex
                << static_cast<meta_object *>(ud)->hdl_;
            ls.push( oss.str( ) );
        } else {
            ls.push( "Unknown object." );
        }
        return 1;
    }

    const struct luaL_Reg smbus_lib[ ] = {

         { "set_address",  &lcall_bus_set_addr  }
        ,{ "close",        &lcall_bus_close     }
        ,{ "functions",    &lcall_bus_functions }

        ,{ "read",         &lcall_bus_read      }
        ,{ "write",        &lcall_bus_write     }
        ,{ "ioctl",        &lcall_bus_ioctl     }

        ,{ "read_bytes",   &lcall_bus_read_byte  }
        ,{ "read_words",   &lcall_bus_read_word  }
        ,{ "read_block",   &lcall_bus_read_block }

        ,{ "write_bytes",  &lcall_bus_write_byte  }
        ,{ "write_words",  &lcall_bus_write_word  }
        ,{ "write_block",  &lcall_bus_write_block }
        ,{ "process_call", &lcall_bus_proc_call   }
        ,{ "__gc",         &lcall_bus_close       }
        ,{ "__tostring",   &lcall_meta_string     }

        ,{ nullptr,      nullptr }
    };

    struct names_codes_type {
        const char * name_;
        uint64_t     code_;
    };

#define CODE_NAME_VALUE( name ) { #name, siface::FUNC_##name }

    const names_codes_type funcs_names_codes[ ] = {
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

#define CTLCODE_NAME_VALUE( name ) { #name, siface::CODE_##name }

    const names_codes_type ioctl_names_codes[ ] = {
         CTLCODE_NAME_VALUE( I2C_RETRIES     )
        ,CTLCODE_NAME_VALUE( I2C_TIMEOUT     )
        ,CTLCODE_NAME_VALUE( I2C_SLAVE       )
        ,CTLCODE_NAME_VALUE( I2C_SLAVE_FORCE )
        ,CTLCODE_NAME_VALUE( I2C_TENBIT      )
        ,CTLCODE_NAME_VALUE( I2C_PEC         )
        ,{ nullptr, 0 }
    };

#undef CTLCODE_NAME_VALUE

    int lcall_register_smbus_meta( lua_State *L )
    {
        metatable mt( meta_name, smbus_lib );
        mt.push( L );
        return 1;
    }

    void register_meta_tables( lua_State *L )
    {
        lua::state ls(L);

        ls.push( lcall_register_smbus_meta );
        lua_call( L, 0, 0 );
    }

    struct module: public iface {

        client::general_info  &info_;
        dev_map                buses_;

        module( client::general_info &info )
            :info_(info)
        {
            register_meta_tables( info_.main_ );
        }

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
            lua::state ls( info_.main_ );
            ls.set( id_path, (void *)nullptr );
            buses_.clear( );
        }

        utils::handle new_bus( unsigned bus_id,
                               unsigned slave = slave_invalid,
                               bool slave_force = false)
        {
            size_t nh = next_handle( );
            dev_sptr r(siface::open( *info_.client_core_,
                                      bus_id, slave, slave_force ));
            buses_[nh] = r;
            return utils::to_handle( nh );
        }

        dev_sptr get_bus( utils::handle hdl )
        {
            auto f(buses_.find( utils::from_handle<size_t>(hdl) ));
            if( f == buses_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second;
        }

        meta_object *push_object( lua_State *L, utils::handle hdl )
        {
            void *ud = lua_newuserdata( L, sizeof(meta_object) );
            meta_object *nfo = static_cast<meta_object *>(ud);
            if( nfo ) {
                luaL_getmetatable( L, meta_name );
                lua_setmetatable(L, -2);
                nfo->hdl_ = hdl;
            }
            return nfo;
        }

        utils::handle get_object_hdl( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            if( ud ) {
                return static_cast<meta_object *>(ud)->hdl_;
            } else {
                return utils::handle( );
            }
        }

        meta_object *get_object( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            if( ud ) {
                return static_cast<meta_object *>(ud);
            } else {
                return nullptr;
            }
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        static
        objects::table_sptr codes_table( const names_codes_type *codes )
        {
            objects::table_sptr t(std::make_shared<objects::table>( ));

            const names_codes_type *p = codes;

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

            res->add( "read",         new_function( &lcall_bus_read ) );
            res->add( "write",        new_function( &lcall_bus_write ) );
            res->add( "ioctl",        new_function( &lcall_bus_ioctl ) );


            res->add( "read_bytes",   new_function( &lcall_bus_read_byte ) );
            res->add( "read_words",   new_function( &lcall_bus_read_word ) );
            res->add( "read_block",   new_function( &lcall_bus_read_block ) );

            res->add( "write_bytes",  new_function( &lcall_bus_write_byte ) );
            res->add( "write_words",  new_function( &lcall_bus_write_word ) );
            res->add( "write_block",  new_function( &lcall_bus_write_block ) );

            res->add( "process_call", new_function( &lcall_bus_proc_call ) );

            res->add( "fcodes",   codes_table( funcs_names_codes ) );
            res->add( "ctlcodes", codes_table( ioctl_names_codes ) );

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

            unsigned busid = ls.get_opt<unsigned>( 1, 0xFFFFFFFF );
            ls.push( siface::bus_available( *m->info_.client_core_, busid ) );

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
            m->push_object( L, nh );

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

            utils::handle hdl = m->get_object_hdl( L, 1 );
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
        if( !m ) {
            return 0;
        }

        lua::state ls(L);
        int n = ls.get_top( );
        for( int i=1; i<=n; i++ ) {
            utils::handle hdl = m->get_object_hdl( L, i );
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

            const names_codes_type *p = funcs_names_codes;

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

        utils::handle h = m->get_object_hdl( L, 1 );
        unsigned max = ls.get_opt<unsigned>( 2, 1025 );

        if( max > 1025 ) { // fkn mgk!
            max = 1025;
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

        utils::handle h = m->get_object_hdl( L, 1 );
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

    int lcall_bus_ioctl ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_object_hdl( L, 1 );
        unsigned   code = ls.get_opt<unsigned>( 2 );
        uint64_t   data = ls.get_opt<uint64_t>( 3 );

        try {
            m->get_bus( h )->ioctl( code, data );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_proc_call( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );

        utils::handle h = m->get_object_hdl( L, 1 );
        uint8_t    code = ls.get_opt<uint8_t>( 2 );

        if( ls.get_top( ) <= 2 ) {
            ls.push(  );
            ls.push( "Bad parameters." );
            return 2;
        }

        int t = ls.get_type( 3 );

        try {
            if( t == base::TYPE_NUMBER ) {
                unsigned data = ls.get_opt<unsigned>( 3 );
                uint16_t res  = m->get_bus( h )->process_call( code,
                                               static_cast<uint16_t>(data) );
                ls.push( res );
            } else if( t == base::TYPE_STRING ) {
                std::string data = ls.get_opt<std::string>( 3 );
                std::string res = m->get_bus( h )->process_call( code, data );
                ls.push( res );
            } else {
                ls.push(  );
                ls.push( "Bad parameter type." );
                return 2;
            }
            //size_t res = m->get_bus( h );
            ls.push( 0 );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

//////////////////// READ WRITE
///

    template <typename T>
    pair_vector8<T> create_cmd_params( lua_State *L, int id )
    {
        lua::state ls( L );
        typedef pair_vector8<T> pair_vector;

        pair_vector res;

        objects::base_sptr o = ls.get_object( id );

        for( size_t i=0; i<o->count( ); ++i ) {
            const objects::base * next = o->at( i );
            if( next->count( ) >= 2 ) {
                const objects::base * id  = next->at( 0 );
                const objects::base * val = next->at( 1 );
                //std::cout << id->num( ) << " = " << val->num( ) << "\n";
                res.push_back(
                    std::make_pair(
                        static_cast<uint8_t>( id->num( ) ),
                        static_cast<T>( val->num( ) ) ) );
            }
        }
        return res;
    }

    template <typename T>
    std::vector<T> create_cmd_from_table( lua_State *L, int id )
    {
        lua::state ls( L );

        std::vector<T> res;
        objects::base_sptr o = ls.get_object( id );

        for( size_t i=0; i<o->count( ); ++i ) {
            res.push_back( static_cast<T>( o->at( i )->at( 1 )->num( ) ) );
        }

        return res;
    }

    template <typename T, typename CallType, typename CallListType>
    int lcall_read_impl( CallType call, CallListType calllist, lua_State *L )
    {
        using pair_vector = pair_vector8<T>;
        using data_vector = std::vector<uint8_t>;

        lua::state ls( L );
        module * m = get_module( L );
        utils::handle h = m->get_object_hdl( L, 1 );

        int t = ls.get_type( 2 );

        try {
            if( t == base::TYPE_NUMBER ) {

                unsigned cmd = ls.get_opt<unsigned>( 2 );
                ls.push( (m->get_bus( h ).get( )->*call)( cmd ) );

            } else if( t == LUA_TTABLE ) {

                data_vector tab = create_cmd_from_table<uint8_t>( L, 2 );

                pair_vector res = (m->get_bus( h ).get( )->*calllist)( tab );
                objects::table nt;

                for( auto &v: res ) {
                    nt.add( new_integer( v.first ), new_integer( v.second ) );
                }

                nt.push( L );
            } else {
                ls.push( );
                ls.push( "Bad command type. Need 'number' or 'table'." );
                return 2;
            }
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    template <typename T, typename CallType, typename CallListType>
    int lcall_write_impl( CallType call, CallListType calllist, lua_State *L )
    {
        using pair_vector = pair_vector8<T>;

        lua::state ls( L );
        module * m = get_module( L );
        utils::handle h = m->get_object_hdl( L, 1 );

        int t = ls.get_type( 2 );

        try {
            if( LUA_TNUMBER == t ) {
                unsigned cmd  = ls.get_opt<unsigned>( 2 );
                unsigned data = ls.get_opt<unsigned>( 3 );
                (m->get_bus( h ).get( )->*call)( static_cast<uint8_t>( cmd ),
                                                 static_cast<T>( data ) );
                ls.push( true );
            } else if( LUA_TTABLE == t ) {
                pair_vector tab = create_cmd_params<T>( L, 2 );
                (m->get_bus( h ).get( )->*calllist)( tab );
                ls.push( true );
            } else {
                ls.push( );
                ls.push( "Bad command type. Need 'number' or 'table'." );
                return 2;
            }
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }


    int lcall_bus_read_byte( lua_State *L )
    {
        return lcall_read_impl<uint8_t>( &siface_type::read_byte,
                                         &siface_type::read_bytes, L );
    }

    int lcall_bus_read_word( lua_State *L )
    {
        return lcall_read_impl<uint16_t>( &siface_type::read_word,
                                          &siface_type::read_words, L );
    }

    int lcall_bus_write_byte( lua_State *L )
    {
        return lcall_write_impl<uint8_t>( &siface_type::write_byte,
                                          &siface_type::write_bytes, L );
    }

    int lcall_bus_write_word( lua_State *L )
    {
        return lcall_write_impl<uint16_t>( &siface_type::write_word,
                                           &siface_type::write_words, L );
    }


    int lcall_bus_read_block( lua_State *L )
    {
        lua::state ls( L );
        module * m = get_module( L );
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            unsigned cmd = ls.get_opt<unsigned>( 2 );
            ls.push( m->get_bus( h )->read_block( cmd ) );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_bus_write_block( lua_State *L )
    {
        lua::state ls( L );
        module * m = get_module( L );
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            unsigned cmd     = ls.get_opt<unsigned>( 2 );
            std::string data = ls.get_opt<std::string>( 3 );

            m->get_bus( h )->write_block( cmd, data );
            ls.push( true );

        } catch( const std::exception &ex ) {
            ls.push( false );
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

