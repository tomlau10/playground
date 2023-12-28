#import <UIKit/UIKit.h>
#import "UIActor.h"

@interface UIScene : UIView {
	UIActor *player;
}
@property (nonatomic,retain) UIActor *player;
- (void) movePlayerWith: (UITouch *) touch;
@end
