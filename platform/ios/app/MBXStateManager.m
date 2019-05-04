#import "MBXStateManager.h"
#import <Mapbox/Mapbox.h>
#import "MBXState.h"
#import "MBXViewController.h"

@interface MBXStateManager()

@property (strong, nonatomic) MBXState *currentState;

@end

@implementation MBXStateManager

+ (instancetype) sharedManager {
    static dispatch_once_t once;
    static MBXStateManager* sharedManager;
    dispatch_once(&once, ^{
        sharedManager = [[self alloc] init];
    });

    return sharedManager;
}

- (MBXState*)currentState {
    NSMutableDictionary *mapStateDictionary = [[NSUserDefaults standardUserDefaults] objectForKey:@"mapStateKey"];


    if (mapStateDictionary == nil) {
        _currentState = nil;
    } else {
        _currentState = [[MBXState alloc] init];

        if (mapStateDictionary[MBXCamera]) {
            MGLMapCamera *unpackedCamera = [NSKeyedUnarchiver unarchiveObjectWithData:mapStateDictionary[MBXCamera]];
            _currentState.camera = unpackedCamera;
        }

        _currentState.showsUserLocation = [mapStateDictionary[MBXShowsUserLocation] boolValue];
        _currentState.userTrackingMode = [mapStateDictionary[MBXUserTrackingMode] boolValue];
        _currentState.showsUserHeadingIndicator = [mapStateDictionary[MBXMapShowsHeadingIndicator] boolValue];
        _currentState.showsMapScale = [mapStateDictionary[MBXShowsMapScale] boolValue];
        _currentState.showsZoomLevelOrnament = [mapStateDictionary[MBXShowsZoomLevelOrnament] boolValue];
        _currentState.showsTimeFrameGraph = [mapStateDictionary[MBXShowsTimeFrameGraph] boolValue];
        _currentState.framerateMeasurementEnabled = [mapStateDictionary[MBXMapFramerateMeasurementEnabled] boolValue];
        _currentState.debugMask = ((NSNumber *)mapStateDictionary[MBXDebugMaskValue]).intValue;
        _currentState.debugLoggingEnabled = [mapStateDictionary[MBXDebugLoggingEnabled] boolValue];
    }

    return _currentState;
}

- (void)saveState:(MBXState*)mapState {

    NSMutableDictionary *mapStateDictionary = [NSMutableDictionary dictionary];

    NSData *cameraData = [NSKeyedArchiver archivedDataWithRootObject:mapState.camera];
    [mapStateDictionary setObject:cameraData forKey:MBXCamera];
    [mapStateDictionary setValue:@(mapState.showsUserLocation) forKey:MBXShowsUserLocation];
    [mapStateDictionary setValue:@(mapState.userTrackingMode) forKey:MBXUserTrackingMode];
    [mapStateDictionary setValue:@(mapState.showsUserHeadingIndicator) forKey:MBXMapShowsHeadingIndicator];
    [mapStateDictionary setValue:@(mapState.showsMapScale) forKey:MBXShowsMapScale];
    [mapStateDictionary setValue:@(mapState.showsZoomLevelOrnament) forKey:MBXShowsZoomLevelOrnament];
    [mapStateDictionary setValue:@(mapState.showsTimeFrameGraph) forKey:MBXShowsTimeFrameGraph];
    [mapStateDictionary setValue:@(mapState.framerateMeasurementEnabled) forKey:MBXMapFramerateMeasurementEnabled];
    [mapStateDictionary setValue:@(mapState.debugMask) forKey:MBXDebugMaskValue];
    [mapStateDictionary setValue:@(mapState.debugLoggingEnabled) forKey:MBXDebugLoggingEnabled];

    [[NSUserDefaults standardUserDefaults] setObject:mapStateDictionary forKey:@"mapStateKey"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)resetState {
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"mapStateKey"];
}



@end
