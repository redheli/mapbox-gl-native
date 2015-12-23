#ifndef MBGL_STORAGE_DEFAULT_FILE_SOURCE
#define MBGL_STORAGE_DEFAULT_FILE_SOURCE

#include <mbgl/storage/file_source.hpp>

namespace mbgl {

class FileCache;

class DefaultFileSource : public FileSource {
public:
    DefaultFileSource(FileCache*, const std::string& root = "", const std::string& offlinePath = "");
    ~DefaultFileSource() override;

    void setAccessToken(const std::string&);
    std::string getAccessToken() const;

    std::unique_ptr<FileRequest> request(const Resource&, Callback) override;
    
    std::unique_ptr<FileRequest> downloadStyle(const std::string &url, Callback);

private:
    friend class DefaultFileRequest;
    friend class StyleFileRequest;
    class Impl;
    const std::unique_ptr<Impl> impl;
};

} // namespace mbgl

#endif
