#include <iostream>

#include <QtQml>
#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include "vtrc-common/vtrc-exception.h"

#include "frclient.h"
#include "application-data.h"

namespace fr {  namespace declarative {
    application_data global_app_data;
}}

void usage( )
{
    std::cout << "usage: gui-client /path/to/qml/file.qml" << std::endl;
}

int main( int argc, char *argv[] )
{ try {

    if( argc < 2 ) {
        usage( );
        return 1;
    }

    const char *path = argv[1];

    fr::declarative::global_app_data.reset_pools( 1, 1 );

    QGuiApplication app( argc, argv );

    QtQuick2ApplicationViewer viewer;

    qmlRegisterType<fr::declarative::FrClient>( "fr.client", 1, 0, "FrClient" );

    viewer.rootContext( )->setContextProperty( "pool", &app );
    viewer.setMainQmlFile( QString(path) );
    viewer.setClearBeforeRendering(false);
    viewer.showExpanded( );

    return app.exec( );

} catch( const vtrc::common::exception &ex ) {
    std::cerr << "Protocol client error: "
              << ex.what( ) << ": "
              << ex.additional( )
              << "\n";
    return 10;
} catch( const std::exception &ex ) {
    std::cerr << "General client error: " << ex.what( ) << "\n";
    return 10;
} }


