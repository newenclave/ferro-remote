
#include <stdlib.h>

#include "client-core/fr-client.h"

#include "vtrc-client/vtrc-client.h"
#include "vtrc-mutex.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "protocol/ferro.pb.h"

//#include "ferro-remote-config.h"

//#if FR_WITH_LUA
//#include "fr-lua/lua-wrapper.hpp"
//#endif

namespace fr {  namespace client { namespace core {

    namespace vclient = vtrc::client;
    namespace vcommon = vtrc::common;

    namespace {

        enum connection_name { CONN_NONE, CONN_PIPE, CONN_TCP };

        struct connect_info {
            connection_name     name_;
            std::string         server_;
            unsigned short      port_;
            bool                tcp_nowait_;
        };

        typedef std::map<size_t, async_op_callback_type> cb_map;

        typedef vtrc::shared_ptr<connect_info> connect_info_sptr;

        connect_info_sptr get_connect_info( std::string const &name, bool nw )
        {
            size_t delim_pos = name.find_last_of( ':' );

            connect_info_sptr ci(vtrc::make_shared<connect_info>( ));

            ci->name_ = CONN_NONE;

            if( delim_pos == std::string::npos ) {

                /// local: <localname>
                ci->name_ = CONN_PIPE;
                ci->server_ = name;

            } else {

                ci->name_ = CONN_TCP;
                ci->server_.assign( name.begin( ),
                                   name.begin( ) + delim_pos );
                ci->port_ = atoi( name.c_str( ) + delim_pos + 1 );
                ci->tcp_nowait_ = nw;

            }
            return ci;
        }
    }

    struct callbacks_info {
        cb_map              acb_;
        vtrc::shared_mutex  acb_lock_;
    };

    typedef vtrc::weak_ptr<callbacks_info>   callbacks_info_wptr;
    typedef vtrc::shared_ptr<callbacks_info> callbacks_info_sptr;

    struct client_core::impl {

        vclient::vtrc_client_sptr client_;
        connect_info_sptr         last_connection_;
        mutable vtrc::mutex       client_lock_;
        callbacks_info_sptr       cbi_;
        client_core              *parent_;

        impl( vcommon::pool_pair &pp )
            :client_(vclient::vtrc_client::create(pp))
            ,cbi_(vtrc::make_shared<callbacks_info>( ))
        {
            client_->on_connect_connect(
                        vtrc::bind( &impl::on_connect, this ) );

            client_->on_disconnect_connect(
                        vtrc::bind( &impl::on_disconnect, this ) );

            client_->on_ready_connect(
                        vtrc::bind( &impl::on_ready, this ) );

            client_->on_init_error_connect(
                        vtrc::bind( &impl::on_init_error, this,
                                     vtrc::placeholders::_1,
                                     vtrc::placeholders::_2 ) );
        }

        void on_init_error( const vtrc::rpc::errors::container & /*ec*/,
                            const char *message )
        {
            parent_->on_init_error_( message );
        }

        void on_connect( )
        {
            parent_->on_connect_( );
        }

        void on_disconnect( )
        {
            parent_->on_disconnect_( );
        }

        void on_ready( )
        {
            parent_->on_ready_( );
        }

        void set_new_info( const std::string &server, bool nw )
        {
            connect_info_sptr inf( get_connect_info( server, nw ) );
            vtrc::lock_guard<vtrc::mutex> lck(client_lock_);
            last_connection_ = inf;
        }

        connect_info_sptr get_info( )
        {
            vtrc::lock_guard<vtrc::mutex> lck(client_lock_);
            connect_info_sptr c(last_connection_);
            return c;
        }

        void connect( )
        {
            connect_info_sptr ci(get_info( ));
            switch( ci->name_ ) {

            case CONN_PIPE:
                client_->connect( ci->server_ );
                break;

            case CONN_TCP:
                client_->connect( ci->server_, ci->port_ );
                break;

            case CONN_NONE:
                throw std::runtime_error( "Bad server" );
                break;
            };
        }

        void async_connect( client_core::async_closure_func &func )
        {
            connect_info_sptr ci(get_info( ));
            switch( ci->name_ ) {

            case CONN_PIPE:
                client_->async_connect( ci->server_, func );
                break;

            case CONN_TCP:

                client_->async_connect( ci->server_, ci->port_, func );
                break;

            case CONN_NONE:
                throw std::runtime_error( "Bad server" );
                break;

            };
        }
    };

    class proto_event_impl: public fr::proto::events {

    public:

        callbacks_info_wptr cbi_;

        proto_event_impl( callbacks_info_sptr &cbi )
            :cbi_(cbi)
        { }

        void async_op(::google::protobuf::RpcController* /*controller*/,
                      const ::fr::proto::async_op_data* request,
                      ::fr::proto::empty*                /*response*/,
                      ::google::protobuf::Closure* done)
        {
            vcommon::closure_holder holder( done );

            callbacks_info_sptr cbi(cbi_.lock( ));
            if( !cbi ) {
                return;
            }

            vtrc::unique_shared_lock lck( cbi->acb_lock_ );
            cb_map::iterator f(cbi->acb_.find( request->id( ) ));

            if( f != cbi->acb_.end( ) ) {
                f->second( request->error( ).code( ),
                           request->data( ) );
            } else {
                ///std::cout << "not found\n";
            }
        }
    };

    client_core::client_core( vtrc::common::pool_pair &pp )
        :impl_(new impl(pp))
    {
        vtrc::shared_ptr<proto_event_impl>
                e( vtrc::make_shared<proto_event_impl>( impl_->cbi_ ));
        impl_->client_->assign_rpc_handler( e );
        impl_->parent_ = this;
    }

    client_core::~client_core(  )
    {
        delete impl_;
    }

    void client_core::connect( const std::string &server )
    {
        impl_->set_new_info( server, false );
        impl_->connect( );
    }

    void client_core::connect( const std::string &server, bool tcp_nowait )
    {
        impl_->set_new_info( server, tcp_nowait );
        impl_->connect( );
    }

    void client_core::async_connect( const std::string &server,
                                     async_closure_func f )
    {
        impl_->set_new_info( server, false );
        impl_->async_connect( f );
    }

    void client_core::async_connect( const std::string &server,
                                     bool tcp_nowait, async_closure_func f )
    {
        impl_->set_new_info( server, tcp_nowait );
        impl_->async_connect( f );
    }

    void client_core::reconnect( )
    {
        impl_->client_->disconnect( );
        impl_->connect( );
    }

    void client_core::async_reconnect( async_closure_func f )
    {
        impl_->client_->disconnect( );
        impl_->async_connect( f );
    }

    void client_core::disconnect( )
    {
        impl_->client_->disconnect( );
    }

    vtrc::common::rpc_channel *client_core::create_channel( )
    {
        return impl_->client_->create_channel( );
    }

    vtrc::common::rpc_channel *client_core::create_channel( unsigned flags )
    {
        return impl_->client_->create_channel( flags );
    }

    void client_core::register_async_op( size_t id, async_op_callback_type cb )
    {
        vtrc::unique_shared_lock lck( impl_->cbi_->acb_lock_ );
        impl_->cbi_->acb_[id] = cb;
    }

    void client_core::unregister_async_op( size_t id )
    {
        vtrc::upgradable_lock lck( impl_->cbi_->acb_lock_ );
        cb_map::iterator f(impl_->cbi_->acb_.find(id));
        if( f != impl_->cbi_->acb_.end( ) ) {
            vtrc::upgrade_to_unique utl(lck);
            impl_->cbi_->acb_.erase( f );
        }
    }

    void client_core::set_id ( const std::string &id )
    {
        impl_->client_->set_session_id( id );
    }

    void client_core::set_key( const std::string &key )
    {
        impl_->client_->set_session_key( key );
    }

    void client_core::set_id_key( const std::string &id, const std::string &key )
    {
        impl_->client_->set_session_key( id, key );
    }

    std::string client_core::get_id( ) const
    {
        return impl_->client_->get_session_id( );
    }

    bool client_core::has_key( ) const
    {
        return impl_->client_->is_key_set( );
    }

}}}
