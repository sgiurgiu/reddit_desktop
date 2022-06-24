import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import com.zergiu.reddit 1.0

ScrollView {
    id: tabPanelItem
    Layout.fillWidth: true
    Layout.fillHeight: true
    contentWidth: parent.width

    property string url
    property string name
    property SubredditTabButton tabButton
    property AccessToken accessToken

    Component.onCompleted: {
        postsModel.loadPosts(url, accessToken)
    }

    Connections {
        target: tabButton
        function onClicked() {
            //rect.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1);
            console.log("clicked")
        }
    }

    Column {
        id: postsColumn
        anchors.fill: parent
        Repeater {
            model: PostsListModel {
                id: postsModel
            }
            delegate: PostComponent {
                implicitWidth:  tabPanelItem.implicitWidth
            }
        }
    }

}
