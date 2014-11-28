#include "fr-client-os.h"
#include "client-core/interfaces/IOS.h"

#include <iostream>
#include <memory>

namespace fr { namespace declarative {

    namespace ifaces = fr::client::interfaces;

    struct FrClientOS::impl {
        std::shared_ptr<ifaces::os::iface>   os_iface_;
        impl( )
            :os_iface_(nullptr)
        { }
    };

    FrClientOS::FrClientOS( QObject *parent )
        :FrBaseComponent(parent)
        ,impl_(new impl)
    { }

    FrClientOS::~FrClientOS( )
    {
        std::cout << "Os delete!" << std::endl;
        delete impl_;
    }

    void FrClientOS::reinit( )
    {
        FrClient *cl = client( );
        if( cl ) {
            QObject::connect( cl, SIGNAL( channelReady( ) ),
                              this, SLOT( ready( ) ) );

            if( cl->ready( ) ) {
                impl_->os_iface_.reset(
                            ifaces::os::create( cl->core_client( ) ) );
            }
        }
    }

    int FrClientOS::execute( const QString &cmd ) const
    {
        if( impl_->os_iface_ ) {
            return impl_->os_iface_->execute( cmd.toLocal8Bit( ).constData( ) );
        }
        return -1;
    }

    void FrClientOS::ready(  )
    {
        impl_->os_iface_.reset(
                    ifaces::os::create( client( )->core_client( ) ) );
    }

}}
