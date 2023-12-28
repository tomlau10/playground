#import <UIKit/UIKit.h>

@class SpaceWarViewController;

@interface SpaceWarAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    SpaceWarViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet SpaceWarViewController *viewController;

@end

