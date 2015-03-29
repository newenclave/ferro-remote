#include <iostream>

#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/IGPIO.h"
#include "../utils.h"

#include "../event-container.h"

namespace fr { namespace lua { namespace m { namespace gpio {

namespace {

    using namespace objects;
    namespace giface = fr::client::interfaces::gpio;
    typedef   giface::iface siface_type;

    typedef std::shared_ptr<giface::iface> dev_sptr;

    struct dev_info {
        dev_sptr dev_;
        bool edge_support_;
        dev_info( )
            :edge_support_(false)
        { }
    };

    typedef std::shared_ptr<dev_info>  dev_info_sptr;
    typedef std::map<size_t, dev_info> dev_map;

    typedef std::shared_ptr<lua::event_container>  eventor_sptr;
    typedef std::weak_ptr<lua::event_container>    eventor_wptr;


    const std::string     module_name("gpio");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".gpio.__i";

    struct module;

    struct name_code_type {
        const char *name_;
        unsigned    code_;
    };

    const name_code_type edges[ ] = {
         { "none",      giface::EDGE_NONE }
        ,{ "rising",    giface::EDGE_RISING }
        ,{ "falling",   giface::EDGE_FALLING }
        ,{ "both",      giface::EDGE_BOTH }
        ,{ nullptr,     (unsigned)(-1) } // -1 is never used
    };

    const name_code_type dirs[ ] = {
         { "in",    giface::DIRECT_IN }
        ,{ "out",   giface::DIRECT_OUT }
        ,{ nullptr, giface::EDGE_NONE }
    };

    const char * val2str( const name_code_type *p, unsigned val,
                          const char *def )
    {
        do {
            if( p->code_ == val ) {
                return p->name_;
            }
        } while( (++p)->name_ );
        return def;
    }

    unsigned str2val( const name_code_type *p, const std::string &str,
                      unsigned def )
    {
        do {
            if( !str.compare( p->name_ ) ) {
                return p->code_;
            }
        } while( (++p)->name_ );
        return def;
    }

    giface::direction_type str2dir( const std::string &val )
    {
        return giface::direction_val2enum(
                    str2val( dirs, val, giface::DIRECT_NONE ) );
    }

    giface::edge_type str2edge( const std::string &val )
    {
        return giface::edge_val2enum(
                    str2val( edges, val, giface::EDGE_NONE ) );
    }

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_export   ( lua_State *L );
    int lcall_info     ( lua_State *L );
    int lcall_set      ( lua_State *L );
    int lcall_get      ( lua_State *L );
    int lcall_close    ( lua_State *L );
    int lcall_unexport ( lua_State *L );

    int lcall_file_events   ( lua_State *L );
    int lcall_file_subscribe( lua_State *L );

    std::vector<std::string> events_names( )
    {
        std::vector<std::string> res;

        res.push_back( "on_changed" );

        return res;
    }


    struct module: public iface {

        client::general_info           &info_;
        bool                            available_;
        dev_map                         devs_;
        std::map<size_t, eventor_sptr>  events_;
        std::vector<std::string>        events_name_;

        module( client::general_info &info )
            :info_(info)
            ,events_name_(events_names( ))
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void reinit( )
        {
            available_ = giface::available( *info_.client_core_ );
        }

        void deinit( )
        {
            devs_.clear( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "available", new_boolean( available_ ) );

            if( available_ ) {
                res->add( "export",     new_function( &lcall_export ) );
                res->add( "unexport",   new_function( &lcall_unexport ) );
                res->add( "info",       new_function( &lcall_info ) );
                res->add( "set",        new_function( &lcall_set ) );
                res->add( "get",        new_function( &lcall_get ) );
                res->add( "close",      new_function( &lcall_close ) );

                res->add( "events",    new_function( &lcall_file_events ) );
                res->add( "subscribe", new_function( &lcall_file_subscribe ) );
            }

            return res;
        }


        utils::handle new_dev( unsigned id, unsigned dir, unsigned val )
        {
            size_t nh = info_.eventor_->next_index( );

            dev_info nd;
            switch (dir) {
            case giface::DIRECT_IN:
                nd.dev_.reset( giface::create_input( *info_.client_core_,
                                                      id ) );
                break;
            case giface::DIRECT_OUT:
                nd.dev_.reset( giface::create_output( *info_.client_core_,
                                                       id, val ) );
                break;
            default:
                nd.dev_.reset( giface::create( *info_.client_core_, id ) );
                break;
            }
            nd.edge_support_ = nd.dev_->edge_supported( );
            devs_[nh] = nd;
            events_[nh] = new_eventor( );
            return utils::to_handle(nh);
        }

        dev_sptr get_dev( utils::handle hdl )
        {
            auto f( devs_.find( utils::from_handle<size_t>(hdl) ) );
            if( f == devs_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second.dev_;
        }

        eventor_sptr new_eventor( )
        {
            eventor_sptr res = std::make_shared<
                                    lua::event_container
                               >( info_, events_name_ );
            return res;
        }

        eventor_sptr get_eventor( utils::handle hdl )
        {
            static const eventor_sptr empty;
            auto f(events_.find( utils::from_handle<size_t>(hdl) ));
            return (f != events_.end( )) ? f->second : empty;
        }

        dev_info get_dev_info( utils::handle hdl )
        {
            auto f( devs_.find( utils::from_handle<size_t>(hdl) ) );
            if( f == devs_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second;
        }

        void gpio_event( unsigned err, unsigned val, uint64_t interval,
                         eventor_wptr evtr )
        {
            eventor_sptr le(evtr.lock( ));
            if( le ) {
                FR_LUA_EVENT_PROLOGUE( "on_changed", *le );
                result->add( "interval",  new_integer( interval ) );
                if( err ) {
                    result->add( "error", new_integer( err ) );
                } else {
                    result->add( "value", new_integer( val ) );
                }
                FR_LUA_EVENT_EPILOGUE;
            }
        }

        void register_event( dev_sptr f, eventor_sptr e )
        {
            f->register_for_change_int(
                        std::bind( &module::gpio_event, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3,
                                        eventor_wptr(e) ) );
        }

        void unregister_event( dev_sptr f )
        {
            f->unregister( );
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_export( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {

            unsigned id  = ls.get_opt<unsigned>( 1 );
            unsigned dir = giface::DIRECT_IN; // 2
            unsigned val = ls.get_opt<unsigned>( 3 );;

            int n = ls.get_top( );
            int t = ls.get_type( 2 );

            if( n > 1 ) {
                if( t == base::TYPE_NUMBER ) {
                    dir = ls.get_opt<unsigned>( 2 );
                } else if( t == base::TYPE_STRING ) {
                    dir = str2dir(ls.get_opt<std::string>(2));
                }
            }

            ls.push( m->new_dev( id, dir, val ) );

        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    void set_value( dev_info &dev, lua::state &ls, int id )
    {
        unsigned val = ls.get_opt<unsigned>( id, 0 );
        dev.dev_->set_value( val );
    }

    void set_al( dev_info &dev, lua::state &ls, int id )
    {
        bool val = ls.get_opt<bool>( id, false );
        dev.dev_->set_active_low( val );
    }

    void set_edge( dev_info &dev, lua::state &ls, int id )
    {
        if( !dev.edge_support_ ) {
            return;
        }
        switch( ls.get_type( id ) ) {
        case base::TYPE_STRING:
            dev.dev_->set_edge( str2edge( ls.get_opt<std::string>( id ) ) );
            break;
        case base::TYPE_NUMBER:
        case base::TYPE_NONE:
            dev.dev_->set_edge( giface::edge_val2enum(
                             ls.get_opt<unsigned>( id, giface::EDGE_NONE ) ) );
            break;
        default:
            throw std::runtime_error( "Bad value for 'edge'." );
        }
    }

    void set_dir( dev_info &dev, lua::state &ls, int id )
    {
        switch( ls.get_type( id ) ) {
        case base::TYPE_STRING:
            dev.dev_->set_direction( str2dir( ls.get_opt<std::string>( id ) ) );
        case base::TYPE_NUMBER:
        case base::TYPE_NONE:
            dev.dev_->set_direction( giface::direction_val2enum(
                             ls.get_opt<unsigned>( id, giface::DIRECT_IN ) ) );
        default:
            throw std::runtime_error( "Bad value for 'direction'." );
        }
    }

    void set_from_string( dev_info &dev, lua::state &ls, int id )
    {
        std::string name = ls.get_opt<std::string>( id );
        if( !name.compare( "edge" ) ) {
            set_edge( dev, ls, id + 1 );
        } else if( !name.compare( "direction" ) ) {
            set_dir( dev, ls, id + 1 );
        } else if( !name.compare( "active_low" ) ) {
            set_al( dev, ls, id + 1 );
        } else if( !name.compare( "value" ) ) {
            set_value( dev, ls, id + 1 );
        } else {
            throw std::runtime_error( std::string( "Bad param name " ) + name );
        }
    }

    int lcall_set( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        int n = ls.get_top( );

        if( n < 2 ) {
            ls.push( false );
            return 1;
        }

        try {
            int ft = ls.get_type( 2 );
            auto dev = m->get_dev_info( h );
            switch( ft ) {
            case base::TYPE_STRING:
                set_from_string( dev, ls, 2 );
                ls.push( true );
                break;
            case base::TYPE_NUMBER:
                dev.dev_->set_value( ls.get_opt<unsigned>( 2 ) );
                ls.push( true );
                break;
            case base::TYPE_TABLE:
            default:
                ls.push( false );
                ;;;
            }
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    void get_push_from_string( dev_info &dev, lua::state &ls, int id )
    {
        std::string name = ls.get_opt<std::string>( id );
        if( !name.compare( "edge" ) ) {
            if( dev.edge_support_ ) {
                ls.push( val2str( edges, dev.dev_->edge( ), "none" ) );
            } else {
                ls.push( "not supported" );
            }
        } else if( !name.compare( "direction" ) ) {
            ls.push( val2str( dirs, dev.dev_->direction( ), "unknown" ) );
        } else if( !name.compare( "active_low" ) ) {
            ls.push( !!dev.dev_->active_low( ) );
        } else if( !name.compare( "value" ) ) {
            ls.push( dev.dev_->value( ) );
        } else {
            throw std::runtime_error( std::string( "Bad param name " ) + name );
        }
    }

    int lcall_get( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        try {
            int ft = ls.get_type( 2 );
            auto dev = m->get_dev_info( h );
            switch( ft ) {
            case base::TYPE_STRING:
                get_push_from_string( dev, ls, 2 );
                break;
            case base::TYPE_NONE:
                ls.push( dev.dev_->value( ) );
                break;
            default:
                ls.push( );
                ls.push( "Nothing to do." );
                return 2;
            }
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_info( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        try {
            auto dev = m->get_dev_info( h );
            giface::info i = dev.dev_->get_info( );

            objects::table res;
            res.add( "id",          new_integer( i.id ) );
            res.add( "value",       new_integer( i.value ) );
            res.add( "active_low",  new_boolean( !!i.active_low ) );

            res.add( "direction", new_string(
                                    val2str( dirs, i.direction, "unknown" ) ) );
            if( dev.edge_support_ ) {
                res.add( "edge", new_string(
                                    val2str( edges, i.edge, "unknown" ) ) );
            }
            res.push( L );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_close( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = ls.get_opt<utils::handle>( 1 );
        m->devs_.erase( utils::from_handle<size_t>( h ) );
        ls.push( true );
        return 1;
    }

    int lcall_unexport( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        try {
            m->get_dev( h )->unexport_device( );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push(  );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_file_events ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        auto f  = m->get_eventor( h );

        if( !f ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        return f->push_state( L );
    }

    int lcall_file_subscribe( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );
        utils::handle h = ls.get_opt<utils::handle>( 1 );

        auto e = m->get_eventor( h );

        if( !e ) {
            ls.push( );
            ls.push( "Bad handle." );
            return 2;
        }

        auto di = m->get_dev_info( h );

        int n = ls.get_top( );

        if( n > 2 ) {
            m->register_event( di.dev_, e );
        } else {
            m->unregister_event( di.dev_ );
        }

        return e->subscribe( L, 1 );
    }


}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

