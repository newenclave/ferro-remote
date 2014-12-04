#ifndef FRBASECOMPONENT_H
#define FRBASECOMPONENT_H

#include <QObject>
#include <QVariant>

#include "fr-client.h"

namespace fr { namespace declarative {

    class FrBaseComponent: public QObject {

        Q_OBJECT

        Q_PROPERTY( bool failed
                    READ failed WRITE setFailed NOTIFY failedChanged )

        Q_PROPERTY( QString error READ error )

        mutable bool    failed_;
        mutable QString error_;

    public:

        explicit FrBaseComponent( QObject *parent = 0 );

    protected:

        void setError( const QString &value ) const;
        bool prologueCall( ) const;

        virtual bool clientFailed( ) const { return false; }

    public:

        QString error( ) const;

        bool failed( ) const;
        void setFailed( bool value ) const;

    signals:
        void failedChanged( bool value ) const;
    };

    class FrComponent: public FrBaseComponent {

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

        explicit FrComponent( QObject *parent = 0 );
        ~FrComponent( );

        FrClient *client( ) const;
        void setClient( FrClient *new_value );

    signals:

        void clientChanged( const fr::declarative::FrClient *value ) const;

    public slots:

    private slots:

        void onReady( bool value );

    };

}}

#endif // FRBASECOMPONENT_H
