#include <QSharedPointer>
#include <QQmlEngine>
#include <QtQml>

#include "fr-component-creator.h"
#include "fr-client-os.h"
#include "fr-client-fs.h"
#include "fr-client-file.h"
#include "fr-client-gpio.h"
#include "fr-client-i2c.h"
#include "fr-client-spi.h"

#include "qtquick2applicationviewer.h"

namespace fr { namespace declarative {

    template<typename T>
    T * create_component( FrClient *client )
    {
        T *inst = new T;
        if( client ) {
            inst->setClient( client );
        }
        //QQmlEngine::setObjectOwnership( inst, QQmlEngine::JavaScriptOwnership );
        inst->deleteLater( );
        return inst;
    }

    FrComponentCreator::FrComponentCreator(QObject *parent) :
        QObject(parent)
    {

    }

    QObject *FrComponentCreator::newClient( QObject *parent )
    {
        return new FrClient( parent );
    }

    QObject *FrComponentCreator::newOs( FrClient *client )
    {
        QObject *inst = create_component<FrClientOS>( client );
        return inst;
    }

    QObject *FrComponentCreator::newFs( FrClient *client, const QString &path )
    {
        FrClientFs *inst = create_component<FrClientFs>( client );
        if( path != "" ) {
            inst->setPath( path );
        }
        return inst;
    }

    QObject *FrComponentCreator::newFile( FrClient *client,
                                          const QString &path,
                                          const QString &mode )
    {
        FrClientFile *inst = create_component<FrClientFile>( client );
        if( path != "" ) {
            inst->setPath( path );
            inst->setMode( mode );
        }
        return inst;
    }

    QObject *FrComponentCreator::newGpio( FrClient *client, unsigned index )
    {
        FrClientGpio *inst = create_component<FrClientGpio>( client );
        if( index != 0xFFFFFFFF ) {
            inst->setIndex( index );
        }
        return inst;
    }

    QObject *FrComponentCreator::newI2c( FrClient *client, unsigned /*bus*/ )
    {
        FrClientI2c *inst = create_component<FrClientI2c>( client );
        return inst;
    }

    void FrComponentCreator::setContextProperty( QtQuick2ApplicationViewer &v )
    {
        v.rootContext( )
            ->setContextProperty( "frComponents", QVariant::fromValue( this ) );
    }

    void FrComponentCreator::registerFrClasses( )
    {
        qmlRegisterType<FrClient>    ( "Fr.Client", 1, 0, "FrClient"     );
        qmlRegisterType<FrClientOS>  ( "Fr.Client", 1, 0, "FrClientOS"   );
        qmlRegisterType<FrClientFs>  ( "Fr.Client", 1, 0, "FrClientFs"   );
        qmlRegisterType<FrClientFile>( "Fr.Client", 1, 0, "FrClientFile" );
        qmlRegisterType<FrClientGpio>( "Fr.Client", 1, 0, "FrClientGpio" );
        qmlRegisterType<FrClientI2c> ( "Fr.Client", 1, 0, "FrClientI2c"  );
        qmlRegisterType<FrClientSPI> ( "Fr.Client", 1, 0, "FrClientSPI"  );
    }

}}
