#include <QSharedPointer>
#include <iostream>
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
        bool         opened_;
        impl( )
            :mode_("rb")
            ,opened_(false)
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
            if( cl->ready( ) && impl_->opened_ ) {
                FR_QML_CALL_PROLOGUE0
                impl_->iface_create( cl );
                FR_QML_CALL_EPILOGUE(  )
            }
        } else {
            close( );
        }
    }

    void FrClientFile::on_ready( bool value )
    {
        FR_QML_CALL_PROLOGUE0
        if( value && impl_->opened_  ) {
            impl_->iface_create( client( ) );
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

    quint64 FrClientFile::position( ) const
    {
        FR_QML_CALL_PROLOGUE

        return impl_->iface_->tell( );

        FR_QML_CALL_EPILOGUE( (quint64)(-1) );
    }

    bool FrClientFile::opened( ) const
    {
        return impl_->iface_.data( ) != nullptr;
    }

    void FrClientFile::iocontrol( unsigned code, quint64 data )
    {
        FR_QML_CALL_PROLOGUE

        impl_->iface_->ioctl( code, data );

        FR_QML_CALL_EPILOGUE( )
    }

    QByteArray FrClientFile::read( unsigned maximum ) const
    {
        FR_QML_CALL_PROLOGUE
        if( maximum ) {
            std::vector<char> data( maximum );

            size_t r = impl_->iface_->read( &data[0], maximum );

            return QByteArray( &data[0], r );
        }
        FR_QML_CALL_EPILOGUE( QByteArray( ) )
    }

    unsigned FrClientFile::write( const QByteArray &data ) const
    {
        FR_QML_CALL_PROLOGUE
        size_t r = impl_->iface_->write( data.constData( ), data.size( ) );
        return (unsigned)(r);
        FR_QML_CALL_EPILOGUE( 0 )
    }

    void FrClientFile::open( )
    {
        FR_QML_CALL_PROLOGUE0
        setFailed( false );
        impl_->iface_create( client( ) );
        impl_->opened_ = true;
        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientFile::close( )
    {
        impl_->iface_.reset( );
        impl_->opened_ = false;
    }

    void FrClientFile::seek( quint64 value, seek_whence whence ) const
    {
        FR_QML_CALL_PROLOGUE
        impl_->iface_->seek( value, fiface::whence_value2enum( whence ) );
        FR_QML_CALL_EPILOGUE( )
    }


}}
