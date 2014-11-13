#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include "client-core/fr-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

int main(int argc, char *argv[])
{
    vtrc::common::pool_pair pp( 1, 1 );

    fr::client::core::client_core cc(pp);

    QGuiApplication app( argc, argv );

    QtQuick2ApplicationViewer viewer;
    viewer.setMainQmlFile(QStringLiteral("qml/gui-client/main.qml"));
    viewer.showExpanded();

    return app.exec( );
}
