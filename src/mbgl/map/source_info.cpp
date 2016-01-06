#include <mbgl/map/source_info.hpp>
#include <mbgl/map/tile_id.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/token.hpp>
#include <mbgl/platform/log.hpp>

#include <mapbox/geojsonvt.hpp>
#include <mapbox/geojsonvt/convert.hpp>

namespace mbgl {

SourceInfo::~SourceInfo() = default;

std::string SourceInfo::tileURL(const TileID& id, float pixelRatio) const {
    return util::replaceTokens(tiles.at(0), [&](const std::string& token) -> std::string {
        if (token == "z") {
            return util::toString(std::min(id.z, static_cast<int8_t>(max_zoom)));
        } else if (token == "x") {
            return util::toString(id.x);
        } else if (token == "y") {
            return util::toString(id.y);
        } else if (token == "prefix") {
            std::string prefix{ 2 };
            prefix[0] = "0123456789abcdef"[id.x % 16];
            prefix[1] = "0123456789abcdef"[id.y % 16];
            return prefix;
        } else if (token == "ratio") {
            return pixelRatio > 1.0 ? "@2x" : "";
        } else {
            return "";
        }
    });
}

namespace {

void parse(const JSValue& value, std::vector<std::string>& target, const char* name) {
    if (!value.HasMember(name)) {
        return;
    }

    const JSValue& property = value[name];
    if (!property.IsArray()) {
        Log::Warning(Event::ParseStyle, "%s must be an array", name);
        return;
    }

    for (rapidjson::SizeType i = 0; i < property.Size(); i++) {
        if (!property[i].IsString()) {
            Log::Warning(Event::ParseStyle, "%s members must be strings", name);
            return;
        }
    }

    for (rapidjson::SizeType i = 0; i < property.Size(); i++) {
        target.emplace_back(std::string(property[i].GetString(), property[i].GetStringLength()));
    }
}

void parse(const JSValue& value, std::string& target, const char* name) {
    if (!value.HasMember(name)) {
        return;
    }

    const JSValue& property = value[name];
    if (!property.IsString()) {
        Log::Warning(Event::ParseStyle, "%s must be a string", name);
        return;
    }

    target = { property.GetString(), property.GetStringLength() };
}

void parse(const JSValue& value, uint16_t& target, const char* name) {
    if (!value.HasMember(name)) {
        return;
    }

    const JSValue& property = value[name];
    if (!property.IsUint()) {
        Log::Warning(Event::ParseStyle, "%s must be an unsigned integer", name);
        return;
    }

    unsigned int uint = property.GetUint();
    if (uint > std::numeric_limits<uint16_t>::max()) {
        Log::Warning(Event::ParseStyle, "values for %s that are larger than %d are not supported", name, std::numeric_limits<uint16_t>::max());
        return;
    }

    target = uint;
}

template <size_t N>
void parse(const JSValue& value, std::array<float, N>& target, const char* name) {
    if (!value.HasMember(name)) {
        return;
    }

    const JSValue& property = value[name];
    if (!property.IsArray() || property.Size() != N) {
        Log::Warning(Event::ParseStyle, "%s must be an array with %d numbers", name, N);
        return;
    }

    for (rapidjson::SizeType i = 0; i < property.Size(); i++) {
        if (!property[i].IsNumber()) {
            Log::Warning(Event::ParseStyle, "%s members must be numbers", name);
        return;
        }
    }

    for (rapidjson::SizeType i = 0; i < property.Size(); i++) {
        target[i] = property[i].GetDouble();
    }
}

} // end namespace

void SourceInfo::parseTileJSON(const JSValue& value) {
    parse(value, tiles, "tiles");
    parse(value, tile_size, "tileSize");
    parse(value, min_zoom, "minzoom");
    parse(value, max_zoom, "maxzoom");
    parse(value, attribution, "attribution");
    parse(value, center, "center");
    parse(value, bounds, "bounds");
}

void SourceInfo::parseGeoJSON(const JSValue& value) {
    using namespace mapbox::geojsonvt;

    try {
        geojsonvt = std::make_unique<GeoJSONVT>(Convert::convert(value, 0));
    } catch (const std::exception& ex) {
        Log::Error(Event::ParseStyle, "Failed to parse GeoJSON data: %s", ex.what());
        // Create an empty GeoJSON VT object to make sure we're not infinitely waiting for
        // tiles to load.
        geojsonvt = std::make_unique<GeoJSONVT>(std::vector<ProjectedFeature>{});
    }
}

} // namespace mbgl
