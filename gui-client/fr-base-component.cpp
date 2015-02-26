#include "fr-base-component.h"

namespace fr { namespace declarative {


    FrBaseComponent::FrBaseComponent( QObject *parent )
        :QObject(parent)
        ,ready_(false)
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

    void FrBaseComponent::setReady( bool value ) const
    {
        if( value != ready_ ) {
            ready_ = value;
            emit readyChanged( ready_ );
        }
    }

    bool FrBaseComponent::ready( ) const
    {
        return ready_;
    }

}}
