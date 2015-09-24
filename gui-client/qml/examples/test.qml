import QtQuick 2.0

import Fr.Client 1.0

Rectangle {
    id: mainWindow
    FrClient {
        id: frClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }
}


