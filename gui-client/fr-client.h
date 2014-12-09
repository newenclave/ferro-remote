#ifndef FRCLIENT_H
#define FRCLIENT_H

#include <QObject>
#include <QtGui/QGuiApplication>

#include "fr-base-component.h"

namespace fr {

    namespace client { namespace core {
        class client_core;
    }}

namespace declarative {

    class FrClient: public FrBaseComponent {

        Q_OBJECT
        Q_PROPERTY( bool ready READ ready NOTIFY readyChanged )

//        Q_PROPERTY( QString sessionId
//                    READ sessionId WRITE setSessionId
//                    NOTIFY sessionIdChanged )

//        Q_PROPERTY( QString sessionKey
//                    READ sessionKey WRITE setSessionKey
//                    NOTIFY sessionKeyChanged )

        struct impl;
        impl  *impl_;

    public:

        explicit FrClient(QObject *parent = 0);
        ~FrClient( );

    public:

        bool ready( ) const;

        client::core::client_core &core_client( );

        QString sessionId( ) const;
        void setSessionId( const QString &sid );

        QString sessionKey( ) const;
        void setSessionKey( const QString &key );


    signals:

        void connected( );
        void disconnected( );
        void channelReady( );
        void initError( const QString &message );
        void readyChanged( bool value );

        void sessionIdChanged( QString sid ) const;
        void sessionKeyChanged( QString key ) const;

    public slots:

        void connect( const QString &server );
        void disconnect( );
    };

}}

#endif // FRCLIENT_H
