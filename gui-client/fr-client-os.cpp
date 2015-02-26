#include "fr-client-os.h"
#include "client-core/interfaces/IOS.h"

#include <QSharedPointer>

#include <iostream>

#include "fr-qml-call-wrappers.h"

namespace fr { namespace declarative {

    namespace ifaces = fr::client::interfaces;
    namespace vcomm = vtrc::common;

    typedef QSharedPointer<ifaces::os::iface> iface_qsptr;

    struct FrClientOS::impl {
        iface_qsptr  os_iface_;
        impl( )
            :os_iface_(nullptr)
        { }
    };

    FrClientOS::FrClientOS( QObject *parent )
        :FrClientComponent(parent)
        ,impl_(new impl)
    { }

    FrClientOS::~FrClientOS( )
    {
        //std::cout << "Os delete!" << std::endl;
        delete impl_;
    }

    void FrClientOS::on_reinit( )
    {
        FrClient *cl = client( );
        if( cl ) {
            QObject::connect( cl, SIGNAL( channelReady( ) ),
                              this, SLOT( onReady( ) ) );

            if( cl->ready( ) ) {
                impl_->os_iface_.reset(
                            ifaces::os::create( cl->core_client( ) ) );
                setReady( true );
            }
        }
    }

    bool FrClientOS::clientFailed( ) const
    {
        if( impl_->os_iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }

    int FrClientOS::execute( const QString &cmd ) const
    {
        FR_QML_CALL_PROLOGUE

        int res = impl_->os_iface_->execute( cmd.toUtf8( ).constData( ) );
        return res;

        FR_QML_CALL_EPILOGUE( -1 )
    }

    void FrClientOS::onReady(  )
    {
        impl_->os_iface_.reset(
                    ifaces::os::create( client( )->core_client( ) ) );
        setReady( true );
    }

}}
