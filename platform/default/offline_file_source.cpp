#include "offline_file_source.hpp"
#include "online_file_source.hpp"
#include <mbgl/storage/response.hpp>

#include <mbgl/map/tile_id.hpp>
#include <mbgl/util/thread.hpp>
#include <mbgl/util/exception.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/platform/log.hpp>

#include <cassert>
#include <stdlib.h>

#include "sqlite3.hpp"
#include <sqlite3.h>

namespace mbgl {

using namespace mapbox::sqlite;

class OfflineFileRequest : public FileRequest {
public:
    OfflineFileRequest(std::unique_ptr<WorkRequest> workRequest_)
        : workRequest(std::move(workRequest_)) {
    }

private:
    std::unique_ptr<WorkRequest> workRequest;
};
    
class OfflineStyleFileRequest : public FileRequest {
public:
    OfflineStyleFileRequest(std::unique_ptr<WorkRequest> workRequest_)
    : workRequest(std::move(workRequest_)) {
    }
    
private:
    std::unique_ptr<WorkRequest> workRequest;
};
    
class OfflineStyleFileFakeRequest : public FileRequest {
public:
    OfflineStyleFileFakeRequest() {
        
    }
};

OfflineFileSource::OfflineFileSource(OnlineFileSource *inOnlineFileSource, const std::string& path)
    : thread(std::make_unique<util::Thread<Impl>>(util::ThreadContext{ "OfflineFileSource", util::ThreadType::Unknown, util::ThreadPriority::Low }, path, inOnlineFileSource)),
      onlineFileSource(inOnlineFileSource) {
}

OfflineFileSource::~OfflineFileSource() = default;

class OfflineFileSource::Impl {
public:
    explicit Impl(const std::string& path, OnlineFileSource *inOnlineFileSource);
    ~Impl();

    void handleRequest(Resource, Callback);
    void handleDownloadStyle(const std::string &, Callback);

private:
    void respond(Statement&, Callback);

    const std::string path;
    std::unique_ptr<::mapbox::sqlite::Database> db;
    OnlineFileSource *onlineFileSource;
    std::unique_ptr<FileRequest> styleRequest;
};

OfflineFileSource::Impl::Impl(const std::string& path_, OnlineFileSource *inOnlineFileSource)
    : path(path_),
      onlineFileSource(inOnlineFileSource) {
}

OfflineFileSource::Impl::~Impl() {
    try {
        db.reset();
    } catch (mapbox::sqlite::Exception& ex) {
        Log::Error(Event::Database, ex.code, ex.what());
    }
}

void OfflineFileSource::Impl::handleDownloadStyle(const std::string &url, Callback callback) {
    (void)url;
    (void)callback;
    try {
        if (!db) {
            Log::Error(Event::Database, path.c_str());
            db = std::make_unique<Database>(path.c_str(), ReadOnly);
        }
        
        //First try loading this style
        Statement getStmt = db->prepare("SELECT `value` FROM `metadata` WHERE `name` = ?");
        
        const auto name = "gl_style_" + util::mapbox::canonicalURL(url);
        getStmt.bind(1, name.c_str());
        if (getStmt.run()) {
            Response response;
            //The data is going to be a string representation of JSON
            response.data = std::make_shared<std::string>(getStmt.get<std::string>(0));
            callback(response);
        } else {
            //Actually download the style
            Log::Error(Event::Setup, "loading style %s", url.c_str());
            styleRequest = onlineFileSource->request({ Resource::Kind::Style, url }, [this, url, name, callback](Response res) {
                if (res.stale) {
                    // Only handle fresh responses.
                    return;
                }
                styleRequest = nullptr;
                
                if (res.error) {
                    if (res.error->reason == Response::Error::Reason::NotFound && url.find("mapbox://") == 0) {
                        Log::Error(Event::Setup, "style %s could not be found or is an incompatible legacy map or style", url.c_str());
                    } else {
                        Log::Error(Event::Setup, "loading style failed: %s", res.error->message.c_str());
                    }
                } else {
                    //loadStyleJSON(*res.data, base);
                    //Put this new data in the DB and then call handleDownloadStyle again
                    std::unique_ptr<::mapbox::sqlite::Database> dbWritable;
                    dbWritable = std::make_unique<Database>(path.c_str(), ReadWrite | Create);
                    
                    bool insertStmtResult;
                    {
                        Statement insertStmt = dbWritable->prepare("INSERT INTO metadata (name, value) VALUES (?, ?)");
                        insertStmt.bind(1, name.c_str());
                        insertStmt.bind(2, res.data->c_str());
                        insertStmtResult = insertStmt.run(false);
                    }
                    if (insertStmtResult) {
                        //Start over
                        Log::Error(Event::Database, "Successfully downloaded and stored style, starting over");
                        db.reset();
                        handleDownloadStyle(url, callback);
                        return;
                    } else {
                        Response response;
                        response.error = std::make_unique<Response::Error>(Response::Error::Reason::Other);
                        callback(response);
                    }
                }
                
            });
        }

        
    } catch(const std::exception& ex) {
        Log::Error(Event::Database, ex.what());
        std::string exAsString = std::string(ex.what());
        if ((exAsString.rfind("no such table") != std::string::npos) ||
            (exAsString.rfind("unable to open database") != std::string::npos)) {
            //Create the table in the database
            std::unique_ptr<::mapbox::sqlite::Database> dbWritable;
            dbWritable = std::make_unique<Database>(path.c_str(), ReadWrite | Create);

            bool createStmtResult;
            {
                Statement createStmt = dbWritable->prepare("CREATE TABLE metadata (name TEXT, value TEXT);");
                createStmtResult = createStmt.run(false);
            }
            dbWritable.reset();
            if (createStmtResult) {
                //Start over
                Log::Error(Event::Database, "Created metadata table, starting over");
                //We have to close and re-open the database because we changed the data
                db.reset();
                handleDownloadStyle(url, callback);
                return;
            } else {
                Response response;
                response.error = std::make_unique<Response::Error>(Response::Error::Reason::Other);
                callback(response);
            }
        } else {
            Log::Error(Event::Database, ex.what());
            Response response;
            response.error = std::make_unique<Response::Error>(Response::Error::Reason::NotFound);
            callback(response);
        }
    }
}
    
void OfflineFileSource::Impl::handleRequest(Resource resource, Callback callback) {
    try {
        if (!db) {
            db = std::make_unique<Database>(path.c_str(), ReadOnly);
        }

        if (resource.kind == Resource::Kind::Tile) {
            const auto canonicalURL = util::mapbox::canonicalURL(resource.url);
            auto parts = util::split(canonicalURL, "/");
            const int8_t  z = atoi(parts[parts.size() - 3].c_str());
            const int32_t x = atoi(parts[parts.size() - 2].c_str());
            const int32_t y = atoi(util::split(util::split(parts[parts.size() - 1], ".")[0], "@")[0].c_str());

            const auto id = TileID(z, x, (pow(2, z) - y - 1), z); // flip y for MBTiles

            Statement getStmt = db->prepare("SELECT `tile_data` FROM `tiles` WHERE `zoom_level` = ? AND `tile_column` = ? AND `tile_row` = ?");

            getStmt.bind(1, (int)id.z);
            getStmt.bind(2, (int)id.x);
            getStmt.bind(3, (int)id.y);

            respond(getStmt, callback);

        } else if (resource.kind != Resource::Kind::Unknown) {
            std::string key = "";
            if (resource.kind == Resource::Kind::Glyphs) {
                key = "gl_glyph";
            } else if (resource.kind == Resource::Kind::Source) {
                key = "gl_source";
            } else if (resource.kind == Resource::Kind::SpriteImage) {
                key = "gl_sprite_image";
            } else if (resource.kind == Resource::Kind::SpriteJSON) {
                key = "gl_sprite_metadata";
            } else if (resource.kind == Resource::Kind::Style) {
                key = "gl_style";
            }
            assert(key.length());

            Statement getStmt = db->prepare("SELECT `value` FROM `metadata` WHERE `name` = ?");

            const auto name = key + "_" + util::mapbox::canonicalURL(resource.url);
            getStmt.bind(1, name.c_str());

            respond(getStmt, callback);
        }
    } catch (const std::exception& ex) {
        Log::Error(Event::Database, ex.what());

        Response response;
        response.error = std::make_unique<Response::Error>(Response::Error::Reason::NotFound);
        callback(response);
    }
}

void OfflineFileSource::Impl::respond(Statement& statement, Callback callback) {
    if (statement.run()) {
        Response response;
        response.data = std::make_shared<std::string>(statement.get<std::string>(0));
        callback(response);
    } else {
        Response response;
        response.error = std::make_unique<Response::Error>(Response::Error::Reason::NotFound);
        callback(response);
    }
}

std::unique_ptr<FileRequest> OfflineFileSource::request(const Resource& resource, Callback callback) {
    return std::make_unique<OfflineFileRequest>(thread->invokeWithCallback(&Impl::handleRequest, callback, resource));
}

std::unique_ptr<FileRequest> OfflineFileSource::downloadStyle(const std::string &url, Callback callback) {
    //@TODO: this needs to happen on the Impl thread when the libuv issues get worked out
    thread->invokeSync<void>(&Impl::handleDownloadStyle, url, callback);
    
    return std::make_unique<OfflineStyleFileFakeRequest>();
    //return std::make_unique<OfflineStyleFileRequest>(thread->invokeWithCallback(&Impl::handleDownloadStyle, callback, url));
}
    
} // namespace mbgl
