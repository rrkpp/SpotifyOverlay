#include "spotifyoverlay.h"
#include "openvr.h"
#include "openvroverlaycontroller.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SpotifyOverlay *w = new SpotifyOverlay;

    COpenVROverlayController::SharedInstance()->Init();
    COpenVROverlayController::SharedInstance()->SetWidget(w);

    w->initManifest();
    //w->show();

    return a.exec();
}
