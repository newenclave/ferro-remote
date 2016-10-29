import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    id: parentRect;
    height: 80
    width:  350
    color:  "white"

    FrClient {
        id: remoteClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }

    Item {
        FrClientOS {
            id: shellExecutor
            client: remoteClient
        }
        Connections {
            target: runButton
            onClicked: shellExecutor.execute( command.text )
        }
    }

    Column {
        anchors.centerIn: parentRect
        spacing: 10

        TextField {
            id: command
            height: 20
            width:  parentRect.width - 10
            text: "echo `uname -a`"
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
