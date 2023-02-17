#include "../src/mpris_server.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

/*
int main()
{
    auto connection = sdbus::createSessionBusConnection("org.mpris.MediaPlayer2.genericplayer");
    auto obj = sdbus::createObject(*connection, "/org/mpris/MediaPlayer2");
    obj->registerMethod("Raise").onInterface("org.mpris.MediaPlayer2").implementedAs([] { printf("hi\n"); });
    obj->finishRegistration();
    // auto obj2 = sdbus::createObject(*connection, "/org/mpris");
    // obj2->registerMethod("Get").onInterface("org.freedesktop.DBus.Properties").implementedAs([] (
    connection->enterEventLoop();
    return 0;
}
*/

int main()
{
    int i = 0;
    int64_t pos = 0;
    bool playing = false;

    auto server = mpris::Server::make("genericplayer");

    server.set_identity("A generic player");
    server.set_supported_uri_schemes({ "file" });
    server.set_supported_mime_types({ "application/octet-stream", "text/plain" });
    server.set_metadata({
        { mpris::Field::TrackId,    "/1"             },
        { mpris::Field::Album,      "an album"       },
        { mpris::Field::Title,      "best song ever" },
        { mpris::Field::Artist,     "idk"            },
        { mpris::Field::Length,     1000             }
    } );
    server.set_maximum_rate(2.0);
    server.set_minimum_rate(0.1);

    server.on_quit([&] { std::exit(0); });
    server.on_next([&] { i++; });
    server.on_previous([&] { i--; });
    server.on_pause([&] {
        playing = false;
        server.set_playback_status(mpris::PlaybackStatus::Paused);
    });
    server.on_play_pause([&] {
        playing = !playing;
        server.set_playback_status(playing ? mpris::PlaybackStatus::Playing
                                           : mpris::PlaybackStatus::Paused);
    });
    server.on_stop([&] {
        playing = false;
        server.set_playback_status(mpris::PlaybackStatus::Stopped);
    });
    server.on_play([&] {
        playing = true;
        server.set_playback_status(mpris::PlaybackStatus::Playing);
    });
    server.on_seek([&] (int64_t p) { printf("changing pos: %ld\n", pos); pos = p; server.set_position(pos); });
    server.on_set_position([&] (sdbus::ObjectPath path, int64_t pos) { });
    server.on_open_uri([&] (std::string uri) { std::cout << "not opening uri, sorry\n"; });

    server.on_loop_status_changed([&] (mpris::LoopStatus status) { });
    server.on_rate_changed([&] (double rate) { });
    server.on_shuffle_changed([&] (bool shuffle) { });
    server.on_volume_changed([&] (double vol) { });

    server.start_loop_async();
    for (;;) {
        if (playing) {
            printf("%ld\n", pos);
            pos++;
            server.set_position(pos);
        } else {
            printf("i'm paused (or stopped)\n");
        }
        sleep(1);
    }

    return 0;
}
