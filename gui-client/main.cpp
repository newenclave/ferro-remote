#include <iostream>

#include <QtQml>
#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include "client-core/fr-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "frclient.h"

void usage( )
{
    std::cout << "usage: gui-client /path/to/qml/file.qml" << std::endl;
}

int main( int argc, char *argv[] )
{
    if( argc < 2 ) {
        usage( );
        return 1;
    }

    const char *path = argv[1];

    vtrc::common::pool_pair pp( 1, 1 );

    fr::client::core::client_core cc(pp);

    QGuiApplication app( argc, argv );

    QtQuick2ApplicationViewer viewer;

    qmlRegisterType<fr::declarative::FrClient>( "my.test", 1, 0, "FrClient" );

    viewer.setMainQmlFile( QString(path) );
    viewer.setClearBeforeRendering(false);
    viewer.showExpanded( );

    return app.exec( );
}

