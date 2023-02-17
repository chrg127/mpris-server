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

using namespace std::literals::string_literals;

using StringList = std::vector<std::string>;

static const auto PREFIX       = "org.mpris.MediaPlayer2."s;
static const auto OBJECT_PATH  = "/org/mpris/MediaPlayer2"s;
static const auto MP2          = "org.mpris.MediaPlayer2"s;
static const auto MP2PLAYER    = "org.mpris.MediaPlayer2.Player"s;

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

template <typename T, typename R, typename... Args>
std::function<R(Args...)> member_fn(T *obj, R (T::*fn)(Args...) const)
{
    return [=](Args&&... args) -> R { return (obj->*fn)(args...); };
}

std::string playback_status_to_string(PlaybackStatus status) { return playback_status_strings[static_cast<int>(status)]; }
std::string loop_status_to_string(        LoopStatus status) { return     loop_status_strings[static_cast<int>(status)]; }

} // namespace detail

std::string metadata_entry_to_string(MetadataEntry entry) { return metadata_strings[static_cast<int>( entry)]; }

class Server {
    std::string name;
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IObject> object;

    void prop_changed(const std::string &prop);

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

    CALLBACK(fullscreen_changed,    bool)
    CALLBACK(loop_status_changed,   LoopStatus)
    CALLBACK(rate_changed,          double)
    CALLBACK(shuffle_changed,       bool)
    CALLBACK(volume_changed,        double)

#undef CALLBACK

private:
    bool fullscreen                  = false;
    std::string identity             = "";
    std::string desktop_entry        = "";
    StringList supported_uri_schemes = {};
    StringList supported_mime_types  = {};

    PlaybackStatus playback_status                 = PlaybackStatus::Stopped;
    LoopStatus loop_status                         = LoopStatus::None;
    double rate                                    = 0.0;
    bool shuffle                                   = false;
    std::map<std::string, sdbus::Variant> metadata = {};
    double volume                                  = 0.0;
    int64_t position                               = 0;
    double maximum_rate                            = 0.0;
    double minimum_rate                            = 0.0;

    bool can_control()     const { return bool(loop_status_changed_fn) && bool(shuffle_changed_fn)
                                       && bool(volume_changed_fn)      && bool(stop_fn);                }
    bool can_go_next()     const { return can_control() && bool(next_fn);                               }
    bool can_go_previous() const { return can_control() && bool(previous_fn);                           }
    bool can_play()        const { return can_control() && bool(play_fn)      && bool(play_pause_fn);   }
    bool can_pause()       const { return can_control() && bool(pause_fn)     && bool(play_pause_fn);   }
    bool can_seek()        const { return can_control() && bool(seek_fn);                               }

    void set_fullscreen_external(bool value);
    void set_loop_status_external(std::string value);
    void set_rate_external(double value);
    void set_shuffle_external(bool value);
    void set_volume_external(double value);

public:
    static Server make(const std::string &name);

    Server(const std::string &player_name);
    void start_loop();
    void start_loop_async();

    void set_fullscreen(bool value)                         { fullscreen            = value; prop_changed("Fullscreen"); }
    void set_identity(const std::string &value)             { identity              = value; prop_changed("Identity"); }
    void set_desktop_entry(const std::string &value)        { desktop_entry         = value; prop_changed("DesktopEntry"); }
    void set_supported_uri_schemes(const StringList &value) { supported_uri_schemes = value; prop_changed("Fullscreen"); }
    void set_supported_mime_types(const StringList &value)  { supported_mime_types  = value; prop_changed("Fullscreen"); }

    void set_playback_status(PlaybackStatus value)          { playback_status       = value; prop_changed("Fullscreen"); }
    void set_loop_status(LoopStatus value)                  { loop_status           = value; prop_changed("Fullscreen"); }
    void set_rate(double value)                             { rate                  = value; prop_changed("Fullscreen"); }
    void set_shuffle(bool value)                            { shuffle               = value; prop_changed("Fullscreen"); }
    void set_metadata(const std::map<MetadataEntry, sdbus::Variant> &value)
    {
        metadata.clear();
        for (auto [k, v] : value)
            metadata[metadata_entry_to_string(k)] = v;
        prop_changed("Fullscreen");
    }
    void set_volume(double value)                           { volume                = value; prop_changed("Fullscreen"); }
    void set_position(int64_t value)                        { position              = value; prop_changed("Fullscreen"); }
    void set_maximum_rate(double value)                     { maximum_rate          = value; prop_changed("Fullscreen"); }
    void set_minimum_rate(double value)                     { minimum_rate          = value; prop_changed("Fullscreen"); }

    void send_seeked_signal(int64_t position);
};

Server Server::make(const std::string &name)
{
    auto server = Server(name);
    return server;
}

Server::Server(const std::string &player_name)
    : name(player_name)
{
    connection = sdbus::createSessionBusConnection();
    connection->requestName(PREFIX + player_name);
    object = sdbus::createObject(*connection, OBJECT_PATH);

    object->registerMethod("Raise")      .onInterface(MP2)      .implementedAs([&] { if (raise_fn)                  raise_fn();      });
    object->registerMethod("Quit")       .onInterface(MP2)      .implementedAs([&] { if (quit_fn)                   quit_fn();       });
    object->registerMethod("Next")       .onInterface(MP2PLAYER).implementedAs([&] { if (can_go_next())             next_fn();       });
    object->registerMethod("Previous")   .onInterface(MP2PLAYER).implementedAs([&] { if (can_go_previous())         previous_fn();   });
    object->registerMethod("Pause")      .onInterface(MP2PLAYER).implementedAs([&] { if (can_pause())               pause_fn();      });
    object->registerMethod("PlayPause")  .onInterface(MP2PLAYER).implementedAs([&] { if (can_play() || can_pause()) play_pause_fn(); });
    object->registerMethod("Stop")       .onInterface(MP2PLAYER).implementedAs([&] { if (can_control())             stop_fn();       });
    object->registerMethod("Play")       .onInterface(MP2PLAYER).implementedAs([&] { if (can_play())                play_fn();       });
    object->registerMethod("Seek")       .onInterface(MP2PLAYER).implementedAs([&] (int64_t n)                         { if (can_seek()) seek_fn(n); })              .withInputParamNames("Offset");
    object->registerMethod("SetPosition").onInterface(MP2PLAYER).implementedAs([&] (sdbus::ObjectPath id, int64_t pos) { if (can_seek()) set_position_fn(id, pos); }).withInputParamNames("TrackId", "Position");
    object->registerMethod("OpenUri")    .onInterface(MP2PLAYER).implementedAs([&] (std::string uri) { if (open_uri_fn) open_uri_fn(uri); }).withInputParamNames("Uri");

#define M(f) detail::member_fn(this, &Server::f)
    object->registerProperty("CanQuit")             .onInterface(MP2).withGetter([&] { return bool(quit_fn); });
    object->registerProperty("Fullscreen")          .onInterface(MP2).withGetter([&] { return fullscreen; }).withSetter(M(set_fullscreen_external));
    object->registerProperty("CanSetFullscreen")    .onInterface(MP2).withGetter([&] { return bool(fullscreen_changed_fn); });
    object->registerProperty("CanRaise")            .onInterface(MP2).withGetter([&] { return bool(raise_fn); });
    object->registerProperty("HasTrackList")        .onInterface(MP2).withGetter([&] { return false; });
    object->registerProperty("Identity")            .onInterface(MP2).withGetter([&] { return identity; });
    object->registerProperty("DesktopEntry")        .onInterface(MP2).withGetter([&] { return desktop_entry; });
    object->registerProperty("SupportedUriSchemes") .onInterface(MP2).withGetter([&] { return supported_uri_schemes; });
    object->registerProperty("SupportedMimeTypes")  .onInterface(MP2).withGetter([&] { return supported_mime_types; });

    object->registerProperty("PlaybackStatus").onInterface(MP2PLAYER).withGetter([&] { return detail::playback_status_to_string(playback_status); });
    object->registerProperty("LoopStatus")    .onInterface(MP2PLAYER).withGetter([&] { return detail::loop_status_to_string(loop_status); })
                                                                     .withSetter(M(set_loop_status_external));
    object->registerProperty("Rate")          .onInterface(MP2PLAYER).withGetter([&] { return rate; })
                                                                     .withSetter(M(set_rate_external));
    object->registerProperty("Shuffle")       .onInterface(MP2PLAYER).withGetter([&] { return shuffle; })
                                                                     .withSetter(M(set_shuffle_external));
    object->registerProperty("Metadata")      .onInterface(MP2PLAYER).withGetter([&] { return metadata; });
    object->registerProperty("Volume")        .onInterface(MP2PLAYER).withGetter([&] { return volume; })
                                                                     .withSetter(M(set_volume_external));
    object->registerProperty("Position")      .onInterface(MP2PLAYER).withGetter([&] { return position; });
    object->registerProperty("MinimumRate")   .onInterface(MP2PLAYER).withGetter([&] { return minimum_rate; });
    object->registerProperty("MaximumRate")   .onInterface(MP2PLAYER).withGetter([&] { return maximum_rate; });
    object->registerProperty("CanGoNext")     .onInterface(MP2PLAYER).withGetter(M(can_go_next));
    object->registerProperty("CanGoPrevious") .onInterface(MP2PLAYER).withGetter(M(can_go_previous));
    object->registerProperty("CanPlay")       .onInterface(MP2PLAYER).withGetter(M(can_play));
    object->registerProperty("CanPause")      .onInterface(MP2PLAYER).withGetter(M(can_pause));
    object->registerProperty("CanSeek")       .onInterface(MP2PLAYER).withGetter(M(can_seek));
    object->registerProperty("CanControl")    .onInterface(MP2PLAYER).withGetter(M(can_control));
#undef M

    object->registerSignal("Seeked").onInterface(MP2PLAYER).withParameters<int64_t>("Position");

    object->finishRegistration();
}

void Server::start_loop()       { connection->enterEventLoop(); }
void Server::start_loop_async() { connection->enterEventLoopAsync(); }

void Server::set_fullscreen_external(bool value)
{
    if (fullscreen_changed_fn)
        throw sdbus::Error(PREFIX + name + ".Error", "Cannot set Fullscreen (CanSetFullscreen is false).");
    set_fullscreen(value);
    fullscreen_changed_fn(fullscreen);
}

void Server::set_loop_status_external(std::string value)
{
    for (auto i = 0u; i < std::size(loop_status_strings); i++) {
        if (value == loop_status_strings[i]) {
            if (!can_control())
                throw sdbus::Error(PREFIX + name + ".Error", "Cannot set loop status (CanControl is false).");
            set_loop_status(static_cast<LoopStatus>(i));
            loop_status_changed_fn(loop_status);
        }
    }
}

void Server::set_rate_external(double value)
{
    set_rate(value);
    if (rate_changed_fn)
        rate_changed_fn(rate);
}

void Server::set_shuffle_external(bool value)
{
    if (!can_control())
        throw sdbus::Error(PREFIX + name + ".Error", "Cannot set shuffle (CanControl is false).");
    set_shuffle(value);
    shuffle_changed_fn(shuffle);
}

void Server::set_volume_external(double value)
{
    if (!can_control())
        throw sdbus::Error(PREFIX + name + ".Error", "Cannot set volume (CanControl is false).");
    set_volume(value);
    volume_changed_fn(volume);
}

void Server::send_seeked_signal(int64_t position)
{
    object->emitSignal("Seeked").onInterface(MP2PLAYER).withArguments(position);
}

void Server::prop_changed(const std::string &prop)
{
    object->emitPropertiesChangedSignal(MP2, std::vector<std::string>{ prop });
}

} // namespace mpris

#endif
