#include <QSharedPointer>

#include "fr-client-fs.h"

#include "client-core/interfaces/IFilesystem.h"

#include "vtrc-common/vtrc-exception.h"

#include <iostream>
#include <string>
#include <functional>

#include <QException>

namespace fr { namespace declarative {

    namespace iface = fr::client::interfaces;
    typedef QSharedPointer<iface::filesystem::iface> iface_qsptr;

    struct FrClientFs::impl {
        std::string currentPath_;
        iface_qsptr iface_;
    };


    class MyException : public QException
    {
    public:
        void raise( ) const { throw *this; }
        MyException *clone( ) const { return new MyException(*this); }
    };

    FrClientFs::FrClientFs( QObject *parent )
        :FrBaseComponent(parent)
        ,impl_(new impl)
    {

    }

    FrClientFs::~FrClientFs( )
    {
        delete impl_;
    }

    void FrClientFs::on_reinit( )
    {
        if( client( ) ) {
            impl_->iface_.reset(
                iface::filesystem::create( client( )->core_client( ),
                                           impl_->currentPath_ ) );
            if( client( )->ready( ) ) {
                impl_->iface_->cd( impl_->currentPath_ );
            }
        }
    }

    void FrClientFs::on_ready( bool value )
    {
        if( value ) {
            impl_->iface_->cd( impl_->currentPath_ );
        } else {
            impl_->iface_.reset( );
        }
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
        if( impl_->iface_.data( ) ) {
            return impl_->iface_->exists( path.toUtf8( ).constData( ) );
        }
        return false;
    }

    void FrClientFs::mkdir( const QString &path ) const
    {
        if( impl_->iface_.data( ) ) {
            impl_->iface_->mkdir( path.toUtf8( ).constData( ) );
        }
    }

    void FrClientFs::remove( const QString &path ) const
    {
        if( impl_->iface_.data( ) ) {
            impl_->iface_->del( path.toUtf8( ).constData( ) );
        }
    }

    void FrClientFs::removeAll( const QString &path ) const
    {
        if( impl_->iface_.data( ) ) {
            impl_->iface_->remove_all( path.toUtf8( ).constData( ) );
        }
    }

}}

