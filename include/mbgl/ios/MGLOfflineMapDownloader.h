//
//  MGLOfflineMapDownloader.h
//  mbgl
//
//  Created by Adam Hunter on 12/22/15.
//
//

#import <Foundation/Foundation.h>

#import "MGLGeometry.h"
#import "MGLMapView.h"

@interface MGLOfflineMapDownloader : NSObject

/** Begin downloading an offline section of map.  Returns an object which encapsulates this request, allowing
    for tracking, etc.
 @param dbPath The file path to the database file to use.
 @param styleURL The URL of the Mapbox Style to use with this map.  Offline map databases are fixed to a particular style because of the need to download the corresponding images and fonts.
 @param delegate A delegate object to recieve progress information
 @param coordinateBounds The bounds of the map region to download
 @param minimumZ minimum zoom level (i.e. highest zoom out)
 @param maximumZ maximum zoom level (i.e. highest zoom in)
 */
+(MGLOfflineMapDownloader *) beginDownloadingToPath:(NSString *)dbPath
                                           StyleURL:(NSURL *)styleURL
                                           delegate:(NSObject<MGLMapViewDelegate> *)delegate
                                   coordinateBounds:(MGLCoordinateBounds)coordinateBounds
                                           minimumZ:(float)minimumZ
                                           maximumZ:(float)maximumZ;
-(void) suspend;
-(void) resume;
-(void) cancel;

@end
