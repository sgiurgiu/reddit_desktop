#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QVariant>
#include "accesstokenprovider.h"

Q_DECLARE_METATYPE(user)
Q_DECLARE_METATYPE(access_token)


int main(int argc, char** argv)
{

    QCoreApplication::setOrganizationName("Zergiu");
    QCoreApplication::setOrganizationDomain("zergiu.com");
    QCoreApplication::setApplicationName("Reddit Desktop");
    QCoreApplication::setApplicationVersion(REDDITDESKTOP_VERSION);

    qmlRegisterType<AccessToken>("com.zergiu.reddit", 1, 0,"AccessToken");
    qmlRegisterType<AccessTokenProvider>("com.zergiu.reddit", 1, 0, "AccessTokenProvider");


    QGuiApplication app(argc, argv);
    QNetworkAccessManager nas(&app);
    app.setProperty("NetworkAccessManager",QVariant::fromValue(&nas));
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/reddit_desktop_qml/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
