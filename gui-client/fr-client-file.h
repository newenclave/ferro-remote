#ifndef FRCLIENTFILE_H
#define FRCLIENTFILE_H

#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientFile: public FrClientComponent
    {
        Q_OBJECT
        Q_PROPERTY( QString path
                    READ path WRITE setPath NOTIFY pathChanged )

        Q_PROPERTY( QString mode
                    READ mode WRITE setMode NOTIFY modeChanged )

        Q_PROPERTY( bool device
                    READ device WRITE setDevice NOTIFY deviceChanged )

        Q_PROPERTY( quint64 position READ position )

        Q_PROPERTY( bool events READ events
                    WRITE setEvents NOTIFY eventsChanged )

        Q_PROPERTY( bool opened READ opened )

        struct impl;
        impl  *impl_;

    private:

        virtual void on_reinit( );
        virtual void on_ready( bool value );
        virtual bool clientFailed( ) const;

    public:

        explicit FrClientFile( QObject *parent = 0 );
        ~FrClientFile(  );

    public:

        enum seek_whence {
             SeekSet = 0
            ,SeekCur = 1
            ,SeekEnd = 2
        };

        Q_ENUMS( seek_whence )

        QString path( ) const;
        void setPath( const QString &value );

        bool events( ) const;
        void setEvents( bool value );

        QString mode( ) const;
        void setMode( const QString &value );

        bool device( ) const;
        void setDevice( bool value );

        quint64 position( ) const;

        bool opened( ) const;
        void setOpened( bool value ) const;

    public:

        Q_INVOKABLE void iocontrol( unsigned code, quint64 data );

        Q_INVOKABLE QByteArray read( unsigned maximum ) const;
        Q_INVOKABLE unsigned write( const QByteArray &data ) const;

    signals:

        void pathChanged( QString value ) const;
        void modeChanged( QString value ) const;
        void openedChanged( bool value ) const;
        void eventsChanged( bool value ) const;
        void deviceChanged( bool value ) const;

        void fileEvent( unsigned error, QByteArray data, quint64 inter ) const;

    public slots:

        void open( );
        void close( );
        void seek( quint64 value, seek_whence whence = SeekSet ) const;

    };

}}
#endif // FRCLIENTFILE_H
