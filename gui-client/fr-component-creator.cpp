
#include "fr-component-creator.h"
#include "fr-client-os.h"
#include "fr-client-fs.h"

#include <QSharedPointer>

namespace fr { namespace declarative {

    template<typename T>
    T * create_component( FrClient *client )
    {
        T *inst = new T;
        if( client ) {
            inst->setClient( client );
        }
        return inst;
    }

    FrComponentCreator::FrComponentCreator(QObject *parent) :
        QObject(parent)
    {

    }

    QObject *FrComponentCreator::newClient( QObject *parent ) const
    {
        return new FrClient( parent );
    }

    QObject *FrComponentCreator::newOs( FrClient *client ) const
    {
        return create_component<FrClientOS>( client );
    }

    QObject *FrComponentCreator::newFs( FrClient *client,
                                        const QString &path ) const
    {
        FrClientFs *inst = create_component<FrClientFs>( client );
        if( path != "" ) {
            inst->setPath( path );
        }
        return inst;
    }

}}
