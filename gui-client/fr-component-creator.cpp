
#include "fr-component-creator.h"
#include "fr-client-os.h"
#include "fr-client-fs.h"

#include <QSharedPointer>
#include <QQmlEngine>

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

}}
