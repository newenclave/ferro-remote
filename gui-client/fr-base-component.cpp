#include "fr-base-component.h"

namespace fr { namespace declarative {

    struct FrBaseComponent::impl {
        mutable bool               failed_;
        mutable QString            error_;
        impl( )
            :failed_(false)
        { }
    };

    FrBaseComponent::FrBaseComponent( QObject *parent )
        :QObject(parent)
        ,impl_(new impl)
        ,client_(nullptr)
    { }

    FrBaseComponent::~FrBaseComponent( )
    {
        delete impl_;
    }

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

            setFailed( false );
            on_reinit( );
            emit clientChanged( client_ );
        }
    }

    QString FrBaseComponent::error( ) const
    {
        return impl_->error_;
    }

    bool FrBaseComponent::failed( ) const
    {
        return impl_->failed_;
    }

    void FrBaseComponent::setFailed( bool value ) const
    {
        if( value != impl_->failed_ ) {
            impl_->failed_ = value;
            emit failedChanged( impl_->failed_ );
        }
    }

    bool FrBaseComponent::prologueCall( ) const
    {
        return !clientFailed( ) && !failed( );
    }

    void FrBaseComponent::setError( const QString &value ) const
    {
        impl_->error_ = value;
    }

    void FrBaseComponent::onReady( bool value )
    {
        on_ready( value );
    }

}}
