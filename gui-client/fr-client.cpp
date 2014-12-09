#include <iostream>
#include <string>

#include <QException>
#include <functional>

#include "fr-client.h"
#include "application-data.h"
#include "client-core/fr-client.h"

#include "fr-qml-call-wrappers.h"

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
        :FrBaseComponent(parent)
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
    {
        FR_QML_CALL_PROLOGUE0
        setFailed( false );
        if( impl_->state_ == impl::state_none ) {
            impl_->client_.async_connect( server.toLocal8Bit( ).constData( ),
                   fr::client::core::client_core::async_closure_func( ) );
            impl_->state_ = impl::state_connect;
        }

        FR_QML_CALL_EPILOGUE(  )
    }

    void FrClient::disconnect( )
    {
        impl_->client_.disconnect( );
    }

    client::core::client_core &FrClient::core_client( )
    {
        return impl_->client_;
    }

    QString FrClient::sessionId( ) const
    {
        return QString::fromUtf8( impl_->client_.get_id( ).c_str( ) );
    }

    void FrClient::setSessionId( const QString &sid )
    {
        std::string old( impl_->client_.get_id( ) );
        std::string n(sid.toUtf8( ).constData( ));

        if( n != old ) {
            impl_->client_.set_id( n );
            emit sessionIdChanged( sid );
        }
    }

    QString FrClient::sessionKey( ) const
    {
        return QString::fromUtf8( impl_->client_.has_key( ) ? "********" : "" );
    }

    void FrClient::setSessionKey( const QString &sk )
    {
        std::string n(sk.toUtf8( ).constData( ));
        impl_->client_.set_key( n );
        emit sessionKeyChanged( sk.size( ) == 0 ? "" : "********" );
    }

}}
