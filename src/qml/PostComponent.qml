import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import com.zergiu.reddit 1.0

Item {
    implicitHeight: row.implicitHeight* 1.2
    required property var post

    RowLayout {
        id: row
        Column {
            Layout.fillHeight: true
            Button {
                id: up
                text:"Up"
            }
            Label {
                text : post.score
            }
            Button {
                id: down
                text:"Down"
            }
        }

        Image {
            id : thumbnail
            source: post.thumbnail
        }

        Column {
            Layout.fillHeight: true

            RowLayout {
                spacing: 5
                Label {
                    text: post.subreddit
                }

                Label {
                    text: "Posted by "+post.author +" "+post.humanReadableTimeDifference
                }
            }

            Label {
                id: postTitle
                text: post.title
                wrapMode: Text.WordWrap
            }

            Label {
                text: "("+post.domain+")"
            }
        }
    }
}
