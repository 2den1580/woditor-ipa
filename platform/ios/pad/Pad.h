#pragma once
#import <UIKit/UIKit.h>

@interface WPad : NSObject
@property(nonatomic, copy) void (^onKey)(BOOL down, int key);
@property(nonatomic, copy) void (^onMouseSettings)(void);
@property(nonatomic) BOOL diagonal;
@property(nonatomic, readonly) UIView* view;

- (instancetype)initWithHost:(UIView*)host gameDirs:(NSArray<NSString*>*)dirs;
- (void)sync;
- (void)areaChanged;
- (void)releaseAll;
- (void)statsFps:(int)render screen:(int)screen;
- (void)statsTitle:(NSString*)title;
- (void)demoStep:(int)step;
@end

NSMutableDictionary* PadReadLayout(NSArray<NSString*>* gameDirs);
void PadWriteLayout(NSDictionary* layout);
NSMutableDictionary* PadGameDefaultLayout(NSArray<NSString*>* gameDirs);
