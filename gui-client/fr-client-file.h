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

        QString path( ) const;
        void setPath( const QString &value );

        QString mode( ) const;
        void setMode( const QString &value );

    signals:

        void pathChanged( QString value ) const;
        void modeChanged( QString value ) const;

    public slots:

    };

}}
#endif // FRCLIENTFILE_H
