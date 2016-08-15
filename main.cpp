#include "spotifyoverlay.h"
#include "openvr.h"
#include "openvroverlaycontroller.h"
#include <QApplication>

void logMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QDateTime date;
    QString text = msg;
    text.prepend("[" + date.currentDateTime().toString() + "]: ");

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

    QTextStream ts(&file);
    ts << text << '\n' << flush;
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(logMsgHandler); // Enable logging

    QApplication a(argc, argv);
    SpotifyOverlay *w = new SpotifyOverlay;

    COpenVROverlayController::SharedInstance()->Init();
    COpenVROverlayController::SharedInstance()->SetWidget(w);

    w->initManifest();
    //w->show();

    return a.exec();
}
