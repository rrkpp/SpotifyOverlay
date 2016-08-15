#include "spotifyoverlay.h"
#include "openvroverlaycontroller.h"
#include "ui_spotifyoverlay.h"
#include <Windows.h>

SpotifyOverlay::SpotifyOverlay(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::SpotifyOverlay),
    manifestFile(QCoreApplication::applicationDirPath() + "/spotify.vrmanifest")
{
    ui->setupUi(this);

    initNetwork();      // Set up network manager and SSL config.
    getTokenOAuth();    // When OAuth token is returned, the function to retrieve the CSRF token is called automatically.

    // Initialize Play/Pause icons.
    playIcon = QIcon(":/img/play.png");
    pauseIcon = QIcon(":/img/pause.png");

    // Set up search result buttons.
    searchResults = new SearchResult[maxResults];

    for (int i=0; i < maxResultsDisplayed; i++)
    {
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setSpacing(0);
        layout->setMargin(0);

        QLabel* titleLabel = new QLabel();
        titleLabel->setObjectName("trackLabel");
        titleLabel->setFont(QFont("Open Sans", 12, 500));
        titleLabel->setMaximumWidth(250);
        titleLabel->setStyleSheet("color: white;");

        QLabel* artistLabel = new QLabel();
        artistLabel->setObjectName("artistLabel");
        artistLabel->setFont(QFont("Open Sans", 12));
        artistLabel->setMaximumWidth(250);
        artistLabel->setStyleSheet("color: white;");

        QLabel* albumLabel = new QLabel();
        albumLabel->setObjectName("albumLabel");
        albumLabel->setFont(QFont("Open Sans", 12));
        albumLabel->setMaximumWidth(250);
        albumLabel->setStyleSheet("color: white;");

        layout->addWidget(titleLabel);
        layout->addWidget(artistLabel);
        layout->addWidget(albumLabel);

        QPushButton* btn = this->findChild<QPushButton*>("searchResultBtn" + QString::number(i + 1));

        if (btn)
        {
            btn->setLayout(layout);
            connect(btn, SIGNAL(released()), this, SLOT(search_selected()));
        }
    }

    // Connect SIGNALs/SLOTs.
    connect(ui->playButton, SIGNAL(released()), this, SLOT(playToggle()));
    connect(ui->prevButton, SIGNAL(released()), this, SLOT(skip_prev()));
    connect(ui->nextButton, SIGNAL(released()), this, SLOT(skip_next()));
    connect(ui->searchButton, SIGNAL(released()), this, SLOT(openKeyboard()));

    connect(ui->scrollup, SIGNAL(released()), this, SLOT(search_scrollUp()));
    connect(ui->scrolldown, SIGNAL(released()), this, SLOT(search_scrollDown()));
}

SpotifyOverlay::~SpotifyOverlay()
{
    delete ui;
    delete netMgr;
    delete searchResults;
}

void SpotifyOverlay::initManifest()
{
    // Manifest-related code taken directly from LibreVR/Revive Project.
    if (!vr::VRApplications()->IsApplicationInstalled("appKey"))
    {
        QJsonObject strings, english;
        english["name"] = "Spotify Overlay";
        english["description"] = "Search tracks and control Spotify playback in VR!";
        strings["en_us"] = english;

        QJsonObject overlay;
        overlay["app_key"] = appKey;
        overlay["launch_type"] = "binary";
        overlay["binary_path_windows"] = "SpotifyOverlay.exe";
        overlay["arguments"] = "";
        overlay["image_path"] = "spotify.png";
        overlay["is_dashboard_overlay"] = true;
        overlay["strings"] = strings;

        QJsonArray apps;
        apps.append(overlay);
        manifest["applications"] = apps;

        if (manifestFile.open(QIODevice::WriteOnly))
        {
            QJsonDocument doc(manifest);
            QByteArray array = doc.toJson();

            manifestFile.write(array);
            manifestFile.close();
        }
    }

    QFileInfo info(manifestFile);
    QString path = QDir::toNativeSeparators(info.absoluteFilePath());

    vr::VRApplications()->AddApplicationManifest(path.toUtf8());
    vr::VRApplications()->SetApplicationAutoLaunch(appKey, true);
}

void SpotifyOverlay::onOverlayShown()
{
    // If we haven't already established a connection to Spotify,
    // try again. This way, the overlay will continue to work even
    // if Spotify wasn't running at the time of application start.
    if (key_OAuth.length() <= 0)
    {
        getTokenOAuth();
    }
    else if (key_CSRF.length() <= 0)
    {
        getTokenCSRF();
    }
    else
    {
        getSpotifyStatus();
    }
}

bool SpotifyOverlay::isConnectedToSpotify()
{
    return key_OAuth.length() > 0 && key_CSRF.length() > 0;
}

void SpotifyOverlay::initNetwork()
{
    // Initialize network manager and SSL config.
    netMgr = new QNetworkAccessManager(this);

    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2);
}

void SpotifyOverlay::getTokenOAuth()
{
    // Send a request to Spotify for an OAuth token.
    // When the request returns, a new request for a CSRF
    // token will be sent as well.
    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://open.spotify.com/token"));

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(oath_result()));
}

void SpotifyOverlay::oath_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        key_OAuth = obj.value("t").toString();

        qDebug().noquote() << "Retrieved OAuth key of length " + QString::number(key_OAuth.length());

        getTokenCSRF();
    }
}

void SpotifyOverlay::getTokenCSRF()
{
    // Send a request to the local Spotify HTTPS
    // server for a CSRF token.
    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/simplecsrf/token.json")); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(csrf_result()));
}

void SpotifyOverlay::csrf_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        key_CSRF = obj.value("token").toString();

        qDebug().noquote() << "Retrieved CSRF key of length " + QString::number(key_CSRF.length());

        if (key_OAuth.length() > 0 && key_CSRF.length() > 0)
            getSpotifyStatus();
    }
}

void SpotifyOverlay::updateSearchResults()
{
    for (int i=0; i < maxResultsDisplayed; i++)
    {
        SearchResult sr = searchResults[i + scrollOffset];
        QPushButton* btn = this->findChild<QPushButton*>("searchResultBtn" + QString::number(i + 1));

        if (btn)
        {
            QLabel* trackLabel = btn->findChild<QLabel*>("trackLabel");
            QLabel* artistLabel = btn->findChild<QLabel*>("artistLabel");
            QLabel* albumLabel = btn->findChild<QLabel*>("albumLabel");

            QFontMetrics fm_track(trackLabel->font());
            QString trackText = fm_track.elidedText(sr.song, Qt::ElideRight, trackLabel->width());

            QFontMetrics fm_artist(artistLabel->font());
            QString artistText = fm_artist.elidedText(sr.artist, Qt::ElideRight, artistLabel->width());

            QFontMetrics fm_album(albumLabel->font());
            QString albumText = fm_album.elidedText(sr.album, Qt::ElideRight, albumLabel->width());

            trackLabel->setText(trackText);
            artistLabel->setText(artistText);
            albumLabel->setText(albumText);
            btn->setProperty("resultId", i + scrollOffset);
        }
    }
}



/*
 * =============================================
 * PLAYBACK FUNCTIONS
 * =============================================
 */

void SpotifyOverlay::play(int resultId)
{
    SearchResult sr = searchResults[resultId];

    if (sr.trackUri.length() <= 0)
        return;

    setIsPlaying(true);

    // First, send a request to the local Spotify server
    // for Spotify to play the selected track.
    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/play.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&uri=" + QUrl::toPercentEncoding(sr.trackUri) + "&context=" + QUrl::toPercentEncoding(sr.albumUri))); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);

    // Then, send a request to Spotify Web API and
    // retrieve track info and album art.
    request = QNetworkRequest();
    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://api.spotify.com/v1/tracks/" + QUrl::toPercentEncoding(sr.trackUri.replace("spotify:track:", ""))));

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(trackInfo_result()));
}

void SpotifyOverlay::pause()
{
    setIsPlaying(false);

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/pause.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&pause=true")); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);
}

void SpotifyOverlay::unpause()
{
    setIsPlaying(true);

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/pause.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&pause=false")); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);
}

void SpotifyOverlay::setIsPlaying(bool playing)
{
    isPlaying = playing;

    if (isPlaying)
    {
        ui->playButton->setIcon(pauseIcon);
    }
    else
    {
        ui->playButton->setIcon(playIcon);
    }
}

void SpotifyOverlay::playToggle()
{
    if (!isConnectedToSpotify())
        return;

    if (isPlaying)
    {
        pause();
    }
    else
    {
        unpause();
    }
}

void SpotifyOverlay::skip_prev()
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press
    ip.ki.wVk = VK_MEDIA_PREV_TRACK;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));

    // Release
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    QTimer::singleShot(250, this, SLOT(getSpotifyStatus()));
}

void SpotifyOverlay::skip_next()
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press
    ip.ki.wVk = VK_MEDIA_NEXT_TRACK;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));

    // Release
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    QTimer::singleShot(250, this, SLOT(getSpotifyStatus()));
}



/*
 * =============================================
 * SEARCH FUNCTIONS
 * =============================================
 */

void SpotifyOverlay::search()
{
    QString query = ui->searchField->text();
    QNetworkRequest request;

    scrollOffset = 0;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://api.spotify.com/v1/search?q=" + query + "&type=track&limit=" + QString::number(maxResults)));

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(search_result()));
}

void SpotifyOverlay::search_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();
        QJsonArray tracks = obj.value("tracks").toObject().value("items").toArray();

        resultCount = tracks.size();

        // Iterate through search results and populate result list.
        for (int i=0; i < resultCount; i++)
        {
            QJsonObject track = tracks[i].toObject();

            SearchResult sr;
            sr.song = track.value("name").toString();
            sr.artist = track.value("artists").toArray()[0].toObject().value("name").toString();
            sr.album = track.value("album").toObject().value("name").toString();
            sr.trackUri = track.value("uri").toString();
            sr.albumUri = track.value("album").toObject().value("uri").toString();
            sr.trackNum = track.value("track_number").toInt();

            searchResults[i] = sr;

            if (i < maxResultsDisplayed)
            {
                QPushButton* btn = this->findChild<QPushButton*>("searchResultBtn" + QString::number(i + 1));

                if (btn)
                {
                    QLabel* trackLabel = btn->findChild<QLabel*>("trackLabel");
                    QLabel* artistLabel = btn->findChild<QLabel*>("artistLabel");
                    QLabel* albumLabel = btn->findChild<QLabel*>("albumLabel");

                    QFontMetrics fm_track(trackLabel->font());
                    QString trackText = fm_track.elidedText(sr.song, Qt::ElideRight, trackLabel->width());

                    QFontMetrics fm_artist(artistLabel->font());
                    QString artistText = fm_artist.elidedText(sr.artist, Qt::ElideRight, artistLabel->width());

                    QFontMetrics fm_album(albumLabel->font());
                    QString albumText = fm_album.elidedText(sr.album, Qt::ElideRight, albumLabel->width());

                    trackLabel->setText(trackText);
                    artistLabel->setText(artistText);
                    albumLabel->setText(albumText);
                    btn->setProperty("resultId", i);
                }
            }
        }
    }
}

void SpotifyOverlay::search_selected()
{
    int resultId = sender()->property("resultId").toInt();

    play(resultId);
}

void SpotifyOverlay::search_scrollUp()
{
    if (scrollOffset > 0)
        scrollOffset -= 1;

    updateSearchResults();
}

void SpotifyOverlay::search_scrollDown()
{
    if (scrollOffset < resultCount - maxResultsDisplayed)
        scrollOffset += 1;

    updateSearchResults();
}

/*
 * =============================================
 * END SEARCH FUNCTIONS
 * =============================================
 */



void SpotifyOverlay::openKeyboard()
{
    if (!isConnectedToSpotify())
        return;

    vr::VROverlay()->ShowKeyboardForOverlay(COpenVROverlayController::SharedInstance()->m_ulOverlayHandle, vr::k_EGamepadTextInputModeNormal, vr::k_EGamepadTextInputLineModeSingleLine, "Search", 256, "", false, 0);

    vr::VREvent_t pEvent;
    bool polling = true;

    while (polling)
    {
        vr::VROverlay()->PollNextOverlayEvent(COpenVROverlayController::SharedInstance()->m_ulOverlayHandle, &pEvent, sizeof(pEvent));

        if (pEvent.eventType == vr::VREvent_KeyboardDone)
            polling = false;
    }

    char pchText[256];
    uint32_t cchText = 256;

    vr::VROverlay()->GetKeyboardText(pchText, cchText);
    ui->searchField->setText(QString(pchText));

    search();
}



/*
 * =============================================
 * PRIVATE SLOTS
 * =============================================
 */

void SpotifyOverlay::getSpotifyStatus()
{
    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/status.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF))); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(spotifyStatus_result()));
}

// TODO: Periodic status updates in case users are messing around
// via mobile, other PC's, the desktop, etc..
void SpotifyOverlay::spotifyStatus_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        setIsPlaying(obj.value("playing").toBool());

        QString uri = obj.value("track").toObject().value("track_resource").toObject().value("uri").toString();

        QNetworkRequest request = QNetworkRequest();
        request.setSslConfiguration(sslConfig);
        request.setUrl(QUrl("https://api.spotify.com/v1/tracks/" + QUrl::toPercentEncoding(uri.replace("spotify:track:", ""))));

        QNetworkReply* reply = netMgr->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(trackInfo_result()));
    }
}

void SpotifyOverlay::trackInfo_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {   
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        QFontMetrics fm_track(ui->trackTitle->font());
        QString trackText = fm_track.elidedText(obj.value("name").toString(), Qt::ElideRight, ui->trackTitle->width());

        QFontMetrics fm_artist(ui->trackAlbum->font());
        QString artistText = fm_artist.elidedText(obj.value("artists").toArray()[0].toObject().value("name").toString(), Qt::ElideRight, ui->trackAlbum->width());

        QFontMetrics fm_album(ui->trackArtist->font());
        QString albumText = fm_album.elidedText(obj.value("album").toObject().value("name").toString(), Qt::ElideRight, ui->trackArtist->width());

        ui->trackTitle->setText(trackText);
        ui->trackAlbum->setText(albumText);
        ui->trackArtist->setText(artistText);

        QString artUrl = obj.value("album").toObject().value("images").toArray()[0].toObject().value("url").toString();

        QNetworkRequest request;
        request.setUrl(QUrl(artUrl));

        QNetworkReply* reply = netMgr->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(albumArt_result()));
    }
}

void SpotifyOverlay::albumArt_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    QImage img;
    img.loadFromData(result->readAll());

    ui->albumArt->setPixmap(QPixmap::fromImage(img));
}
