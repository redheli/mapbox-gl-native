#ifndef MBGL_STORAGE_RESOURCE
#define MBGL_STORAGE_RESOURCE

#include <string>
#include <functional>

namespace mbgl {

struct Resource {
    enum Kind : uint8_t {
        Unknown = 0,
        Style,
        Source,
        Tile,
        Glyphs,
        SpriteImage,
        SpriteJSON
    };

    Resource(Kind, const std::string&);

    const Kind kind;
    const bool isAsset;
    const std::string url;

    inline bool operator==(const Resource &res) const {
        return kind == res.kind && url == res.url;
    }

    struct Hash {
        std::size_t operator()(Resource const& r) const {
            return std::hash<std::string>()(r.url) ^ (std::hash<uint8_t>()(r.kind) << 1);
        }
    };

};

} // namespace mbgl

#endif
