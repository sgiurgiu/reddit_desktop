#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QVariant>
#include "accesstokenprovider.h"
#include "userinformationprovider.h"
#include "entities/userinfo.h"
#include "subredditslistmodel.h"
#include "postslistmodel.h"

int main(int argc, char** argv)
{

    QCoreApplication::setOrganizationName("Zergiu");
    QCoreApplication::setOrganizationDomain("zergiu.com");
    QCoreApplication::setApplicationName("Reddit Desktop");
    QCoreApplication::setApplicationVersion(REDDITDESKTOP_VERSION);

    qmlRegisterType<AccessToken>("com.zergiu.reddit", 1, 0,"AccessToken");
    qmlRegisterType<AccessTokenProvider>("com.zergiu.reddit", 1, 0, "AccessTokenProvider");
    qmlRegisterType<UserInformationProvider>("com.zergiu.reddit", 1, 0, "UserInformationProvider");
    qmlRegisterType<UserInfo>("com.zergiu.reddit", 1, 0, "UserInfo");
    qmlRegisterType<SubredditsListModel>("com.zergiu.reddit", 1, 0, "SubredditsListModel");
    qmlRegisterType<PostsListModel>("com.zergiu.reddit", 1, 0, "PostsListModel");


    QGuiApplication app(argc, argv);
    QNetworkAccessManager nas(&app);
    app.setProperty("NetworkAccessManager",QVariant::fromValue(&nas));
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/com/zergiu/reddit/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
