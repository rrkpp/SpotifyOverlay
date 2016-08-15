#ifndef SPOTIFYOVERLAY_H
#define SPOTIFYOVERLAY_H

#include "openvr.h"
#include <QMainWindow>
#include <QtNetwork>
#include <QUrl>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QScrollBar>

namespace Ui {
    class SpotifyOverlay;
}

struct SearchResult {
    int trackNum;

    QString trackUri;
    QString albumUri;

    QString song;
    QString artist;
    QString album;
};

class SpotifyOverlay : public QMainWindow
{
    Q_OBJECT

public:
    explicit SpotifyOverlay(QWidget* parent = 0);
    ~SpotifyOverlay();

    void initManifest();
    void onOverlayShown();
    bool isConnectedToSpotify();

    const char* appKey = "spotify.dashboard.overlay";

private:
    void initNetwork();

    void getTokenOAuth();
    void getTokenCSRF();
    void updateSearchResults();

    void play(int resultId);
    void pause();
    void unpause();

    void setIsPlaying(bool playing);

    Ui::SpotifyOverlay* ui;

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

    SearchResult* searchResults;
    const int maxResults = 25;      // Number of results requested from Spotify
    int maxResultsDisplayed = 5;    // Number of buttons available for search results
    int resultCount = 0;            // Number of results actually returned by Spotify
    int scrollOffset = 0;           // How far down the results list we should be scrolled

private slots:
    void oath_result();
    void csrf_result();

    void playToggle();
    void skip_prev();
    void skip_next();

    void search();
    void search_result();
    void search_selected();
    void search_scrollUp();
    void search_scrollDown();

    void openKeyboard();

    void getSpotifyStatus();
    void spotifyStatus_result();
    void trackInfo_result();
    void albumArt_result();
};

#endif // SPOTIFYOVERLAY_H
