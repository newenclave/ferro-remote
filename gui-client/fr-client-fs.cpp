#include <QSharedPointer>

#include "fr-client-fs.h"
#include "client-core/interfaces/IFilesystem.h"

#include "vtrc-common/vtrc-exception.h"

#include <iostream>
#include <string>
#include <functional>
#include <memory>

#include <QException>
#include <QSharedPointer>

#include "fr-qml-call-wrappers.h"
#include "fr-client-fs-iterator.h"
#include "fr-client-file.h"

namespace fr { namespace declarative {

    namespace iface = fr::client::interfaces;
    typedef QSharedPointer<iface::filesystem::iface> iface_qsptr;

    typedef fr::client::interfaces::filesystem::info_data fs_info_data;

    FrFilesystemInfo::FrFilesystemInfo( const fs_info_data &data )
        :data_(new fs_info_data(data))
    { }

    FrFilesystemInfo::~FrFilesystemInfo( )
    {
        delete data_;
    }

    bool FrFilesystemInfo::exists( ) const
    {
        return data_->is_exist;
    }

    bool FrFilesystemInfo::directory( ) const
    {
        return data_->is_directory;
    }

    bool FrFilesystemInfo::empty( ) const
    {
        return data_->is_empty;
    }

    bool FrFilesystemInfo::regular( ) const
    {
        return data_->is_regular;
    }

    bool FrFilesystemInfo::symlink( ) const
    {
        return data_->is_symlink;
    }

    struct FrClientFs::impl {
        std::string currentPath_;
        iface_qsptr iface_;
    };

    FrClientFs::FrClientFs( QObject *parent )
        :FrClientComponent(parent)
        ,impl_(new impl)
    {

    }

    FrClientFs::~FrClientFs( )
    {
        //std::cout << "Fs delete!" << std::endl;
        delete impl_;
    }

    void FrClientFs::on_reinit( )
    {
        if( client( ) && client( )->ready( ) ) {
            impl_->iface_.reset(
                iface::filesystem::create( client( )->core_client( ),
                                           impl_->currentPath_ ) );
            setReady( true );
            FR_QML_CALL_PROLOGUE
            impl_->iface_->cd( impl_->currentPath_ );
            FR_QML_CALL_EPILOGUE( )
        }
    }

    bool FrClientFs::clientFailed( ) const
    {
        if( impl_->iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }

    void FrClientFs::on_ready( bool value )
    {
        FR_QML_CALL_PROLOGUE0
        if( value ) {
            impl_->iface_.reset(
                iface::filesystem::create( client( )->core_client( ),
                                           impl_->currentPath_ ) );
            setReady( true );
            impl_->iface_->cd( impl_->currentPath_ );
        } else {
            impl_->iface_.reset( );
        }
        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientFs::setPath( const QString &new_value )
    {
        std::string path(new_value.toUtf8( ).constData( ));
        if( impl_->currentPath_ != path ) {
            impl_->currentPath_ = path;
            emit pathChanged( new_value );
        }
    }

    QString FrClientFs::path( ) const
    {
        return QString::fromUtf8( impl_->currentPath_.c_str( ) );
    }

    bool FrClientFs::exists( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE
        return impl_->iface_->exists( path.toUtf8( ).constData( ) );
        FR_QML_CALL_EPILOGUE( false )
    }

    void FrClientFs::mkdir( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE

        impl_->iface_->mkdir( path.toUtf8( ).constData( ) );

        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientFs::remove( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE
        impl_->iface_->del( path.toUtf8( ).constData( ) );
        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientFs::removeAll( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE

        impl_->iface_->remove_all( path.toUtf8( ).constData( ) );

        FR_QML_CALL_EPILOGUE( )
    }

    QObject *FrClientFs::info( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE

        fs_info_data fid;
        impl_->iface_->info( path.toUtf8( ).constData( ), fid );
        FrFilesystemInfo *inst = new FrFilesystemInfo( fid );
        inst->deleteLater( );
        return inst;

        FR_QML_CALL_EPILOGUE( nullptr )
    }

    QObject *FrClientFs::begin( const QString &path ) const
    {
        FR_QML_CALL_PROLOGUE
        FrClientFsIterator *inst =
                new FrClientFsIterator( begin_iterate( path ) );
        inst->deleteLater( );
        return inst;
        FR_QML_CALL_EPILOGUE( nullptr )
    }

    QByteArray FrClientFs::readFile( const QString &path,
                                     unsigned maximum,
                                     quint64 from ) const
    {
        FR_QML_CALL_PROLOGUE
        if( maximum ) {
            std::vector<char> data( maximum );
            size_t r = impl_->iface_->read_file( path.toUtf8( ).constData( ),
                                                 &data[0], maximum, from );
            return QByteArray( &data[0], r );
        }
        FR_QML_CALL_EPILOGUE( QByteArray( ) )
    }

    unsigned FrClientFs::writeFile( const QString &path,
                                    const QByteArray &data, quint64 from ) const
    {
        FR_QML_CALL_PROLOGUE
        size_t r = impl_->iface_->write_file( path.toUtf8( ).constData( ),
                                              data.constData( ),
                                              data.size( ), from );
        return (unsigned)(r);

        FR_QML_CALL_EPILOGUE( 0 )
    }

    QObject *FrClientFs::openFile( const QString &path, const QString &mode )
    {
        std::unique_ptr<FrClientFile> inst(new FrClientFile( this ));
        inst->setClient( client( ) );
        inst->setPath( path );
        inst->setMode( mode );
        inst->open( );
        return inst.release( );
    }

    iface::filesystem::directory_iterator_impl *
                        FrClientFs::begin_iterate( const QString &path ) const
    {
        if( impl_->iface_ ) {
            return impl_->iface_->begin_iterate( path.toUtf8( ).constData( ) );
        } else {
            return nullptr;
        }
    }



}}
