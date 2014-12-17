#include <QSharedPointer>

#include "fr-client-i2c.h"

#include "client-core/interfaces/II2C.h"
#include "fr-qml-call-wrappers.h"

namespace fr { namespace declarative {

    namespace {

        namespace i2c_ns = fr::client::interfaces::i2c;
        typedef i2c_ns::iface i2c_iface;

    }

    struct FrClientI2c::impl {

        QSharedPointer<i2c_iface> iface_;
        unsigned bus_id_;
        quint16  slave_addr_;

        impl( )
            :bus_id_(FrClientI2c::InvalidBusId)
            ,slave_addr_(FrClientI2c::InvalidSlaveAddress)
        { }

        void reinit_iface( FrClient *client )
        {
            if( !client ) {
                return;
            }

            if( bus_id_ != FrClientI2c::InvalidBusId ) {
                iface_.reset( i2c_ns::open( client->core_client( ),
                                            bus_id_, slave_addr_ ) );
            }
        }

    };

    FrClientI2c::FrClientI2c( QObject *parent )
        :FrClientComponent(parent)
        ,impl_(new impl)
    { }

    FrClientI2c::~FrClientI2c( )
    {
        delete impl_;
    }

    void FrClientI2c::on_reinit( )
    {
        if( client( ) && client( )->ready( ) ) {
            FR_QML_CALL_PROLOGUE0
            setFailed( false );
            impl_->reinit_iface( client( ) );
            FR_QML_CALL_EPILOGUE( )
        }
    }

    void FrClientI2c::on_ready( bool value )
    {
        if( value ) {
            FR_QML_CALL_PROLOGUE0
            impl_->reinit_iface( client( ) );
            FR_QML_CALL_EPILOGUE( )
        } else {
            impl_->iface_.reset( );
        }
    }

    bool FrClientI2c::clientFailed( ) const
    {
        if( impl_->iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }

    quint32 FrClientI2c::busId( ) const
    {
        return impl_->bus_id_;
    }

    void FrClientI2c::setBusId( quint32 value )
    {
        if( impl_->bus_id_ != value ) {
            if( value == InvalidBusId ) {
                impl_->iface_.reset( );
            } else {
                impl_->bus_id_ = value;
                FR_QML_CALL_PROLOGUE
                impl_->reinit_iface( client( ) );
                emit busIdChanged( impl_->bus_id_ );
                FR_QML_CALL_EPILOGUE( )
            }
        }
    }

    quint16 FrClientI2c::slaveAddress( ) const
    {
        return impl_->slave_addr_;
    }

    void FrClientI2c::setSlaveAddress( quint16 value )
    {
        if( value != impl_->slave_addr_ ) {
            FR_QML_CALL_PROLOGUE
            impl_->iface_->set_address( value );
            impl_->slave_addr_ = value;
            emit slaveAddressChanged( impl_->slave_addr_ );
            FR_QML_CALL_EPILOGUE( )
        }
    }


}}
