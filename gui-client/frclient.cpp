#include <iostream>
#include <string>

#include <QException>

#include "frclient.h"
#include "application-data.h"
#include "client-core/fr-client.h"

namespace fr { namespace declarative {

    struct FrClient::impl {
        client::core::client_core client_;
        bool connected_;
        impl( )
            :client_(global_app_data.pools( ))
            ,connected_(false)
        { }
    };

    FrClient::FrClient( QObject *parent )
        :QObject(parent)
        ,impl_(new impl)
    { }

    FrClient::~FrClient( )
    {
        delete impl_;
    }

    bool FrClient::connected( ) const
    {
        return impl_->connected_;
    }

    void FrClient::connect( const QString &server )
    { try {
        if( !impl_->connected_ ) {
            impl_->client_.connect( server.toLocal8Bit( ).constData( ) );
            impl_->connected_ = true;
            emit connectedChanged( true );
        }
    } catch( std::exception & /*ex*/ ) {
        throw;
    } }

    void FrClient::disconnect( )
    {
        impl_->client_.disconnect( );
        impl_->connected_ = false;
        emit connectedChanged( false );
    }

}}
