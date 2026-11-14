#pragma once
#import "Pad.h"
#import "PadUI.h"
#import "PadViews.h"

@interface PadEntry : NSObject
@property(nonatomic, strong) PadButtonView* view;
@property(nonatomic) int kind;
@property(nonatomic, copy) NSString *json, *shape, *icon;
@end

@interface PadPassView : UIView
@property(nonatomic) BOOL blockAll;
@end

@interface WPad ()
@property(nonatomic, strong) PadPassView *root, *buttons;
@property(nonatomic, strong) NSMutableDictionary<NSString*, PadEntry*>* entries;
@property(nonatomic, strong) PadStyle* style;
@property(nonatomic) CGSize area;
@property(nonatomic, strong) NSArray<NSString*>* dirs;
@property(nonatomic) BOOL editing;
- (void)setMenuHidden:(BOOL)hidden;
- (void)setStyleAndSave:(PadStyle*)st;
- (void)place:(UIView*)v cfg:(NSDictionary*)cfg size:(CGSize)size;
- (void)setPadVisible:(BOOL)vis;
@end

@interface WPad (Editor)
- (void)toggleEditMode;
- (void)editorAfterSync;
- (void)editorAreaChanged;
- (void)showCreateButton;
- (void)showEditorMenu;
- (void)showPanelFor:(PadButtonView*)button;
- (void)removePanel;
- (void)dismissEditorMenu;
@end

@interface WPad (Windows)
- (void)showStyleWindow;
- (void)showPresets;
- (void)showMouseSettings;
- (void)toggleStatsWindow;
- (void)statsUpdateFps:(int)render screen:(int)screen;
- (void)statsUpdateTitle:(NSString*)title;
- (void)statsAreaChanged;
@end

@interface PadIconPicker : UIScrollView
+ (PadIconPicker*)pickerWithTile:(CGFloat)tile selected:(NSString*)sel onSelect:(void (^)(NSString* icon))cb;
@end
