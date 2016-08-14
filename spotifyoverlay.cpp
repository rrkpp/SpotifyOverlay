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

    // Configure search results' vertical layout.
    searchLayout.setAlignment(Qt::AlignTop);
    searchLayout.setSpacing(0);
    searchLayout.setMargin(0);

    // Initialize Play/Pause icons.
    playIcon = QIcon(":/img/play.png");
    pauseIcon = QIcon(":/img/pause.png");

    // TODO: Find a way to get past the fact that scrollbars are completely
    // performance inept when running in OpenVR. For now, only 5 results.
    ui->searchResults->widget()->setLayout(&searchLayout);
    ui->searchResults->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Connect SIGNALs/SLOTs.
    connect(ui->playButton, SIGNAL(released()), this, SLOT(playToggle()));
    connect(ui->prevButton, SIGNAL(released()), this, SLOT(skip_prev()));
    connect(ui->nextButton, SIGNAL(released()), this, SLOT(skip_next()));
    connect(ui->searchButton, SIGNAL(released()), this, SLOT(openKeyboard()));
}

SpotifyOverlay::~SpotifyOverlay()
{
    delete ui;
    delete netMgr;
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

void SpotifyOverlay::getSpotifyStatus()
{
    qDebug() << "Checking status..";

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/status.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF))); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(updateSpotifyStatus()));
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

void SpotifyOverlay::oath_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        key_OAuth = obj.value("t").toString();

        qDebug().noquote() << "Retrieved OAuth key: " + key_OAuth;

        getTokenCSRF();
    }
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

        qDebug().noquote() << "Retrieved CSRF key: " + key_CSRF;

        getSpotifyStatus();
    }
}

void SpotifyOverlay::play(QString uri)
{
    qDebug().noquote() << "Playing track: " + uri;

    setIsPlaying(true);

    // First, send a request to the local Spotify server
    // for Spotify to play the selected track.
    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/play.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&uri=" + QUrl::toPercentEncoding(uri) + "&context=" + QUrl::toPercentEncoding(uri))); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);

    // Then, send a request to Spotify Web API and
    // retrieve track info and album art.
    request = QNetworkRequest();
    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://api.spotify.com/v1/tracks/" + QUrl::toPercentEncoding(uri.replace("spotify:track:", ""))));

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(updateTrackInfo()));
}

void SpotifyOverlay::playToggle()
{
    if (isPlaying)
    {
        pause();
    }
    else
    {
        unpause();
    }
}

void SpotifyOverlay::pause()
{
    qDebug() << "Pausing track..";

    setIsPlaying(false);

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/pause.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&pause=true")); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);
}

void SpotifyOverlay::unpause()
{
    qDebug() << "Unpausing track..";

    setIsPlaying(true);

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://aaaaaaaaaa.spotilocal.com:4370/remote/pause.json?oauth=" + QUrl::toPercentEncoding(key_OAuth) + "&csrf=" + QUrl::toPercentEncoding(key_CSRF) + "&pause=false")); // TODO: Randomize subdomain and port number.
    request.setRawHeader("Origin", "https://open.spotify.com");

    netMgr->get(request);
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

    getSpotifyStatus();
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

    getSpotifyStatus();
}

void SpotifyOverlay::search()
{
    QString query = ui->searchField->text();

    qDebug().noquote() << "Searching: " + query;

    QNetworkRequest request;

    request.setSslConfiguration(sslConfig);
    request.setUrl(QUrl("https://api.spotify.com/v1/search?q=" + query + "&type=track&limit=" + QString::number(maxResults)));

    QNetworkReply* reply = netMgr->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(search_result()));
}

void SpotifyOverlay::search_result()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    // Clear out the old search results before attempting
    // to populate the list with new results.
    QLayout* resultsLayout = ui->searchResults->widget()->layout();
    QLayoutItem* item;

    while (resultsLayout->count() > 0)
    {
        item = resultsLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();
        QJsonArray tracks = obj.value("tracks").toObject().value("items").toArray();

        // Iterate through search results and populate result list.
        for (int i=0; i < tracks.size(); i++)
        {
            QJsonObject track = tracks[i].toObject();
            QString title = track.value("name").toString();
            QString artist = track.value("artists").toArray()[0].toObject().value("name").toString();
            QString album = track.value("album").toObject().value("name").toString();

            QPushButton* widget = new QPushButton();
            widget->setMinimumHeight(50);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            widget->setStyleSheet("QPushButton {background-color: #232323; border: none; border-bottom: 1px solid black;} QPushButton:hover {background-color: #393939} QPushButton * {border: none; background: none;}");

            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(0);
            layout->setMargin(0);

            QLabel* titleLabel = new QLabel(title);
            titleLabel->setFont(QFont("Open Sans", 12, 500));
            titleLabel->setMaximumWidth(250);
            titleLabel->setStyleSheet("color: white;");

            QLabel* artistLabel = new QLabel(artist);
            artistLabel->setFont(QFont("Open Sans", 12));
            artistLabel->setMaximumWidth(250);
            artistLabel->setStyleSheet("color: white;");

            QLabel* albumLabel = new QLabel(album);
            albumLabel->setFont(QFont("Open Sans", 12));
            albumLabel->setMaximumWidth(250);
            albumLabel->setStyleSheet("color: white;");

            layout->addWidget(titleLabel);
            layout->addWidget(artistLabel);
            layout->addWidget(albumLabel);

            widget->setLayout(layout);
            widget->setProperty("uri", track.value("uri").toString());

            connect(widget, SIGNAL(released()), this, SLOT(search_selected()));

            ui->searchResults->widget()->layout()->addWidget(widget);
            ui->searchResults->widget()->adjustSize();
        }
    }
}

void SpotifyOverlay::search_selected()
{
    QString uri = sender()->property("uri").toString();
    play(uri);
}

void SpotifyOverlay::openKeyboard()
{
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

// TODO: Periodic status updates in case users are messing around
// via mobile, other PC's, the desktop, etc..
void SpotifyOverlay::updateSpotifyStatus()
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
        connect(reply, SIGNAL(finished()), this, SLOT(updateTrackInfo()));
    }
}

void SpotifyOverlay::updateTrackInfo()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    if (result->size() > 0)
    {
        QString jsonString = QString(result->readAll());

        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject obj = doc.object();

        ui->trackTitle->setText(obj.value("name").toString());
        ui->trackAlbum->setText(obj.value("album").toObject().value("name").toString());
        ui->trackArtist->setText(obj.value("artists").toArray()[0].toObject().value("name").toString());

        QString artUrl = obj.value("album").toObject().value("images").toArray()[0].toObject().value("url").toString();

        QNetworkRequest request;
        request.setUrl(QUrl(artUrl));

        QNetworkReply* reply = netMgr->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(albumArtDownloaded()));
    }
}

void SpotifyOverlay::albumArtDownloaded()
{
    QNetworkReply* result = (QNetworkReply*) sender();

    QImage img;
    img.loadFromData(result->readAll());

    ui->albumArt->setPixmap(QPixmap::fromImage(img));
}
