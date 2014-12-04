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


    struct FrComponent::impl {
        mutable bool               failed_;
        mutable QString            error_;
        impl( )
            :failed_(false)
        { }
    };

    FrComponent::FrComponent( QObject *parent )
        :FrBaseComponent(parent)
        ,impl_(new impl)
        ,client_(nullptr)
    { }

    FrComponent::~FrComponent( )
    {
        delete impl_;
    }

    FrClient *FrComponent::client( ) const
    {
        return client_;
    }

    void FrComponent::setClient( FrClient *new_value )
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

    void FrComponent::onReady( bool value )
    {
        on_ready( value );
    }

}}
