import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Drawer {
    y: menuBar.height
    width: 0.33 * appWindow.width
    height : appWindow.height - menuBar.height
    modal: inPortrait
    interactive: inPortrait
    position: inPortrait ? 0 : 1
    visible: !inPortrait

    ListView {
        id: subredditSubscriptions
        anchors.fill: parent
        headerPositioning: ListView.OverlayHeader
        header: Pane {
            id: header
            z: 2
            width: parent.width
            Label {
                text: qsTr("Some Label here")
            }

            MenuSeparator {
                    parent: header
                    width: parent.width
                    anchors.verticalCenter: parent.bottom
                    visible: !subredditSubscriptions.atYBeginning
            }
        }

        model: 10

        delegate: ItemDelegate {
            text: qsTr("Title %1").arg(index + 1)
            width: subredditSubscriptions.width
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
