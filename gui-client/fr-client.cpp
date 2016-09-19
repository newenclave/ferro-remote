#include <iostream>
#include <string>

#include <QException>
#include <functional>

#include "fr-client.h"
#include "application-data.h"
#include "client-core/fr-client.h"

#include "fr-qml-call-wrappers.h"

#include "vtrc-common/vtrc-hash-iface.h"

namespace fr { namespace declarative {

    void async_connect_handler( const boost::system::error_code & /*err*/ )
    {
        ;;;
    }

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
        bool     connecting_;
        std::string id_;
        std::string key_;
        impl( )
            :client_(global_app_data.pools( ))
            ,state_(state_none)
            ,connecting_(false)
        { }

        void connect_signals( )
        {
            namespace ph = std::placeholders;
            client_.on_connect_connect( std::bind( &impl::on_connect, this ) );

            client_.on_disconnect_connect( std::bind(
                                                &impl::on_disconnect, this ) );

            client_.on_ready_connect( std::bind( &impl::on_ready, this ) );

            client_.on_init_error_connect( std::bind( &impl::on_initerror, this,
                                                      ph::_1 ) );

        }

        void on_connect( )
        {
            state_ = state_connected;
            emit parent_->connectingChanged( false );
            emit parent_->connected( );
        }

        void on_disconnect( )
        {
            state_ = state_none;
            parent_->setReady( false );
            emit parent_->connectingChanged( false );
            emit parent_->disconnected( );
        }

        void on_ready( )
        {
            state_ = state_ready;
            emit parent_->connectingChanged( false );
            emit parent_->channelReady( );
            parent_->setReady( true );
        }

        void on_initerror( const char *message  )
        {
            state_ = state_none;
            parent_->setReady( false );
            emit parent_->connectingChanged( false );
            emit parent_->initError( message );
        }

        void reset_key( )
        {
            namespace vhash = vtrc::common;
            std::unique_ptr<vhash::hash_iface> hash(
                        vhash::hash::sha2::create256( ));
            auto key_pair = id_ + key_;
            auto res = hash->get_data_hash( key_pair.c_str( ),
                                            key_pair.size( ) );
            client_.set_id_key( id_, res );
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

    void FrClient::connect( const QString &server )
    {
        FR_QML_CALL_PROLOGUE0
        setFailed( false );
        if( impl_->state_ == impl::state_none ) {
            impl_->client_.async_connect( server.toLocal8Bit( ).constData( ),
                                          async_connect_handler );
            impl_->state_ = impl::state_connect;
            emit connectingChanged( true );
            emit tryConnect( );
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
        impl_->id_ = sid.toUtf8( ).constData( );
        if( impl_->id_ != old ) {
            impl_->reset_key( );
            emit sessionIdChanged( sid );
        }
    }

    QString FrClient::sessionKey( ) const
    {
        return QString::fromUtf8( impl_->client_.has_key( ) ? "********" : "" );
    }

    void FrClient::setSessionKey( const QString &sk )
    {
        impl_->key_ = sk.toUtf8( ).constData( );
        impl_->reset_key( );
        emit sessionKeyChanged( sk.size( ) == 0 ? "" : "********" );
    }

    bool FrClient::connecting( ) const
    {
        return impl_->state_ == impl::state_connect;
    }

}}
