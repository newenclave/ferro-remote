#ifndef FRCLIENT_H
#define FRCLIENT_H

#include <QObject>
#include <QtGui/QGuiApplication>

namespace fr {

    namespace client { namespace core {
        class client_core;
    }}

namespace declarative {

    class FrClient : public QObject
    {
        Q_OBJECT
        Q_PROPERTY( bool ready READ ready )

        struct impl;
        impl  *impl_;

    public:

        explicit FrClient(QObject *parent = 0);
        ~FrClient( );

    public:

        bool ready( ) const;

        client::core::client_core &core_client( );

    signals:

        void connected( );
        void disconnected( );
        void channelReady( );
        void initError( const QString &message );

    public slots:
        void connect( const QString &server );
        void disconnect( );
    };

}}

#endif // FRCLIENT_H
