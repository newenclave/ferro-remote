import QtQuick 2.0

import fr.client 1.0

Rectangle {
    FrClient {
        id: generalClient
    }
    MouseArea {
        anchors.fill: parent
        onClicked: { generalClient.connect( "127.0.0.1:12345" ); }
    }
}
