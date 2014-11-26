#include "fr-client-os.h"
#include "client-core/interfaces/IOS.h"

#include <memory>

namespace fr { namespace declarative {

    namespace ifaces = fr::client::interfaces;

    struct FrClientOS::impl {
        FrClient                            *client_;
        std::shared_ptr<ifaces::os::iface>   os_iface_;
        impl( )
            :client_(nullptr)
            ,os_iface_(nullptr)
        { }
    };

    FrClientOS::FrClientOS( QObject *parent )
        :QObject(parent)
        ,impl_(new impl)
    {

    }

    FrClientOS::~FrClientOS( )
    {
        delete impl_;
    }

    FrClient *FrClientOS::client( ) const
    {
        return impl_->client_;
    }

    void FrClientOS::setClient( FrClient *new_value )
    {
        if( new_value != impl_->client_ ) {
            impl_->client_ = new_value;

            QObject::connect( impl_->client_,
                              SIGNAL( connectedChanged(bool) ),
                              this, SLOT( onClientConnectChange(bool) )
                            );

            if( impl_->client_->connected( ) ) {
                impl_->os_iface_.reset(
                        ifaces::os::create(
                            impl_->client_->core_client( ) ) );
            }

            emit clientChanged( impl_->client_ );
        }
    }

    int FrClientOS::execute( const QString &cmd ) const
    {
        if( impl_->os_iface_ ) {
            return impl_->os_iface_->execute( cmd.toLocal8Bit( ).constData( ) );
        }
        return -1;
    }

    void FrClientOS::onClientConnectChange( bool value )
    {
        if( value ) {
            impl_->os_iface_.reset(
                        ifaces::os::create(
                            impl_->client_->core_client( ) ) );
        }
    }

}}
