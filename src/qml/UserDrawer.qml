import QtQuick 6.3
import QtQuick.Controls 6.3
import QtQuick.Layouts 6.3
import com.zergiu.reddit 1.0

Drawer {
    id: drawer
    y: menuBar.height
    width: 0.33 * appWindow.width
    height : appWindow.height - menuBar.height
    modal: inPortrait
    interactive: inPortrait
    position: inPortrait ? 0 : 1
    visible: !inPortrait
    signal subredditClicked(string name, string url)

    UserInformationProvider {
        id: userInformationProvider
        onSuccess : function(userInfo){
            headerPane.userInfo = userInfo
        }
        onFailure: {

        }
    }

    SubredditsListModel {
        id: subredditsListModel
    }

    Column {
        anchors.fill: parent
        spacing: 2
        Pane {
            id: headerPane
            property UserInfo userInfo
            z: 2
            width: parent.width
            ColumnLayout {
                   anchors.fill: parent
                   RowLayout{
                       Layout.alignment: Qt.AlignCenter
                        Label {
                            id: usernameLabel
                            text: headerPane.userInfo ? headerPane.userInfo.name : ""
                        }
                        Label {
                            id: karmaLabel
                            text: headerPane.userInfo ? headerPane.userInfo.humanLinkKarma + "-" + headerPane.userInfo.humanCommentKarma : ""
                        }
                   }
                    MenuSeparator {
                        parent: headerPane
                        width: parent.width
                        anchors.verticalCenter: parent.bottom
                        visible: !subredditSubscriptions.atYBeginning
                    }
            }
        }


        TreeView {
            id: subredditSubscriptions
            width: parent.width
            height: parent.height - headerPane.height
           // anchors.fill: parent
            //headerPositioning: TreeView.OverlayHeader

            model: subredditsListModel

            delegate: Item {
                id: treeDelegate

                implicitWidth: padding + label.x + label.implicitWidth + padding
                implicitHeight: label.implicitHeight * 1.5

                readonly property real indent: 20
                readonly property real padding: 5

                // Assigned to by TreeView:
                required property TreeView treeView
                required property bool isTreeNode
                required property bool expanded
                required property int hasChildren
                required property int depth
                required property bool selected
                required property bool multi
                required property string url
                required property string name


                TapHandler {
                    onTapped: {
                        console.log("tapped")

                        if(hasChildren) {
                            treeView.toggleExpanded(row)
                        } else {
                            drawer.subredditClicked(name,url)
                        }
                    }
                }

                Text {
                    id: indicator
                    visible: treeDelegate.isTreeNode && treeDelegate.hasChildren
                    x: padding + (treeDelegate.depth * treeDelegate.indent)
                    anchors.verticalCenter: label.verticalCenter
                    text: "▸"
                    rotation: treeDelegate.expanded ? 90 : 0
                }

                Text {
                    id: label
                    x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
                    width: treeDelegate.width - treeDelegate.padding - x
                    clip: false
                    text: model.display
                }
            }

            ScrollIndicator.vertical: ScrollIndicator { }

        }
    }


    function loadDrawerData(access_token) {
        userInformationProvider.loadUserInformation(access_token)
        subredditsListModel.loadSubscribedSubreddits(access_token)
    }
}
