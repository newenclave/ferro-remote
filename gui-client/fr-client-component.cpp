#include "fr-client-component.h"

namespace fr { namespace declarative {

    struct FrClientComponent::impl {
        mutable bool               failed_;
        mutable QString            error_;
        impl( )
            :failed_(false)
        { }
    };

    FrClientComponent::FrClientComponent( QObject *parent )
        :FrBaseComponent(parent)
        ,impl_(new impl)
        ,client_(nullptr)
    { }

    FrClientComponent::~FrClientComponent( )
    {
        delete impl_;
    }

    FrClient *FrClientComponent::client( ) const
    {
        return client_;
    }

    void FrClientComponent::setClient( FrClient *new_value )
    {
        if( new_value != client_ ) {

            if( client_ ) {
                QObject::disconnect( client_, SIGNAL(readyChanged(bool)),
                                     this,    SLOT(onReady(bool)) );
            }

            client_ = new_value;

            QObject::connect( client_, SIGNAL(readyChanged(bool)),
                              this,    SLOT(onReady(bool)) );

            setFailed( false );
            on_reinit( );
            emit clientChanged( client_ );
        }
    }

    void FrClientComponent::onReady( bool value )
    {
        on_ready( value );
    }

}}
