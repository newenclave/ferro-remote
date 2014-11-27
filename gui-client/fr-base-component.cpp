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
            client_ = new_value;
            emit clientChanged( client_ );
        }
    }

}}
