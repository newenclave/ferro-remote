#ifndef FRCLIENTFSITERATOR_H
#define FRCLIENTFSITERATOR_H

#include <QString>

#include "fr-client-component.h"

namespace fr { namespace client {

    namespace interfaces { namespace filesystem {
        struct directory_iterator_impl;
    }}

}}

namespace fr { namespace declarative {

    class FrClientFs;

    class FrClientFsIterator: public FrBaseComponent {

        Q_OBJECT

        Q_PROPERTY( QString name READ name )
        Q_PROPERTY( QString fullPath READ fullPath )
        Q_PROPERTY( bool end READ end )
        Q_PROPERTY( QObject *info READ info )

        typedef fr::client::interfaces
                  ::filesystem::directory_iterator_impl proto_iter_impl;

        proto_iter_impl *impl_;

    public:

        FrClientFsIterator( proto_iter_impl *iter, QObject *parent = nullptr );

        ~FrClientFsIterator( );

    public:

        QString fullPath( ) const;
        QString name( ) const;
        bool end( ) const;
        QObject *info( ) const;

        Q_INVOKABLE QString next( ); /// make next call, returns name( )

    signals:

        void endReached( ) const;
        void changed( ) const;

    public slots:

    };

}}

#endif // FRCLIENTFSITERATOR_H
