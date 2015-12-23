//
//  MGLOfflineMapDownloader.mm
//  mbgl
//
//  Created by Adam Hunter on 12/22/15.
//
//

#import "MGLOfflineMapDownloader.h"

#include <string>
#include <mbgl/storage/sqlite_cache.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/platform/log.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/thread_context.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/storage/file_source.hpp>

@implementation MGLOfflineMapDownloader


+(MGLOfflineMapDownloader *) beginDownloadingStyleURL:(NSURL *)styleURL
                                             delegate:(NSObject<MGLMapViewDelegate> *)delegate
                                     coordinateBounds:(MGLCoordinateBounds)coordinateBounds
                                             minimumZ:(float)minimumZ
                                             maximumZ:(float)maximumZ
{
    MGLOfflineMapDownloader *result = [[MGLOfflineMapDownloader alloc] init];
    if(result) {
        (void)delegate;
        (void)coordinateBounds;
        (void)minimumZ;
        (void)maximumZ;
        
        // setup mbgl cache & file source
        NSString *fileCachePath = @"";
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
        if ([paths count] != 0) {
            NSString *libraryDirectory = [paths objectAtIndex:0];
            fileCachePath = [libraryDirectory stringByAppendingPathComponent:@"offline_cache.db"];
        }
        std::shared_ptr<mbgl::SQLiteCache> mbglFileCache = mbgl::SharedSQLiteCache::get([fileCachePath UTF8String]);
        mbgl::DefaultFileSource *mbglFileSource = new mbgl::DefaultFileSource(mbglFileCache.get(), "", "");
        
        //First step - download the style URL
        /*const size_t pos = styleURL.rfind('/');
        std::string base = "";
        if (pos != std::string::npos) {
            base = styleURL.substr(0, pos + 1);
        }*/
        std::string styleURLAsString = [[styleURL absoluteString] cStringUsingEncoding:NSUTF8StringEncoding];
        const size_t pos = styleURLAsString.rfind('/');
        std::string base = "";
        if (pos != std::string::npos) {
            base = styleURLAsString.substr(0, pos + 1);
        }
        //mbgl::FileSource* fs = mbgl::util::ThreadContext::getFileSource();
        std::unique_ptr<mbgl::FileRequest> styleRequest;
        styleRequest = mbglFileSource->request({ mbgl::Resource::Kind::Style, [[styleURL absoluteString] cStringUsingEncoding: NSUTF8StringEncoding] }, [&styleRequest, base](mbgl::Response res) {
            if (res.stale) {
                // Only handle fresh responses.
                return;
            }
            styleRequest = nullptr;
            
            if (res.error) {
                /*if (res.error->reason == mbgl::Response::Error::Reason::NotFound && styleURL.find("mapbox://") == 0) {
                    Log::Error(Event::Setup, "style %s could not be found or is an incompatible legacy map or style", styleURL.c_str());
                } else {*/
                mbgl::Log::Error(mbgl::Event::Setup, "loading style failed: %s", res.error->message.c_str());
                    //data.loading = false;
                //}
            } else {
                //loadStyleJSON(*res.data, base);
                mbgl::Log::Error(mbgl::Event::Setup, "successfully loaded style");
            }
            
        });
    }
    return result;
}

-(void) suspend
{
    
}

-(void) resume
{
    
}

-(void) cancel
{
    
}

@end
