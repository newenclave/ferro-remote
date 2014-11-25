#ifndef FRCLIENT_H
#define FRCLIENT_H

#include <QObject>
#include <QtGui/QGuiApplication>

namespace fr { namespace declarative {

    class FrClient : public QObject
    {
        Q_OBJECT
        Q_PROPERTY( bool connected
                    READ connected
                    NOTIFY connectedChanged )

        Q_PROPERTY( QGuiApplication *app
                    READ app WRITE setApp
                    NOTIFY appChanged )

        struct impl;
        impl  *impl_;

    public:

        explicit FrClient(QObject *parent = 0);
        ~FrClient( );

    public:

        bool connected( ) const;
        QGuiApplication *app( );
        void setApp(QGuiApplication *app);

    signals:
        void connectedChanged( bool new_state );

    public slots:
        void connect( const QString &server );
        void disconnect( );
    };

}}

#endif // FRCLIENT_H
