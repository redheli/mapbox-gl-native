#ifndef MBGL_MAP_SOURCE_INFO
#define MBGL_MAP_SOURCE_INFO

#include <mbgl/style/types.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/rapidjson.hpp>

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace mapbox {
namespace geojsonvt {
class GeoJSONVT;
} // namespace geojsonvt
} // namespace mapbox

namespace mbgl {

class TileID;

class SourceInfo {
public:
    ~SourceInfo();

    std::string tileURL(const TileID&, float pixelRatio) const;

    void parseTileJSON(const JSValue&);
    void parseGeoJSON(const JSValue&);

public:
    std::vector<std::string> tiles;
    uint16_t tile_size = util::tileSize;
    uint16_t min_zoom = 0;
    uint16_t max_zoom = 22;
    std::string attribution;
    std::array<float, 3> center = { { 0, 0, 0 } };
    std::array<float, 4> bounds = { { -180, -90, 180, 90 } };

    std::shared_ptr<mapbox::geojsonvt::GeoJSONVT> geojsonvt;
};

} // namespace mbgl

#endif // MBGL_MAP_SOURCE_INFO
