#ifndef MPRISSERVER_HPP_INCLUDED
#define MPRISSERVER_HPP_INCLUDED

#include <cstdio>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <sdbus-c++/sdbus-c++.h>

namespace mpris {

using StringList = std::vector<std::string>;
using Metadata   = std::map<std::string, sdbus::Variant>;

static const auto *PREFIX       = "org.mpris.MediaPlayer2.";
static const auto *OBJECT_PATH  = "/org/mpris/MediaPlayer2";
static const auto *MP2          = "org.mpris.MediaPlayer2";
static const auto *MP2PLAYER    = "org.mpris.MediaPlayer2.Player";

enum class PlaybackStatus { Playing, Paused, Stopped };
enum class LoopStatus     { None, Track, Playlist };

enum class MetadataEntry {
    TrackId     , Length     , ArtUrl      , Album          ,
    AlbumArtist , Artist     , AsText      , AudioBPM       ,
    AutoRating  , Comment    , Composer    , ContentCreated ,
    DiscNumber  , FirstUsed  , Genre       , LastUsed       ,
    Lyricist    , Title      , TrackNumber , Url            ,
    UseCount    , UserRating
};

static const char *playback_status_strings[] = { "Playing"           , "Paused"              , "Stopped" };
static const char *loop_status_strings[]     = { "None"              , "Track"               , "Playlist" };
static const char *metadata_strings[]        = { "mpris:trackid"     , "mpris:length"        , "mpris:artUrl"      , "xesam:album"          ,
                                                 "xesam:albumArtist" , "xesam:artist"        , "xesam:asText"      , "xesam:audioBPM"       ,
                                                 "xesam:autoRating"  , "xesam:comment"       , "xesam:composer"    , "xesam:contentCreated" ,
                                                 "xesam:discNumber"  , "xesam:firstUsed"     , "xesam:genre"       , "xesam:lastUsed"       ,
                                                 "xesam:lyricist"    , "xesam:title"         , "xesam:trackNumber" , "xesam:url"            ,
                                                 "xesam:useCount"    , "xesam:userRating" };

namespace detail {

template <typename T, typename R, typename... Args>
std::function<R(Args...)> member_fn(T *obj, R (T::*fn)(Args...))
{
    return [=](Args&&... args) -> R { return (obj->*fn)(args...); };
}

std::string metadata_entry_to_string(   MetadataEntry entry) { return        metadata_strings[static_cast<int>( entry)]; }
std::string playback_status_to_string(PlaybackStatus status) { return playback_status_strings[static_cast<int>(status)]; }
std::string loop_status_to_string(        LoopStatus status) { return     loop_status_strings[static_cast<int>(status)]; }

} // namespace detail

struct Server {
    std::string service_name;
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IObject> object;

#define PROPERTY(type, name, defval)                        \
private:                                                    \
    type name = defval;                                     \
    type get_##name() { return name; }                      \
                                                            \
public:                                                     \
    void set_##name(const type &value) { name = value; }    \

    PROPERTY(StringList, supported_mime_types, {})
    PROPERTY(StringList, supported_uri_schemes, {})
    PROPERTY(bool, can_quit, false)
    PROPERTY(bool, can_raise, false)
    PROPERTY(bool, can_set_fullscreen, false)
    PROPERTY(bool, fullscreen, false)
    PROPERTY(bool, has_tracklist, false)
    PROPERTY(std::string, desktop_entry, "")
    PROPERTY(std::string, identity, "")

    PROPERTY(bool, can_control, false)
    PROPERTY(bool, can_go_next, false)
    PROPERTY(bool, can_go_previous, false)
    PROPERTY(bool, can_pause, false)
    PROPERTY(bool, can_play, false)
    PROPERTY(bool, can_seek, false)
    PROPERTY(bool, shuffle, false)
    PROPERTY(Metadata, metadata, {})
    PROPERTY(double, volume, 0.0)
    PROPERTY(double, maximum_rate, 0.0)
    PROPERTY(double, minimum_rate, 0.0)
    PROPERTY(double, rate, 0.0)
    PROPERTY(int64_t, position, 0)

#undef PROPERTY

private:
    PlaybackStatus playback_status = PlaybackStatus::Invalid;
    LoopStatus loop_status = LoopStatus::Invalid;

    std::string get_playback_status() { return detail::playback_status_to_string(playback_status); }
    std::string get_loop_status()     { return detail::loop_status_to_string(loop_status); }

public:
    void set_playback_status(PlaybackStatus value) { playback_status = value; }
    void set_playback_status_from_string(std::string value)
    {
        for (auto i = 0u; i < std::size(playback_status_strings); i++)
            if (value == playback_status_strings[i])
                set_playback_status(static_cast<PlaybackStatus>(i));
    }

    void set_loop_status(LoopStatus value)
    {
        if (!can_control)
            throw sdbus::Error(service_name, "Cannot set loop status (CanControl is false)");
        loop_status = value;
    }
    void set_loop_status_from_string(std::string value)
    {
        for (auto i = 0u; i < std::size(loop_status_strings); i++)
            if (value == loop_status_strings[i])
                set_loop_status(static_cast<LoopStatus>(i));
    }

#define CALLBACK(name, ...)                         \
private:                                            \
    std::function<void(__VA_ARGS__)> name##_fn;     \
                                                    \
public:                                             \
    void on_##name(auto &&fn) { name##_fn = fn; }   \

    CALLBACK(quit,          void)
    CALLBACK(raise,         void)
    CALLBACK(next,          void)
    CALLBACK(previous,      void)
    CALLBACK(pause,         void)
    CALLBACK(play_pause,    void)
    CALLBACK(stop,          void)
    CALLBACK(play,          void)
    CALLBACK(seek,          int64_t)
    CALLBACK(set_position,  sdbus::ObjectPath, int64_t)
    CALLBACK(open_uri,      std::string)

#undef CALLBACK

    void try_call(auto &&fn, auto&&... args)
    {
        if (fn)
            fn(args...);
        else
            std::fprintf(stderr, "error: not implemented\n");
    }

public:
    explicit Server(const std::string &name)
        : service_name(std::string(PREFIX) + name)
    {
        connection = sdbus::createSessionBusConnection(service_name.c_str());
        object     = sdbus::createObject(*connection, OBJECT_PATH);
        object->registerMethod("Quit")       .onInterface(MP2)      .implementedAs([&]                                     { try_call(quit_fn);                  });
        object->registerMethod("Raise")      .onInterface(MP2)      .implementedAs([&]                                     { try_call(raise_fn);                 });
        object->registerMethod("Next")       .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(next_fn);                  });
        object->registerMethod("Previous")   .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(previous_fn);              });
        object->registerMethod("Pause")      .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(pause_fn);                 });
        object->registerMethod("PlayPause")  .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(play_pause_fn);            });
        object->registerMethod("Stop")       .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(stop_fn);                  });
        object->registerMethod("Play")       .onInterface(MP2PLAYER).implementedAs([&]                                     { try_call(play_fn);                  });
        object->registerMethod("Seek")       .onInterface(MP2PLAYER).implementedAs([&] (int64_t n)                         { try_call(seek_fn, n);               }).withInputParamNames("Offset");
        object->registerMethod("SetPosition").onInterface(MP2PLAYER).implementedAs([&] (sdbus::ObjectPath id, int64_t pos) { try_call(set_position_fn, id, pos); }).withInputParamNames("TrackId", "Position");
        object->registerMethod("OpenUri")    .onInterface(MP2PLAYER).implementedAs([&] (std::string uri)                   { try_call(open_uri_fn, uri);         }).withInputParamNames("Uri");

#define M(f) detail::member_fn(this, &Server::f)
        object->registerProperty("CanQuit")             .onInterface(MP2).withGetter(M(get_can_quit))             .withSetter(M(set_can_quit));
        object->registerProperty("Fullscreen")          .onInterface(MP2).withGetter(M(get_fullscreen))           .withSetter(M(set_fullscreen));
        object->registerProperty("CanSetFullscreen")    .onInterface(MP2).withGetter(M(get_can_set_fullscreen))   .withSetter(M(set_can_set_fullscreen));
        object->registerProperty("CanRaise")            .onInterface(MP2).withGetter(M(get_can_raise))            .withSetter(M(set_can_raise));
        object->registerProperty("HasTrackList")        .onInterface(MP2).withGetter(M(get_has_tracklist))        .withSetter(M(set_has_tracklist));
        object->registerProperty("Identity")            .onInterface(MP2).withGetter(M(get_identity))             .withSetter(M(set_identity));
        object->registerProperty("DesktopEntry")        .onInterface(MP2).withGetter(M(get_desktop_entry))        .withSetter(M(set_desktop_entry));
        object->registerProperty("SupportedUriSchemes") .onInterface(MP2).withGetter(M(get_supported_uri_schemes)).withSetter(M(set_supported_uri_schemes));
        object->registerProperty("SupportedMimeTypes")  .onInterface(MP2).withGetter(M(get_supported_mime_types)) .withSetter(M(set_supported_mime_types));

        object->registerProperty("CanGoPrevious") .onInterface(MP2PLAYER).withGetter(M(get_can_go_previous)).withSetter(M(set_can_go_previous));
        object->registerProperty("PlaybackStatus").onInterface(MP2PLAYER).withGetter(M(get_playback_status)).withSetter(M(set_playback_status_from_string));
        object->registerProperty("LoopStatus")    .onInterface(MP2PLAYER).withGetter(M(get_loop_status))    .withSetter(M(set_loop_status_from_string));
        object->registerProperty("Rate")          .onInterface(MP2PLAYER).withGetter(M(get_rate))           .withSetter(M(set_rate));
        object->registerProperty("Shuffle")       .onInterface(MP2PLAYER).withGetter(M(get_shuffle))        .withSetter(M(set_shuffle));
        object->registerProperty("Metadata")      .onInterface(MP2PLAYER).withGetter(M(get_metadata))       .withSetter(M(set_metadata));
        object->registerProperty("Volume")        .onInterface(MP2PLAYER).withGetter(M(get_volume))         .withSetter(M(set_volume));
        object->registerProperty("Position")      .onInterface(MP2PLAYER).withGetter(M(get_position))       .withSetter(M(set_position));
        object->registerProperty("MinimumRate")   .onInterface(MP2PLAYER).withGetter(M(get_minimum_rate))   .withSetter(M(set_minimum_rate));
        object->registerProperty("MaximumRate")   .onInterface(MP2PLAYER).withGetter(M(get_maximum_rate))   .withSetter(M(set_maximum_rate));
        object->registerProperty("CanGoNext")     .onInterface(MP2PLAYER).withGetter(M(get_can_go_next))    .withSetter(M(set_can_go_next));
        object->registerProperty("CanPlay")       .onInterface(MP2PLAYER).withGetter(M(get_can_play))       .withSetter(M(set_can_play));
        object->registerProperty("CanPause")      .onInterface(MP2PLAYER).withGetter(M(get_can_pause))      .withSetter(M(set_can_pause));
        object->registerProperty("CanSeek")       .onInterface(MP2PLAYER).withGetter(M(get_can_seek))       .withSetter(M(set_can_seek));
        object->registerProperty("CanControl")    .onInterface(MP2PLAYER).withGetter(M(get_can_control))    .withSetter(M(set_can_control));
#undef M

        object->registerSignal("Seeked").onInterface(MP2PLAYER).withParameters<int64_t>("Position");

        object->finishRegistration();
    }

    void start_loop()       { connection->enterEventLoop(); }
    void start_loop_async() { connection->enterEventLoopAsync(); }

};

} // namespace mpris

#endif
