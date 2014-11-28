#include "fr-base-component.h"

namespace fr { namespace declarative {

    FrBaseComponent::FrBaseComponent(QObject *parent)
        :QObject(parent)
        ,client_(nullptr)
    { }

    FrClient *FrBaseComponent::client( ) const
    {
        return client_;
    }

    void FrBaseComponent::setClient( FrClient *new_value )
    {
        if( new_value != client_ ) {

            if( client_ ) {
                QObject::disconnect( client_, SIGNAL(readyChanged(bool)),
                                     this,    SLOT(onReady(bool)) );
            }

            client_ = new_value;

            QObject::connect( client_, SIGNAL(readyChanged(bool)),
                              this,    SLOT(onReady(bool)) );

            on_reinit( );
            emit clientChanged( client_ );
        }
    }

    void FrBaseComponent::onReady( bool value )
    {
        on_ready( value );
    }

}}
