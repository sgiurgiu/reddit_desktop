import QtQuick.Dialogs 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQml 6.0

Dialog {
    title: qsTr("Reddit Login")
    modal: true
    closePolicy: Popup.CloseOnEscape
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Apply
    anchors.centerIn: parent
    parent: Overlay.overlay

    GridLayout {
        columns: 2
        Label {
            text: qsTr("Username:")
        }
        TextField {
            id: usernameText
        }
        Label {
            text: qsTr("Password:")
        }
        TextField {
            id: passwordText
            echoMode: TextField.Password
        }
        Label {
            text: qsTr("Client Id:")
        }
        TextField {
            id: clientIdText
        }
        Label {
            text: qsTr("Secret:")
        }
        TextField {
            id: secretText
        }
        Label {
            text: qsTr("Website:")
        }
        TextField {
            id: websiteText
        }
        Label {
            text: qsTr("App Name:")
        }
        TextField {
            id: appNameText
        }
        Button {
            id: testButton
            text: qsTr("Test")
            onClicked: {
                standardButton(Dialog.Ok).enabled = true
                standardButton(Dialog.Apply).enabled = true
            }
        }
    }

    Component.onCompleted : {
        standardButton(Dialog.Ok).enabled = false
        standardButton(Dialog.Apply).enabled = false
    }
}
