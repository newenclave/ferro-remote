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
        FrClientFs {
            id: remoteFs
            client: remoteClient
            path: "/"
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
            text: ""
            Connections {
                target: openFile
                onClicked: {
                    var f = remoteFs.openFile( filePath.text, "r" )
                    testField.text = ""
                    while( true ) {
                        var t = f.read( 4096 );
                        if( t.toString().length === 0 ) {
                            break;
                        }
                        testField.text += t;
                    }
                    f.close( )
                }
            }
            Connections {
                target: saveButton
                onClicked: {
                    var f = remoteFs.openFile( filePath.text, "w" )
                    var begin = 0;
                    while( true ) {
                        var res = f.write( testField.text.slice( begin ) );
                        if( res === 0 ) break;
                        begin += res
                    }
                    f.close( )
                }
            }
        }

        Button {
            id: saveButton
            width:  testField.width
            text: "Save file"
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
