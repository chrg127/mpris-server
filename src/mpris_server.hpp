#ifndef MPRISSERVER_HPP_INCLUDED
#define MPRISSERVER_HPP_INCLUDED

#include <sdbus-c++/sdbus-c++.h>
#include <functional>
#include <iostream>

namespace mpris {

static const auto *prefix        = "org.mpris.MediaPlayer2.";
static const auto *object_path   = "/org/mpris/MediaPlayer2";
static const auto *mp2_interface = "org.mpris.MediaPlayer2";
// static const QString dBusPropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
// static const QString dBusPropertiesChangedSignal = QStringLiteral("PropertiesChanged");

struct Server {
    std::string service_name;
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IObject> object;

#define CALLBACK(name, ...)                         \
private: std::function<void(__VA_ARGS__)> name;     \
public:  void on_##name(auto &&fn) { name = fn; }   \

    CALLBACK(quit, void)
#undef CALLBACK

    void try_call(auto &&fn, auto&&... args)
    {
        if (fn)
            fn(args...);
        else
            std::cout << "not implemented";
    }

public:
    explicit Server(const std::string &name)
        : service_name(std::string(prefix) + name)
    {
        connection = sdbus::createSessionBusConnection(service_name.c_str());
        object     = sdbus::createObject(*connection, object_path);
        object->registerMethod("Quit").onInterface(mp2_interface).implementedAs([&] () { try_call(quit); });
        object->finishRegistration();
    }

    void start_loop()       { connection->enterEventLoop(); }
    void start_loop_async() { connection->enterEventLoopAsync(); }

};

} // namespace mpris

#endif
