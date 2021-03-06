#include <iostream>

#include <QtQml>
#include <QtGui/QGuiApplication>

#include "qtquick2applicationviewer.h"

#include "vtrc-common/vtrc-exception.h"

#include "application-data.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "fr-component-creator.h"

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

    using namespace fr::declarative;

    global_app_data.reset_pools( 1, 1 );

    QGuiApplication app( argc, argv );

    QtQuick2ApplicationViewer viewer;

    FrComponentCreator cc;

    FrComponentCreator::registerFrClasses( );
    cc.setContextProperty( viewer );

    viewer.setMainQmlFile( QString(path) );
    viewer.setClearBeforeRendering(false);
    viewer.showExpanded( );

    int res = app.exec( );

    global_app_data.pools( ).stop_all( );
    global_app_data.pools( ).join_all( );

    return res;

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


