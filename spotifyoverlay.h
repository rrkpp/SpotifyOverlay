#ifndef SPOTIFYOVERLAY_H
#define SPOTIFYOVERLAY_H

#include "openvr.h"
#include <QMainWindow>
#include <QtNetwork>
#include <QUrl>
#include <QJsonArray>
#include <QVBoxLayout>

namespace Ui {
    class SpotifyOverlay;
}

class SpotifyOverlay : public QMainWindow
{
    Q_OBJECT

public:
    explicit SpotifyOverlay(QWidget* parent = 0);
    ~SpotifyOverlay();

    void initManifest();
    void pause();
    void unpause();

    const char* appKey = "spotify.dashboard.overlay";

private:
    void initNetwork();
    void getTokenOAuth();
    void getTokenCSRF();
    void getSpotifyStatus();
    void setIsPlaying(bool playing);

    Ui::SpotifyOverlay* ui;
    QVBoxLayout searchLayout;

    QNetworkAccessManager* netMgr;
    QSslConfiguration sslConfig;

    QString key_OAuth;
    QString key_CSRF;

    QFile manifestFile;
    QJsonObject manifest;

    QIcon playIcon;
    QIcon pauseIcon;

    bool isPlaying = false;
    bool keyboardOpen = false;
    const int maxResults = 5;

private slots:
    void oath_result();
    void csrf_result();

    void play(QString uri);
    void playToggle();
    void skip_prev();
    void skip_next();

    void search();
    void search_result();
    void search_selected();

    void openKeyboard();

    void updateSpotifyStatus();
    void updateTrackInfo();
    void albumArtDownloaded();
};

#endif // SPOTIFYOVERLAY_H
