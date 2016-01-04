#include <mbgl/storage/resource.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/algorithm/string.hpp>
#pragma GCC diagnostic pop

namespace mbgl {

namespace algo = boost::algorithm;

Resource::Resource(Kind kind_, const std::string& url_)
    : kind(kind_), isAsset(algo::starts_with(url_, "asset://")), url(url_) {
}

} // namespace mbgl
