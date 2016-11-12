import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.2
import Fr.Client 1.0

Rectangle {

    id: parentRect;
    height: 350
    width:  350
    color:  "white"

    FrClient {
        id: remoteClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }

    Item {
        FrClientFile {
            id: remoteFile
            client: remoteClient
        }
        Connections {
            target: openFile
            onClicked: {
                remoteFile.close( )
                remoteFile.path = filePath.text
                remoteFile.events = true
                remoteFile.open( )
            }
        }
    }

    Column {
        anchors.centerIn: parentRect
        spacing: 10

        Row {
            spacing: 10
            TextField {
                id: filePath
                width: testField.width - 80
                text: "/dev/random"
            }

            Button {
                id: openFile
                text: "Open file"
            }
        }

        TextArea {
            id: testField
            height: parentRect.height - 80
            width:  parentRect.width - 10

            Connections {
                target: openFile
                onClicked: {
                    testField.text = ""
                }
            }

            Connections {
                target: remoteFile
                onFileEvent: {
                    testField.text = testField.text + data
                }
            }
        }
    }


    Connections {
        target: remoteClient
        onTryConnect:   console.log( "remoteClient: I'm trying!!!" )
        onConnected:    console.log( "remoteClient: connected" )
        onChannelReady: console.log( "remoteClient: channel ready" )
        onDisconnected: console.log( "remoteClient: disconnected " )
        onInitError:    console.log( "remoteClient: init error " + message )
    }
}
