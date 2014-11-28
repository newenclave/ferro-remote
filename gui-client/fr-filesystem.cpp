#include <QSharedPointer>

#include "fr-filesystem.h"
#include "client-core/interfaces/IFilesystem.h"

#include <string>

namespace fr { namespace declarative {

    namespace iface = fr::client::interfaces;
    typedef QSharedPointer<iface::filesystem::iface> iface_qsptr;

    struct FrFilesystem::impl {
        std::string currentPath_;
        iface_qsptr iface_;
    };

    FrFilesystem::FrFilesystem(QObject *parent)
        :FrBaseComponent(parent)
        ,impl_(new impl)
    {

    }

    FrFilesystem::~FrFilesystem( )
    {
        delete impl_;
    }

    void FrFilesystem::on_reinit( )
    {
        if( client( ) ) {
            std::string path();
            impl_->iface_.reset(
                iface::filesystem::create( client( )->core_client( ),
                                           impl_->currentPath_ ) );
            if( client( )->ready( ) ) {
                impl_->iface_->cd( impl_->currentPath_ );
            }
        }
    }

    void FrFilesystem::on_ready( bool value )
    {
        if( value ) {
            impl_->iface_->cd( impl_->currentPath_ );
        }
    }

    void FrFilesystem::setPath( const QString &new_value )
    {
        std::string path(new_value.toLocal8Bit( ).constData( ));
        if( impl_->currentPath_ != path ) {
            impl_->currentPath_ = path;
            emit pathChanged( new_value );
        }
    }

    QString FrFilesystem::path( ) const
    {
        return QString::fromLocal8Bit( impl_->currentPath_.c_str( ) );
    }

}}

