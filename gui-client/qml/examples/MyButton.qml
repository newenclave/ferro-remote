 // Button.qml
import QtQuick 2.0

Rectangle {

    width: 80
    height: 25
    color: "lightgray"

    property string text: "Button"

    property bool enabled: true

    signal clicked( )

    border.color: "gray"
    border.width: 1

    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        text: parent.text
        color: parent.enabled ? "black" : "gray"
    }

    MouseArea {
        enabled: parent.enabled
        anchors.fill: parent
        onClicked: {
           parent.clicked( )
        }
        onPressed: { parent.color = "gray" }
        onReleased: { parent.color = "lightgray" }
    }
}

