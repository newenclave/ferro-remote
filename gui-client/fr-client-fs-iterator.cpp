#include <iostream>

#include "fr-client-fs-iterator.h"

#include "client-core/interfaces/IFilesystem.h"

#include "fr-client-fs.h"
#include "fr-qml-call-wrappers.h"

namespace fr { namespace declarative {

    namespace {
        const char *leaf( const char *path )
        {
            const char *lf = path;
            for( ;*path; path++ ) {
                if( (*path == '/' || *path == '\\') && *(path + 1) ) {
                    lf = path + 1;
                }
            }
            return lf;
        }
    }

    FrClientFsIterator::FrClientFsIterator( proto_iter_impl *iter,
                                            QObject *parent )

        :FrBaseComponent(parent)
        ,impl_(iter)
    { }

    FrClientFsIterator::~FrClientFsIterator( )
    {
        ///std::cout << "Iterator delete!" << std::endl;
    }

    QString FrClientFsIterator::next( )
    {
        FR_QML_CALL_PROLOGUE
        if( !end( ) ) {
            impl_->next( );
            emit changed( );
            if( end( ) ) {
                emit endReached( );
            }
            return name( );
        }
        FR_QML_CALL_EPILOGUE( QString( ) );
    }

    QString FrClientFsIterator::fullPath( ) const
    {
        return QString::fromUtf8( impl_->get( ).path.c_str( ) );
    }

    QString FrClientFsIterator::name( ) const
    {
        return QString::fromUtf8( leaf( impl_->get( ).path.c_str( ) ) );
    }

    bool FrClientFsIterator::end( ) const
    {
        return impl_->end( );
    }

    QObject *FrClientFsIterator::info( ) const
    {
        FrFilesystemInfo *ii = new FrFilesystemInfo( impl_->info( ) );
        ii->deleteLater( );
        return ii;
    }

}}
