#ifndef FRFILESYSTEM_H
#define FRFILESYSTEM_H

#include <QObject>
#include "fr-base-component.h"

namespace fr { namespace declarative {

    class FrFilesystem: public FrBaseComponent {

        struct impl;
        impl  *impl_;

        Q_OBJECT
        Q_PROPERTY( QString path
                    READ path WRITE setPath NOTIFY pathChanged)

    private:

        void on_reinit( );
        void on_ready( bool value );

    public:

        explicit FrFilesystem(QObject *parent = 0);
        ~FrFilesystem( );

        void setPath( const QString &new_value );
        QString path( ) const ;

    signals:

        void pathChanged( const QString &value );

    public slots:

    };

}}

#endif // FRFILESYSTEM_H
