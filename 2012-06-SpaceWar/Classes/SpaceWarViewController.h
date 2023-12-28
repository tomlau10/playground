#import <UIKit/UIKit.h>
#import "Config.h"
#import "UIScene.h"
#import "UIActor.h"
#import <AVFoundation/AVFoundation.h>

@interface SpaceWarViewController : UIViewController {
	UIImageView *bg1;
    UIImageView *bg2;
    IBOutlet UIScene *scene;
    IBOutletCollection (UIActor) NSArray *enemies;
	IBOutletCollection (UIActor) NSArray *bullets;
    IBOutletCollection (UIActor) NSArray *items;
	IBOutlet UIActor *player;
	IBOutlet UILabel *lblHP;
	IBOutlet UILabel *lblScore;
	IBOutlet UIButton *btnRun;
	IBOutlet UIButton *btnPause;
	BOOL gameStatus;
	int score;
	NSTimer *gameTimer;
}
@property (nonatomic,retain) IBOutlet UIImageView *bg1;
@property (nonatomic,retain) IBOutlet UIImageView *bg2;
@property (nonatomic,retain) UIScene *scene;
@property (nonatomic,retain) NSArray *enemies;
@property (nonatomic,retain) NSArray *bullets;
@property (nonatomic,retain) NSArray *items;
@property (nonatomic,retain) UIActor *player;
@property (nonatomic,retain) UILabel *lblHP;
@property (nonatomic,retain) UILabel *lblScore;
@property (nonatomic,retain) UIButton *btnRun;
@property (nonatomic,retain) UIButton *btnPause;
@property (readwrite,assign) BOOL gameStatus;
@property (nonatomic,retain) NSTimer *gameTimer;
@property (readwrite,assign) int score;
@property (nonatomic,retain) AVAudioPlayer *fire;
@property (nonatomic,retain) AVAudioPlayer *kill;
@property (nonatomic,retain) AVAudioPlayer *item_s;
@property (readwrite,assign) BOOL upgrade;
- (IBAction) gameStart;
- (void) setup;
- (void) playerSetup;
- (void) bulletsSetup;
- (void) enemiesSetup;
- (void) gameLoop: (NSTimer *) timer;
- (BOOL) move: (UIActor *) actor;
- (BOOL) collisionBetween: (UIActor *) a1 and: (UIActor *) a2;
- (void) gameOver;
@end

