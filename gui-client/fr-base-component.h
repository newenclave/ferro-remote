#ifndef FRBASECOMPONENT_H
#define FRBASECOMPONENT_H

#include <QObject>
#include "fr-client.h"

namespace fr { namespace declarative {

    class FrBaseComponent: public QObject {

        Q_OBJECT
        Q_PROPERTY( fr::declarative::FrClient *client
                    READ client WRITE setClient NOTIFY clientChanged )

        FrClient *client_;

    private:

        virtual void on_reinit( ) { }
        virtual void on_ready( bool value ) { Q_UNUSED(value) }

    public:

        explicit FrBaseComponent( QObject *parent = 0 );

        FrClient *client( ) const;
        void setClient( FrClient *new_value );

    signals:

        void clientChanged( const fr::declarative::FrClient *value );
        //void callFailed( const QString &what );

    public slots:

    private slots:

        void onReady( bool value );

    };

}}

#endif // FRBASECOMPONENT_H
