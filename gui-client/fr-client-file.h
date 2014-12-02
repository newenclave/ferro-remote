#ifndef FRCLIENTFILE_H
#define FRCLIENTFILE_H

#include "fr-base-component.h"


namespace fr { namespace declarative {

    class FrClientFile: public FrBaseComponent
    {
        Q_OBJECT
        Q_PROPERTY( QString path
                    READ path WRITE setPath NOTIFY pathChanged)

        Q_PROPERTY( QString mode
                    READ mode WRITE setMode NOTIFY modeChanged)

        Q_PROPERTY( quint64 position READ position )

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
             POS_SEEK_SET = 0
            ,POS_SEEK_CUR = 1
            ,POS_SEEK_END = 2
        };
        Q_ENUMS( seek_whence )

        QString path( ) const;
        void setPath( const QString &value );

        QString mode( ) const;
        void setMode( const QString &value );

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

    public slots:

        void open( );
        void close( );
        void seek( quint64 value, seek_whence whence = POS_SEEK_SET ) const;

    };

}}
#endif // FRCLIENTFILE_H
