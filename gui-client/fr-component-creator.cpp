#include "fr-component-creator.h"

namespace fr { namespace declarative {

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
        FrClientOS *inst = new FrClientOS;
        if( client ) {
            inst->setClient( client );
        }
        return inst;
    }

}}
