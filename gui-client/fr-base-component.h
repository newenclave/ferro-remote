#ifndef FRBASECOMPONENT_H
#define FRBASECOMPONENT_H

#include <QObject>
#include <QVariant>

namespace fr { namespace declarative {

    class FrBaseComponent: public QObject {

        Q_OBJECT

        Q_PROPERTY( bool failed
                    READ failed WRITE setFailed NOTIFY failedChanged )

        Q_PROPERTY( QString error READ error )
        Q_PROPERTY( bool ready READ ready NOTIFY readyChanged )

        mutable bool    ready_;
        mutable bool    failed_;
        mutable QString error_;


    public:

        explicit FrBaseComponent( QObject *parent = 0 );

    protected:

        void setError( const QString &value ) const;
        bool prologueCall( ) const;

        virtual bool clientFailed( ) const { return false; }

        void setReady( bool value ) const;

    public:

        bool ready( ) const;
        QString error( ) const;

        bool failed( ) const;
        void setFailed( bool value ) const;

    signals:
        void failedChanged( bool value ) const;
        void readyChanged( bool value ) const;
    };

}}

#endif // FRBASECOMPONENT_H
