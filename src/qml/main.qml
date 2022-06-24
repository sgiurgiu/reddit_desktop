import QtQuick 6.0
import QtQuick.Window 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQml 6.0
import reddit_desktop_qml 1.0
import Qt.labs.settings 1.1
import com.zergiu.reddit 1.0

ApplicationWindow
{
    id: appWindow
    visible: true
    width: 800
    height: 600
    title: qsTr("Reddit Desktop")
    property AccessToken accessToken
    readonly property bool inPortrait: appWindow.width < appWindow.height

    Component.onCompleted: {
        busyIndicator.running = true
        accessTokenProvider.refreshAccessToken()
    }

    AccessTokenProvider {
        id: accessTokenProvider
        onSuccess : function (token) {
            busyIndicator.running = false
            appWindow.accessToken = token
            drawer.loadDrawerData(token)
        }

        onFailure: {
            loginDialog.open()
        }
    }

    LoginDialog {
       id: loginDialog
       onAccepted:  {
           console.log("Ok clicked")
           busyIndicator.running = false
       }
       onRejected: {
           console.log("Cancel clicked")
           busyIndicator.running = false
       }
    }

    Action {
        id: exitAction
        text: qsTr("E&xit")
        shortcut: StandardKey.Quit
        icon.name: "application-exit"
        onTriggered: Qt.quit();
    }

    menuBar: MenuBar {
      id: menuBar
      Menu {
          title: qsTr("File")
          MenuItem {
              action: exitAction
          }
      }
    }

    Settings {
        property alias x: appWindow.x
        property alias y: appWindow.y
        property alias width: appWindow.width
        property alias height: appWindow.height
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        running : false
        width: parent.width / 2
        height: parent.height / 2
    }

    UserDrawer {
        id: drawer
        onSubredditClicked: function(name, url){
            console.log("subreddit clicked "+name+", "+url)
            let tabButtonComponent = Qt.createComponent("SubredditTabButton.qml")
            let tabButton = tabButtonComponent.createObject(bar, {text:name})
            bar.addItem(tabButton)

            let tabPageComponent = Qt.createComponent("SubredditPage.qml")
            let tabPage = tabPageComponent.createObject(panels,
                                                        {url:url,
                                                         name:name,
                                                        tabButton:tabButton,
                                                        accessToken: appWindow.accessToken})

            om.append(tabPage)
            bar.currentIndex = bar.count - 1
        }
    }

    TabBar {
        id: bar
        anchors.left : parent.left
        anchors.right : parent.right
        anchors.leftMargin: !inPortrait ? drawer.width : undefined
        //width: appWindow.width - (!inPortrait? 0 : drawer.width )

    }

    StackLayout {
        id : panels
        y : bar.height + 2 + menuBar.height
        height: parent.height - bar.height - menuBar.height - 2
        anchors.top: bar.bottom
        anchors.left : parent.left
        anchors.right : parent.right
        anchors.bottom : parent.bottom

        anchors.leftMargin: !inPortrait ? drawer.width : undefined
        currentIndex: bar.currentIndex
        Repeater {
            model: ObjectModel {
                id: om
            }
        }
    }


}
