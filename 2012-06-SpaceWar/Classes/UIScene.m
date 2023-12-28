#import "UIScene.h"


@implementation UIScene
@synthesize player;

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    NSArray *touchArray = [touches allObjects];
    [self movePlayerWith:[touchArray objectAtIndex:0]];
	
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    NSArray *touchArray = [touches allObjects];
    [self movePlayerWith:[touchArray objectAtIndex:0]];
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	player.destination = player.center;
}


- (void) movePlayerWith: (UITouch *) touch {
	player.destination = [touch locationInView:self];
}

@end
