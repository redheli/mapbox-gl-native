#include "storage.hpp"

#include <mbgl/platform/default/headless_display.hpp>
#include <mbgl/platform/default/headless_view.hpp>
#include <mbgl/storage/file_cache.hpp>
#include <mbgl/storage/online_file_source.hpp>

#include <mbgl/platform/log.hpp>
#include <mbgl/util/work_request.hpp>
#include <mbgl/util/io.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/algorithm/string.hpp>
#pragma GCC diagnostic pop


using namespace mbgl;
using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;
namespace algo = boost::algorithm;

namespace {

void checkRendering(Map& map, const char *name, std::chrono::milliseconds timeout) {
    test::checkImage("test/fixtures/stale/"s + name, test::render(map, timeout), 0.001, 0.1);
}

constexpr size_t len(const char* str) {
    return (*str == '\0') ? 0 : len(str + 1) + 1;
}

}

constexpr auto prefix = "http://127.0.0.1:3000";

// This mock cache object loads everything from the file system and marks it as stale, regardless
// of its age.
class StaleCache : public FileCache {
public:
    ~StaleCache() override = default;

    std::unique_ptr<WorkRequest> get(const Resource &resource, Callback callback) override {
        std::unique_ptr<Response> response;
        if (algo::starts_with(resource.url, prefix)) {
            const auto filename = "test/fixtures/"s + resource.url.substr(len(prefix) + 1);
            try {
                auto data = std::make_shared<std::string>(util::read_file(filename));
                response = std::make_unique<Response>();
                response->data = data;
                response->expires = Seconds::zero();
            } catch (const std::exception& e) {
                Log::Error(Event::Database, "%s", e.what());
            }
        } else {
            Log::Error(Event::Database, "Unhandled URL: %s", resource.url.c_str());
        }

        callback(std::move(response));
        return nullptr;
    }

    void put(const Resource &, std::shared_ptr<const Response>, Hint) override {}
};

const static auto display = std::make_shared<mbgl::HeadlessDisplay>();

TEST_F(Storage, StaleStyle) {
    HeadlessView view(display, 1);
    StaleCache cache;
    OnlineFileSource fileSource(&cache, test::getFileSourceRoot());

    Map map(view, fileSource, MapMode::Still);
    map.setStyleURL(std::string(prefix) + "/stale/style.json");

    checkRendering(map, "stale_style", 1000ms);
}

 TEST_F(Storage, StaleStyleAndTileJSON) {
     HeadlessView view(display, 1);
     StaleCache cache;
     OnlineFileSource fileSource(&cache, test::getFileSourceRoot());

     Map map(view, fileSource, MapMode::Still);
     map.setStyleURL(std::string(prefix) + "/stale/style_and_tilejson.json");

     checkRendering(map, "stale_style_and_tilejson", 2000ms);
 }

 TEST_F(Storage, StaleStyleAndSprite) {
     HeadlessView view(display, 1);
     StaleCache cache;
     OnlineFileSource fileSource(&cache, test::getFileSourceRoot());

     Map map(view, fileSource, MapMode::Still);
     map.setStyleURL(std::string(prefix) + "/stale/style_and_sprite.json");

     checkRendering(map, "stale_style_and_sprite", 2000ms);
 }
