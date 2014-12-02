#include <QSharedPointer>

#include "fr-client-file.h"

#include "client-core/interfaces/IFile.h"

#include "fr-qml-call-wrappers.h"


namespace fr { namespace declarative {

    namespace fiface = fr::client::interfaces::file;

    typedef QSharedPointer<fiface::iface> iface_qsptr;

    struct FrClientFile::impl {
        std::string  path_;
        std::string  mode_;
        iface_qsptr  iface_;
        impl( )
            :mode_("rb")
        { }

        void iface_create( FrClient *cl )
        {
            iface_.reset( fiface::create( cl->core_client( ),
                                          path_, mode_ ) );
        }

    };

    FrClientFile::FrClientFile(QObject *parent)
        :FrBaseComponent(parent)
        ,impl_(new impl)
    {

    }

    FrClientFile::~FrClientFile(  )
    {
        delete impl_;
    }


    void FrClientFile::on_reinit( )
    {
        FrClient *cl = client( );
        if( cl ) {
            QObject::connect( cl, SIGNAL( channelReady( ) ),
                              this, SLOT( ready( ) ) );

            if( cl->ready( ) ) {
                FR_QML_CALL_PROLOGUE
                impl_->iface_create( cl );
                FR_QML_CALL_EPILOGUE( )
            }
        }
    }

    void FrClientFile::on_ready( bool value )
    {
        FR_QML_CALL_PROLOGUE
        if( value ) {
            impl_->iface_create( client( ) );
        } else {
            impl_->iface_.reset( );
        }
        FR_QML_CALL_EPILOGUE( )
    }

    bool FrClientFile::clientFailed( ) const
    {
        if( impl_->iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }

    QString FrClientFile::path( ) const
    {
        return QString::fromUtf8( impl_->path_.c_str( ) );
    }

    void FrClientFile::setPath( const QString &value )
    {
        std::string nvalue( value.toUtf8( ).data( ) );
        if( nvalue != impl_->path_ ) {
            impl_->path_ = nvalue;
            emit pathChanged( value );
        }
    }

    QString FrClientFile::mode( ) const
    {
        return QString::fromUtf8( impl_->mode_.c_str( ) );
    }

    void FrClientFile::setMode( const QString &value )
    {
        std::string nvalue( value.toUtf8( ).data( ) );
        if( nvalue != impl_->mode_ ) {
            impl_->mode_ = nvalue;
            emit modeChanged( value );
        }
    }

}}
