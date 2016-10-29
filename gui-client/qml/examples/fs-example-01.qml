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

    MessageDialog {
        id: messageDialog
        title: "?"
        text: "File exists.\nPress Yes for replace and execute.\n"
              + "Press No for cancell.\nPress Discard for remove old file."
        standardButtons: StandardButton.Yes
                       | StandardButton.No
                       | StandardButton.Discard
        Component.onCompleted: visible = false
    }

    Item {

        property string filePath: "/tmp/fsexample.sh"
        id: shellExec

        FrClientFs {
            id: remoteFs
            client: remoteClient
            path: "/"
        }

        FrClientOS {
            id: executor
            client: remoteClient
        }

        function runShell( )
        {
            remoteFs.writeFile( filePath, command.text )
            executor.execute( "chmod +x " + filePath )
            executor.execute( filePath )
        }

        Connections {
            target: runButton
            onClicked: {
                if( remoteFs.exists( shellExec.filePath ) ) {
                    messageDialog.visible = true;
                } else {
                    shellExec.runShell( )
                }
            }
        }

        Connections {
            target: messageDialog
            onYes: {
                console.log( "Execute script..." )
                shellExec.runShell( )
            }
            onDiscard: {
                console.log( "Removing " + shellExec.filePath )
                remoteFs.remove( shellExec.filePath )
            }
        }


    }

    Column {
        anchors.centerIn: parentRect
        spacing: 10

        TextArea {
            id: command
            height: parentRect.height - 40
            width:  parentRect.width - 10
            text: "#!/bin/bash"
        }

        Button {
            id: runButton
            width:  command.width
            text: "Remote run"
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
