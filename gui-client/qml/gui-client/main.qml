import QtQuick 2.0
import QtQuick.Controls 1.1

import fr.client 1.0

Rectangle {

    width: 480
    height: 640

    FrClient {
        id: generalClient
    }

    Column {
        anchors.margins: 10
        spacing: 10
        FrClientOS {
            id: osIface
            client: generalClient
        }
        Row {
            anchors.margins: 10
            spacing: 10
            TextField {
                id: address
                width: 200
            }

            Button {
                onClicked: {
                    generalClient.connect( address.text )
                    //osIface.client = generalClient
                }
            }
        }
        Row {
            anchors.margins: 10
            spacing: 10
            TextField {
                id: command
                width: 200
            }
            Button {
                id: run
                onClicked: osIface.execute( command.text )
            }
        }
    }

}
