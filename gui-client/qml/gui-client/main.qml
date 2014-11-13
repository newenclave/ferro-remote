import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Window 2.1

Rectangle {
    id: main
    height: 640
    width:  480
    Row {
        anchors.margins: 4
        anchors.left: main.left
        anchors.right: main.right
        anchors.top: main.top

        spacing: 4
        TextField {
        }

        Button {
            id: connect_button
            text: qsTr("connect")
        }
    }
}
