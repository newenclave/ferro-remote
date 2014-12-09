#include "fr-base-component.h"

namespace fr { namespace declarative {


    FrBaseComponent::FrBaseComponent( QObject *parent )
        :QObject(parent)
        ,failed_(false)
    { }

    QString FrBaseComponent::error( ) const
    {
        return error_;
    }

    bool FrBaseComponent::failed( ) const
    {
        return failed_;
    }

    void FrBaseComponent::setFailed( bool value ) const
    {
        if( value != failed_ ) {
            failed_ = value;
            emit failedChanged( failed_ );
        }
    }

    bool FrBaseComponent::prologueCall( ) const
    {
        return !clientFailed( ) && !failed( );
    }

    void FrBaseComponent::setError( const QString &value ) const
    {
        error_ = value;
    }

}}
