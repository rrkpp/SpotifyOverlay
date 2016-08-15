# SpotifyOverlay
A third-party Spotify client built specifically for OpenVR.

![alt text](https://i.imgur.com/xxSJjpn.png "Spotify Overlay")

## Installation
Download this repo and put the files in /bin somewhere, run SpotifyOverlay.exe. Alternatively, just download the files from /bin individually and put them somewhere. Installer will be provided once testing is complete and there is an "official" release. The application will automatically generate a .vrmanifest file and set the overlay to auto-start whenever you boot SteamVR (Thanks to code taken from LibreVR's Revive project). If you don't want the overlay to run every time SteamVR runs, you can disable it in SteamVR > Settings > Applications.

## How to Use
The app should auto-start whenever you load SteamVR. To access the overlay, just open up the SteamVR dashboard by pressing the Steam button on your controller/headset, then look near the bottom for the Spotify overlay button alongside all your other dashboard overlays. Click the 'Search' button to open the SteamVR keyboard, then type in your search terms and hit 'Done'. It'll search for tracks/albums/artists and list them in order of relevancy/popularity according to Spotify. Aim at a search result and pull the trigger to play. When you play a track, it'll queue up within the album the track is from, meaning if you play a track and hit the skip-forward button, it'll play the next track on the album.

## Planned Features
* Clicking on album/artist name to automatically search album/artist name
* Periodically checking Spotify status just in case users modify playback from outside the overlay
* Track progress indicator
* Clicking on search bar in order to open keyboard
* Scrolling with Vive touchpad
* Playlist support
* Cosmetic updates?

## Impossible Features
There are a variety of features that are either not possible or that I simply do not know how to accomplish as a result of how hacky this app's communication with Spotify is. There is no official documentation for the method I'm using to communicate to Spotify with. As a result, the only functions available to me are those that have been documented by people much smarter than me, like Carl Bystrom, who wrote a blog post that made this overlay possible: http://cgbystrom.com/articles/deconstructing-spotifys-builtin-http-server/

Long story short, here's some stuff that probably isn't ever going to happen:
* Queing tracks
* Changing Spotify volume
* Modifying playlists
* Seeking through tracks

## Credits / Thanks
* Carl Bystrom for documenting the local Spotify HTTP server, which made this overlay possible
* LibreVR for developing the open-source, MIT-licensed Revive project, which helped demonstrate how create a usable dashboard overlay, and from which the code for manifest generation was copied. Give him some support: https://github.com/LibreVR/Revive
