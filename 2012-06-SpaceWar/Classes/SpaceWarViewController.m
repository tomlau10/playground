#import "SpaceWarViewController.h"

@implementation SpaceWarViewController
@synthesize fire, kill, scene, enemies, bullets, player, lblHP, lblScore, btnRun, btnPause, gameStatus, score, gameTimer, bg1, bg2, items, item_s, upgrade;

- (void)viewDidLoad {
    self.view.BackgroundColor = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"%@/Galaxy.png"]];
    [super viewDidLoad];
	[self setup];

	gameTimer = [NSTimer scheduledTimerWithTimeInterval:0.04 
												 target:self 
											   selector:@selector(gameLoop:) 
											   userInfo: nil 
												repeats: YES];
    
    NSString *path = [[NSBundle mainBundle] resourcePath];
	NSURL *url;
	NSError *err;
    url = [NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/fire.mp3", path]];
    fire = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&err];
    url = [NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/kill.mp3", path]];
    kill = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&err];
    url = [NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/item.mp3", path]];
    item_s = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&err];
}

- (void) setup {

	[self playerSetup];
	[self bulletsSetup];
	[self enemiesSetup];
    [self itemsSetup];
	
	gameStatus = PAUSE;
	btnRun.alpha = 1;
	btnPause.alpha = 0;
	score = 0;
	[lblScore setText:[NSString stringWithFormat:@"Score: %d", score]];
	[lblHP setText:[NSString stringWithFormat:@"HP: %d", player.hp]];
}

- (void) playerSetup {
	CGPoint pos;
	player.isPlayer = YES;
	player.speed = 50;
	player.hp = 250;
    player._hp = 500;
	pos.x = SCREEN_WIDTH * 0.5;
	pos.y = SCREEN_HEIGHT * 0.75;
	player.center = pos;
	player.destination = player.center;
	scene.player = player;
    upgrade = NO;
}

- (void) bulletsSetup {
	UIActor *actor;
	for (int i=0; i<[bullets count]; i++) {
		actor = [bullets objectAtIndex:i];
		actor.speed = 10;
		actor.hp = actor._hp = 1;
		actor.interval = i * 5;
		actor._interval = 30;
		actor.alpha = 0;
	}
}

- (void) itemsSetup {
	UIActor *actor;
    for (int i=0; i<[items count]; i++) {
		actor = [items objectAtIndex:i];
        actor.speed = 5;
        actor.hp = 0;
        actor._hp = 1;
        actor.alpha = 0;
    }
}

- (void) enemiesSetup {
	UIActor *actor;
	CGPoint pos;
	srand(0);
	
	for (int i=0; i<[enemies count]; i++) {
		actor = [enemies objectAtIndex:i];
		actor.destination = actor.center;
		actor.alpha = 0;
		pos.x = rand() % SCREEN_WIDTH;
		pos.y = -20;
		actor.center = pos;
        if (i==[enemies count]-1) {
            actor.isBoss = YES;
            actor.speed = 3;
            actor._hp = 10;
            actor._interval = 100;
        } else {
            actor.isBoss = NO;
            actor.speed = 1 + i * 0.05;
            actor._hp = 0 + i;
            actor._interval = rand() % 1;
        }
        actor.hp = 0;
        actor.interval = actor._interval;
    }
}

- (IBAction) gameStart {
	if (gameStatus == RUN) {
		gameStatus = PAUSE;
		btnRun.alpha = 1;
		btnPause.alpha = 0;
       	} else {
		gameStatus = RUN;
		btnRun.alpha = 0;
		btnPause.alpha = 1;
        if (player.center.y < (self.view.bounds.size.height/4)){
            float difference = (self.view.bounds.size.height/4) - player.center.y;
            bg1.center = CGPointMake(bg1.center.x, bg1.center.y + (difference/2));
            bg2.center = CGPointMake(bg2.center.x, bg2.center.y + (difference/2));
            if (bg1.center.y > self.view.bounds.size.height + (bg1.bounds.size.height/2)){
                bg1.center = CGPointMake(bg1.center.x, bg2.center.y - 460);
            }
            if (bg2.center.y > self.view.bounds.size.height + (bg2.bounds.size.height/2)){
                bg2.center = CGPointMake(bg2.center.x, bg1.center.y - 460);
            }
        }         	
    }
}

- (void) gameLoop: (NSTimer *) timer {
	if (gameStatus == RUN) {		// if the game is running then ...
		UIActor *bullet, *enemy, *item;
		CGPoint pos;

		[self move: player];		// update the position of player
		
		/** For enemies: check collision (enemies and player) and update position **/
		for (int i=0; i<[enemies count]; i++) {
			
			enemy = [enemies objectAtIndex:i];
			if (enemy.hp > 0) {		// if enemy is still alive then ...
				enemy.destination = player.center;
				enemy.alpha += 0.1;

				[self move: enemy];		// update the position of this enemy
				
				if ([self collisionBetween: enemy and: player]) {	// if it hits the player then ...
					player.hp--;
					[lblHP setText:[NSString stringWithFormat:@"HP: %d", player.hp]];

					if (player.hp <= 0) {		// if player has no hp then ...
						[lblHP setText:[NSString stringWithFormat:@"HP: %d", 0]];
						[self gameOver];
						return;
					}
				}
			} else {		// if enemy dies then ...
				enemy.interval--;
				if (enemy.interval < 0) {		// enemy rebirths 
					enemy.hp = enemy._hp;
                    enemy.interval = enemy._interval;
				}
			}
		}

        /** For items **/
		for (int i=0; i<[items count]; i++) {
            item = [items objectAtIndex:i];
            if(item.hp > 0){
                [self move: item];
                if ([self collisionBetween: item and: player]) {
                    switch (i) {
                        case 0:
                            if (player.hp + 50 > player._hp) {
                                player.hp = player._hp;
                            } else {
                                player.hp += 50;
                            }
                            [lblHP setText:[NSString stringWithFormat:@"HP: %d", player.hp]];
                            break;
                        case 1:
                            for (int k=0; k<[bullets count]; k++) {
                                bullet = [bullets objectAtIndex:k];
                                bullet._hp++;
                            }
                            player.image = [UIImage imageNamed: @"yamato.png"];
                            upgrade = YES;
                            break;
                    }
                    [item_s stop];
                    item_s.currentTime=0;
                    [item_s play];
                    item.alpha = 0;
                    item.hp = 0;
                }
            }
        }
        
		/** For bullets: check collision (bullets and enemies) and update position **/
		for (int i=0; i<[bullets count]; i++) {
			bullet = [bullets objectAtIndex:i];
			bullet.interval--;
			if (bullet.interval < 0) {	// Should this bullet be refired? if yes then ...
				[fire stop];
                fire.currentTime=0;
                [fire play];
                pos = player.center;
				pos.y = -20;
                switch (upgrade) {
                    case YES:
                        bullet.destination = enemy.center;
                        break;
                    default:
                        bullet.destination = pos;
                        break;
                }
				bullet.center = player.center;
				bullet.interval = bullet._interval;
				bullet.alpha = 1;
				bullet.hp = bullet._hp;
			}
			[self move:bullet];		// update the position of this bullet

			if (bullet.hp > 0) {	// bullet has not yet hit anything
				for (int j=0; j<[enemies count]; j++) {
					enemy = [enemies objectAtIndex:j];
					if ([self collisionBetween: bullet and: enemy]) {	// if bullet hits an enemy then ...
						enemy.hp -= bullet.hp;
						bullet.hp--;
                        if (bullet.hp == 0) bullet.alpha = 0;
						bullet.alpha = 0;
						if (enemy.hp <= 0) {	// if enemy has no hp then ...
							[kill stop];
                            kill.currentTime=0;
                            [kill play];
                            pos = enemy.center;
							pos.y = -20;
							pos.x = rand() % SCREEN_WIDTH;
							enemy.center = pos;
                            if (enemy.isBoss) {
                                enemy._hp += 10;
                                int l = rand() % [items count];
                                item = [items objectAtIndex:l];
                                pos.x = rand() % SCREEN_WIDTH;
                                item.center = pos;
                                item.hp = item._hp;
                                pos.y = 500;
                                item.destination = pos;
                                item.alpha = 1;
                            }
							enemy.alpha = 0;
							score++;
							[lblScore setText:[NSString stringWithFormat:@"Score: %d", score]];
						}
					}
				}
			}
		}
	}	
}


- (BOOL) move: (UIActor *) actor {
	BOOL reachDestination;
	float d, x, y, ratio;
	CGPoint pos = actor.center;
	x = actor.destination.x - pos.x;
	y = actor.destination.y - pos.y;
	d = sqrt(y * y + x * x);
	if (d > actor.speed) {
		ratio = d/y;
		pos.y += actor.speed / ratio;
		ratio = d/x;
		pos.x += actor.speed / ratio;
		actor.center = pos;
		reachDestination = NO;
	} else {
		actor.center = actor.destination;
		reachDestination = YES;
	}
	return reachDestination;
   }


- (BOOL) collisionBetween: (UIActor *) a1 and: (UIActor *) a2 {
	CGSize size1 = a1.frame.size;
    CGSize size2 = a2.frame.size;
    float radius1 = (size1.width < size1.height?size1.width:size1.height)/2;
    float radius2 = (size2.width < size2.height?size2.width:size2.height)/2;
	float x = a1.center.x - a2.center.x;
	float y = a1.center.y - a2.center.y;
    float distanceSq =x*x + y*y;
    float collisionDist = radius1 + radius2;
    
    if (distanceSq < (collisionDist * collisionDist))
        return YES;
    else
    return NO;

}


- (void) gameOver {
	gameStatus = PAUSE;
	UIAlertView *alertView = [[[UIAlertView alloc]
							   initWithTitle:@"Space War"
							   message:[NSString stringWithFormat:@"Game Over!! Your score:%d", score] delegate:self
							   cancelButtonTitle:@"Play Again"
							   otherButtonTitles:nil] autorelease];
	[alertView show];
}

- (void)alertView:(UIAlertView *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	[self setup];
}
@end
