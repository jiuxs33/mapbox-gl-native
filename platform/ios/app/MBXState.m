#import "MBXState.h"

NSString *const MBXCamera = @"MBXCamera";
NSString *const MBXUserTrackingMode = @"MBXUserTrackingMode";
NSString *const MBXShowsUserLocation = @"MBXShowsUserLocation";
NSString *const MBXDebugMaskValue = @"MBXDebugMaskValue";
NSString *const MBXShowsZoomLevelOrnament =  @"MBXShowsZoomLevelOrnament";
NSString *const MBXShowsTimeFrameGraph = @"MBXShowsFrameTimeGraph";
NSString *const MBXDebugLoggingEnabled = @"MGLMapboxMetricsDebugLoggingEnabled";
NSString *const MBXShowsMapScale = @"MBXMapShowsScale";
NSString *const MBXMapShowsHeadingIndicator = @"MBXMapShowsHeadingIndicator";
NSString *const MBXMapFramerateMeasurementEnabled = @"MBXMapFramerateMeasurementEnabled";

@interface MBXState()

@end

@implementation MBXState

- (NSString*) debugDescription {
    return [NSString stringWithFormat:@"Camera: %@\nTracking mode: %lu\nShows user location: %@\nDebug mask value: %lu\nShows zoom level ornament: %@\nShows time frame graph: %@\nDebug logging enabled: %@\nShows map scale: %@\nShows user heading indicator: %@\nFramerate measurement enabled: %@", self.camera, (unsigned long)self.userTrackingMode, (self.showsUserLocation) ? @"YES" : @"NO", (unsigned long)self.debugMask, (self.showsZoomLevelOrnament) ? @"YES" : @"NO", (self.showsTimeFrameGraph) ? @"YES" : @"NO", (self.debugLoggingEnabled) ? @"YES" : @"NO", (self.showsMapScale) ? @"YES" : @"NO", (self.showsUserHeadingIndicator) ? @"YES" : @"NO", (self.framerateMeasurementEnabled) ? @"YES" : @"NO"];
}

@end
