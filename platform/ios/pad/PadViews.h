#pragma once
#import "PadGfx.h"

typedef void (^PadKeyBlock)(int key);

@interface PadStyledView : UIView
@property(nonatomic, strong) PadStyle* style;
@property(nonatomic, strong) PadShape* backgroundShape;
@property(nonatomic, strong) PadForeground* foreground;
@property(nonatomic, copy) NSString* foregroundText;
@property(nonatomic) CGFloat foregroundFraction;
@property(nonatomic) CGFloat padScale;
- (void)markDirty;
@end

@interface PadButtonView : PadStyledView
@property(nonatomic) int key;
@property(nonatomic) BOOL toggle;
@property(nonatomic, copy) PadKeyBlock onKeyDown, onKeyUp;
@property(nonatomic) BOOL editMode;
- (void)releaseKeys;
@end

@interface PadDPadView : PadButtonView
@property(nonatomic, copy) NSString* shape;
@property(nonatomic) BOOL diagonal;
@end

@interface PadStickView : PadDPadView
@property(nonatomic) BOOL showZone;
@end
