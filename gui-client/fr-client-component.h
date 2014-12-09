#ifndef FRCLIENTCOMPONENT_H
#define FRCLIENTCOMPONENT_H

#include "fr-client.h"
#include "fr-base-component.h"

namespace fr { namespace declarative {

    //class FrClient;

    class FrClientComponent: public FrBaseComponent {

        Q_OBJECT
        Q_PROPERTY( fr::declarative::FrClient *client
                    READ client WRITE setClient NOTIFY clientChanged )


        struct  impl;
        impl   *impl_;

        fr::declarative::FrClient *client_;

    private:

        virtual void on_reinit( ) { }
        virtual void on_ready( bool value ) { Q_UNUSED(value) }

    public:

        explicit FrClientComponent( QObject *parent = 0 );
        ~FrClientComponent( );

        FrClient *client( ) const;
        void setClient( FrClient *new_value );

    signals:

        void clientChanged( const fr::declarative::FrClient *value ) const;

    public slots:

    private slots:

        void onReady( bool value );

    };

}}


#endif // FRCLIENTCOMPONENT_H
