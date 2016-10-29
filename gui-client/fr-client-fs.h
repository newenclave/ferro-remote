#ifndef FRFILESYSTEM_H
#define FRFILESYSTEM_H

#include <QObject>
#include "fr-client-component.h"

namespace fr { namespace client {

    namespace interfaces { namespace filesystem {
        struct info_data;
        struct directory_iterator_impl;
    }}

}}
//struct info_data {
//    bool is_exist;
//    bool is_directory;
//    bool is_empty;
//    bool is_regular;
//    bool is_symlink;
//};

namespace fr { namespace declarative {

    class FrFilesystemInfo: public QObject {

        Q_OBJECT

        Q_PROPERTY( bool exists      READ exists     )
        Q_PROPERTY( bool directory   READ directory  )
        Q_PROPERTY( bool empty       READ empty      )
        Q_PROPERTY( bool regular     READ regular    )
        Q_PROPERTY( bool symlink     READ symlink    )

        fr::client::interfaces::filesystem::info_data *data_;

    public:

        FrFilesystemInfo( const fr::client::interfaces
                                  ::filesystem::info_data &data );
        ~FrFilesystemInfo(  );

        bool exists( )    const;
        bool directory( ) const;
        bool empty( )     const;
        bool regular( )   const;
        bool symlink( )   const;
    };

    class FrClientFs: public FrClientComponent {

        struct impl;
        impl  *impl_;

        Q_OBJECT
        Q_PROPERTY( QString path
                    READ path WRITE setPath NOTIFY pathChanged)

    private:

        void on_reinit( );
        void on_ready( bool value );

        bool clientFailed( ) const;

    public:

        explicit FrClientFs(QObject *parent = 0);
        ~FrClientFs( );

        void setPath( const QString &new_value );
        QString path( ) const ;

        Q_INVOKABLE bool exists( const QString &path ) const;
        Q_INVOKABLE void mkdir( const QString &path ) const;
        Q_INVOKABLE void remove( const QString &path ) const;
        Q_INVOKABLE void removeAll( const QString &path ) const;

        Q_INVOKABLE QObject *info( const QString &path ) const;

        Q_INVOKABLE QObject *begin( const QString &path = QString( ) ) const;

        Q_INVOKABLE QByteArray readFile( const QString &path,
                                         unsigned maximum,
                                         quint64 from = 0 ) const;

        Q_INVOKABLE unsigned writeFile( const QString &path,
                                        const QByteArray &data,
                                        quint64 from = 0 ) const;

        fr::client::interfaces::filesystem
        ::directory_iterator_impl *begin_iterate( const QString &path  ) const;

    signals:

        void pathChanged( QString value );

    public slots:

    };

}}

#endif // FRFILESYSTEM_H
