#pragma once
#import "PadGfx.h"

NSString* PadStr(NSString* key);

void PadIconsSetGameDirs(NSArray<NSString*>* dirs);
NSString* PadIconsUserDir(void);
PadForeground* PadIconFG(NSString* name);
NSArray<NSString*>* PadIconList(void);
BOOL PadIconIsUser(NSString* name);
NSString* PadIconImport(UIImage* img);
void PadIconImportPng(NSString* name, NSData* png);
NSData* PadIconExportPng(NSString* name);
void PadIconDelete(NSString* name);
UIImage* PadIconTileImage(NSString* name, CGFloat size, uint32_t color);

UIImage* PadPreview(NSString* shape, NSString* icon, NSString* text, PadStyle* st, CGSize box);
NSString* PadShapeTitle(NSString* shape);
NSArray<NSString*>* PadButtonShapes(void);
NSArray<NSString*>* PadDpadShapes(void);

NSArray<NSString*>* PadKeyNames(void);
int PadKeyCode(NSString* name);
NSString* PadKeyName(int code);

UIColor* PadUIColor(uint32_t argb);
UILabel* PadLabel(NSString* text, CGFloat size, uint32_t color, BOOL bold);
UIButton* PadButton(NSString* title, uint32_t color, void (^action)(void));
void PadCard(UIView* v, BOOL selected);
UIView* PadHStack(NSArray<UIView*>* views, CGFloat spacing);
UIView* PadVStack(NSArray<UIView*>* views, CGFloat spacing);
UIView* PadFixed(UIView* v, CGFloat w, CGFloat h);
void PadOnTap(UIView* v, void (^action)(void));
UIViewController* PadTopVC(UIView* any);
void PadToast(UIView* host, NSString* text);
void PadConfirm(UIView* host, NSString* message, NSString* okTitle, void (^yes)(void));

@interface PadSheet : UIView
@property(nonatomic, strong, readonly) UIStackView* content;
@property(nonatomic, copy) void (^onClose)(void);
+ (PadSheet*)sheetIn:(UIView*)host title:(NSString*)title subtitle:(NSString*)sub width:(CGFloat)w;
- (void)close;
@end

NSData* PadZip(NSDictionary<NSString*, NSData*>* files);
NSDictionary<NSString*, NSData*>* PadUnzip(NSData* zip);
