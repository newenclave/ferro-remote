#include <iostream>
#include <string>

#include <QException>
#include <functional>

#include "fr-client.h"
#include "application-data.h"
#include "client-core/fr-client.h"

namespace fr { namespace declarative {

    struct FrClient::impl {

        enum {
             state_none      = 0
            ,state_connect
            ,state_connected
            ,state_ready
        };

        client::core::client_core client_;
        FrClient *parent_;
        unsigned state_;
        impl( )
            :client_(global_app_data.pools( ))
            ,state_(state_none)
        { }

        void connect_signals( )
        {
            client_.on_connect_connect( std::bind( &impl::on_connect, this ) );

            client_.on_disconnect_connect( std::bind(
                                                &impl::on_disconnect, this ) );

            client_.on_ready_connect( std::bind( &impl::on_ready, this ) );

            client_.on_init_error_connect( std::bind( &impl::on_initerror, this,
                                                      std::placeholders::_1 ) );

        }

        void on_connect( )
        {
            state_ = state_connected;
            emit parent_->connected( );
        }

        void on_disconnect( )
        {
            state_ = state_none;
            emit parent_->disconnected( );
            emit parent_->readyChanged( false );
        }

        void on_ready( )
        {
            state_ = state_ready;
            emit parent_->channelReady( );
            emit parent_->readyChanged( true );
        }

        void on_initerror( const char *message  )
        {
            state_ = state_none;
            emit parent_->initError( message );
        }
    };

    FrClient::FrClient( QObject *parent )
        :QObject(parent)
        ,impl_(new impl)
    {
        impl_->parent_ = this;
        impl_->connect_signals( );
    }

    FrClient::~FrClient( )
    {
        delete impl_;
    }

    bool FrClient::ready( ) const
    {
        return impl_->state_ == impl::state_ready;
    }

    void FrClient::connect( const QString &server )
    { try {
        if( impl_->state_ == impl::state_none ) {
            impl_->client_.async_connect( server.toLocal8Bit( ).constData( ),
                   fr::client::core::client_core::async_closure_func( ) );
            impl_->state_ = impl::state_connect;
        }
    } catch( std::exception & /*ex*/ ) {
        throw;
    } }

    void FrClient::disconnect( )
    {
        impl_->client_.disconnect( );
    }

    client::core::client_core &FrClient::core_client( )
    {
        return impl_->client_;
    }

}}
