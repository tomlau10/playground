#import <UIKit/UIKit.h>


@interface UIActor : UIImageView {
	BOOL isPlayer;
    BOOL isBoss;
	float speed;
	int hp, _hp;
	int interval, _interval;
	CGPoint destination;    
}
@property (readwrite,assign) BOOL isPlayer;
@property (readwrite,assign) BOOL isBoss;
@property (readwrite,assign) float speed;
@property (readwrite,assign) int hp;
@property (readwrite,assign) int _hp;
@property (readwrite,assign) int interval;
@property (readwrite,assign) int _interval;
@property (readwrite,assign) CGPoint destination;

@end
