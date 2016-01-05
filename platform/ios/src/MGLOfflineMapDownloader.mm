//
//  MGLOfflineMapDownloader.mm
//  mbgl
//
//  Created by Adam Hunter on 12/22/15.
//
//

#import "MGLOfflineMapDownloader.h"
#import "MGLAccountManager.h"

#include <string>
#include <mbgl/storage/sqlite_cache.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/platform/log.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/thread_context.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/storage/file_source.hpp>

#define OFFLINE_DB_PATH "", _offlineMapPath ? _offlineMapPath.UTF8String : ""

@interface MGLOfflineMapDownloader ()

@end

@implementation MGLOfflineMapDownloader

+(MGLOfflineMapDownloader *) beginDownloadingToPath:(NSString *)dbPath
                                           StyleURL:(NSURL *)styleURL
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
        mbgl::DefaultFileSource *mbglFileSource = new mbgl::DefaultFileSource(mbglFileCache.get(), "", dbPath ? dbPath.UTF8String : "");
        mbglFileSource->setAccessToken([[MGLAccountManager accessToken] cStringUsingEncoding:NSUTF8StringEncoding]);
        
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
        std::unique_ptr<mbgl::FileRequest> styleRequest = mbglFileSource->downloadStyle([[styleURL absoluteString] cStringUsingEncoding: NSUTF8StringEncoding],
                                                     [](mbgl::Response res) {
                                                         if (res.stale) {
                                                             return;
                                                         }
                                                         
                                                         if (res.error) {
                                                             mbgl::Log::Error(mbgl::Event::Setup, "loading style failed: %s", res.error->message.c_str());
                                                         } else {
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
