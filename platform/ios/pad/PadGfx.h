#pragma once
#import <UIKit/UIKit.h>

typedef struct { const char* d; float fillA, strokeA, strokeW; int cap, join, evenOdd; float tx, ty; } PadVecPath;
typedef struct { const char* name; float vw, vh; const PadVecPath* paths; int n; } PadVec;

const PadVec* PadVecFind(NSString* name);

void PadVecDraw(CGContextRef c, const PadVec* v, CGRect r, UIColor* color);

enum { PAD_DARK = 0, PAD_GLASS = 1, PAD_LIGHT = 2, PAD_NEON = 3 };
enum { PAD_EFFECT_NONE = 0, PAD_EFFECT_SHADOW = 1, PAD_EFFECT_GLOW = 2 };
extern const uint32_t kPadNeonColors[8];

@interface PadStyle : NSObject
@property(nonatomic, readonly) int type;
@property(nonatomic, readonly) uint32_t neonColor;
+ (instancetype)styleWithType:(int)type neon:(uint32_t)neon;
+ (instancetype)defaultStyle;
+ (instancetype)load;
- (void)save;
- (BOOL)isEqualToStyle:(PadStyle*)s;
- (uint32_t)fillColor;
- (uint32_t)knobFillColor;
- (CGFloat)borderDp;
- (uint32_t)borderColor;
- (CGFloat)glowDp;
- (uint32_t)contentColor;
- (int)contentEffect;
- (uint32_t)effectColor;
@end

UIColor* PadColor(uint32_t argb);

enum { PAD_VIEW_BUTTON = 0, PAD_VIEW_DPAD = 1, PAD_VIEW_STICK = 2 };
NSString* PadShapeNormalize(NSString* shape);
int PadShapeViewKind(NSString* shape);
CGSize PadShapeSize(NSString* shape);
CGFloat PadSdp(CGFloat sdp);
void PadSetScreenMinSide(CGFloat pts);

@interface PadShape : NSObject
+ (PadShape*)backgroundFor:(NSString*)shape;
+ (PadShape*)circle;
- (void)fill:(CGContextRef)c rect:(CGRect)r;
- (CGRect)contentRect:(CGRect)r;
- (void)drawDetails:(CGContextRef)c rect:(CGRect)r border:(uint32_t)color width:(CGFloat)w;
@end

@interface PadForeground : NSObject
+ (PadForeground*)foregroundFor:(NSString*)shape;
+ (PadForeground*)vector:(NSString*)name;
+ (PadForeground*)image:(UIImage*)img tint:(BOOL)tint;
- (void)draw:(CGContextRef)c rect:(CGRect)r color:(UIColor*)color;
@end

CGFloat PadRenderPad(PadStyle* st);

UIImage* PadRender(PadShape* bg, PadForeground* fg, CGFloat fgFraction, NSString* text, CGSize size, CGFloat scale,
                   PadStyle* st, BOOL knob);
