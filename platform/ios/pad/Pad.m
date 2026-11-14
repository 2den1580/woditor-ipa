#import "PadInternal.h"

static NSString* layoutFile(void) {
  NSString* dir = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  [NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
  return [dir stringByAppendingPathComponent:@"gamepadPos.json"];
}

static NSMutableDictionary* parseJson(NSData* d) {
  if (!d.length) return nil;
  id j = [NSJSONSerialization JSONObjectWithData:d options:NSJSONReadingMutableContainers error:nil];
  return [j isKindOfClass:NSMutableDictionary.class] ? j : nil;
}

static NSMutableDictionary* button(NSString* shape, NSString* name, int code, double px, double py, BOOL left) {
  return [@{@"shape" : shape, @"keyName" : name, @"keyCode" : @(code), @"alpha" : @1, @"scale" : @1,
            @"pctX" : @(px), @"pctY" : @(py), @"isLeft" : @(left), @"isTop" : @NO} mutableCopy];
}

static NSMutableDictionary* defaultLayout(void) {
  NSMutableDictionary* d = [NSMutableDictionary new];
  d[NSUUID.UUID.UUIDString.lowercaseString] = button(@"rectangle", @"ESCAPE", 111, 0.12, 0.15, NO);
  d[NSUUID.UUID.UUIDString.lowercaseString] = button(@"rectangle", @"ENTER", 66, 0.12, 0.25, NO);
  NSMutableDictionary* dp = button(@"dpad", @"", 0, 0.06, 0.06, YES);
  [dp removeObjectForKey:@"keyCode"];
  d[NSUUID.UUID.UUIDString.lowercaseString] = dp;
  return d;
}

NSMutableDictionary* PadGameDefaultLayout(NSArray<NSString*>* gameDirs) {
  for (NSString* dir in gameDirs) {
    NSMutableDictionary* j = parseJson([NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:@"gamepadPos.json"]]);
    if (j) return j;
  }
  return defaultLayout();
}

static NSData* g_cached;

void PadWriteLayout(NSDictionary* layout) {
  NSData* d = [NSJSONSerialization dataWithJSONObject:layout options:NSJSONWritingPrettyPrinted error:nil];
  if (!d) return;
  g_cached = d;
  [d writeToFile:layoutFile() atomically:YES];
}

NSMutableDictionary* PadReadLayout(NSArray<NSString*>* gameDirs) {
  if (!g_cached) {
    NSString* f = layoutFile();
    NSData* d = [NSData dataWithContentsOfFile:f];
    NSMutableDictionary* j = parseJson(d);
    if (!j && d) {
      NSLog(@"Woditor: gamepadPos.json is broken — using a fresh layout (%@.broken)", f.lastPathComponent);
      [d writeToFile:[f stringByAppendingString:@".broken"] atomically:YES];
    }
    if (!j) j = PadGameDefaultLayout(gameDirs);
    if (!d || !parseJson(d)) PadWriteLayout(j);
    else g_cached = d;
  }
  NSMutableDictionary* j = parseJson(g_cached);
  return j ? j : [NSMutableDictionary new];
}

@implementation PadPassView
- (UIView*)hitTest:(CGPoint)p withEvent:(UIEvent*)e {
  UIView* v = [super hitTest:p withEvent:e];
  return (v == self && !_blockAll) ? nil : v;
}
@end

@implementation PadEntry
@end

@interface WPad ()
- (void)buildMenu;
- (void)placeMenu;
- (void)collapseMenuNow;
@end

@implementation WPad {
  __weak UIView* _host;

  UIView *_menu, *_tongue;
  PadStyledView* _menuBtn;
  NSMutableArray<PadStyledView*>* _menuItems;
  BOOL _menuOpen, _menuRight, _dragging, _dimmed;
  CGPoint _dragStart, _menuStart;
  NSTimeInterval _lastTap;
}

- (instancetype)initWithHost:(UIView*)host gameDirs:(NSArray<NSString*>*)dirs {
  if ((self = [super init])) {
    _host = host;
    _dirs = dirs ?: @[];
    _entries = [NSMutableDictionary new];
    _style = PadStyle.load;
    PadIconsSetGameDirs(_dirs);
    _root = [[PadPassView alloc] initWithFrame:host.bounds];
    _root.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _root.multipleTouchEnabled = YES;
    _buttons = [[PadPassView alloc] initWithFrame:_root.bounds];
    _buttons.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _buttons.multipleTouchEnabled = YES;
    [_root addSubview:_buttons];
    [host addSubview:_root];
    _area = host.bounds.size;
    PadSetScreenMinSide(fmin(UIScreen.mainScreen.bounds.size.width, UIScreen.mainScreen.bounds.size.height));
    [self sync];
    [self buildMenu];
  }
  return self;
}

- (UIView*)view { return _root; }

- (void)sendKey:(BOOL)down key:(int)k {
  if (_onKey) _onKey(down, k);
}

- (PadButtonView*)createView:(int)kind {
  PadButtonView* v = kind == PAD_VIEW_STICK ? [PadStickView new]
                     : kind == PAD_VIEW_DPAD ? [PadDPadView new]
                                             : [PadButtonView new];
  __weak WPad* ws = self;
  v.onKeyDown = ^(int k) { [ws sendKey:YES key:k]; };
  v.onKeyUp = ^(int k) { [ws sendKey:NO key:k]; };
  v.style = _style;
  v.editMode = _editing;
  return v;
}

- (void)place:(UIView*)v cfg:(NSDictionary*)cfg size:(CGSize)size {
  double px = [cfg[@"pctX"] doubleValue], py = [cfg[@"pctY"] doubleValue];
  BOOL left = cfg[@"isLeft"] ? [cfg[@"isLeft"] boolValue] : YES, top = cfg[@"isTop"] ? [cfg[@"isTop"] boolValue] : YES;
  CGFloat W = _area.width, H = _area.height;
  CGFloat mx = fmax(0, fmin((int)(W * px), fmax(0, W - size.width)));
  CGFloat my = fmax(0, fmin((int)(H * py), fmax(0, H - size.height)));
  CGFloat x = left ? mx : W - mx - size.width, y = top ? my : H - my - size.height;
  v.bounds = CGRectMake(0, 0, size.width, size.height);
  v.center = CGPointMake(x + size.width / 2, y + size.height / 2);
}

- (void)apply:(PadEntry*)e cfg:(NSDictionary*)cfg {
  NSString* shape = PadShapeNormalize(cfg[@"shape"] ?: @"circle");
  BOOL shapeChanged = ![shape isEqualToString:e.shape];
  PadButtonView* v = e.view;
  v.alpha = cfg[@"alpha"] ? [cfg[@"alpha"] doubleValue] : 1;
  v.padScale = cfg[@"scale"] ? [cfg[@"scale"] doubleValue] : 1;
  CGSize size = PadShapeSize(shape);
  if (e.kind != PAD_VIEW_BUTTON) {
    PadDPadView* d = (PadDPadView*)v;
    d.diagonal = _diagonal;
    d.shape = shape;
    if (shapeChanged && e.kind == PAD_VIEW_DPAD) {
      d.backgroundShape = [PadShape backgroundFor:shape];
      d.foreground = [PadForeground foregroundFor:shape];
    }
  } else {
    if (shapeChanged) v.backgroundShape = [PadShape backgroundFor:shape];
    v.key = [cfg[@"keyCode"] intValue];
    v.toggle = [cfg[@"toggle"] boolValue];
    NSString* icon = [cfg[@"icon"] isKindOfClass:NSString.class] ? cfg[@"icon"] : @"";
    if (!icon.length || ![icon isEqualToString:e.icon]) {
      PadForeground* fg = PadIconFG(icon);
      if (fg) {
        v.foregroundFraction = 0.6;
        v.foreground = fg;
        e.icon = icon;
      } else {
        v.foregroundFraction = 0.9;
        v.foregroundText = [cfg[@"keyName"] isKindOfClass:NSString.class] ? cfg[@"keyName"] : @"UNK";
        e.icon = nil;
      }
    }
  }
  [self place:v cfg:cfg size:size];
  e.shape = shape;
}

- (void)syncRelayout:(BOOL)relayout {
  NSDictionary* layout = PadReadLayout(_dirs);

  for (NSString* idk in _entries.allKeys) {
    NSDictionary* cfg = layout[idk];
    PadEntry* e = _entries[idk];
    if (![cfg isKindOfClass:NSDictionary.class] || PadShapeViewKind(cfg[@"shape"] ?: @"circle") != e.kind) {
      [e.view releaseKeys];
      [e.view removeFromSuperview];
      [_entries removeObjectForKey:idk];
    }
  }

  for (NSString* idk in [layout.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
    NSDictionary* cfg = layout[idk];
    if (![cfg isKindOfClass:NSDictionary.class]) continue;
    NSString* json = [[NSString alloc]
        initWithData:[NSJSONSerialization dataWithJSONObject:cfg options:NSJSONWritingSortedKeys error:nil]
            encoding:NSUTF8StringEncoding];
    PadEntry* e = _entries[idk];
    if (!e) {
      e = [PadEntry new];
      e.kind = PadShapeViewKind(cfg[@"shape"] ?: @"circle");
      e.view = [self createView:e.kind];
      _entries[idk] = e;
      [_buttons addSubview:e.view];
    } else if ([json isEqualToString:e.json]) {
      if (relayout) [self place:e.view cfg:cfg size:PadShapeSize(e.shape)];
      continue;
    }
    [self apply:e cfg:cfg];
    e.json = json;
  }
  [self editorAfterSync];
}

- (void)sync { [self syncRelayout:NO]; }

- (void)areaChanged {
  CGSize s = _host.bounds.size;
  if (s.width <= 0 || s.height <= 0 || CGSizeEqualToSize(s, _area)) return;
  _area = s;
  PadSetScreenMinSide(fmin(UIScreen.mainScreen.bounds.size.width, UIScreen.mainScreen.bounds.size.height));
  [self syncRelayout:YES];
  [self collapseMenuNow];
  [self placeMenu];
  [self editorAreaChanged];
  [self statsAreaChanged];
}

- (void)releaseAll {
  for (PadEntry* e in _entries.allValues) [e.view releaseKeys];
}

- (void)demoStep:(int)step {
  for (UIView* v in [_root.subviews copy])
    if ([v isKindOfClass:PadSheet.class]) [v removeFromSuperview];
  [self removePanel];
  [self dismissEditorMenu];
  switch (step) {
    case 0: [self toggleStatsWindow]; break;
    case 1: [self toggleEditMode]; break;
    case 2: {
      PadEntry* e = _entries.allValues.firstObject;
      for (PadEntry* x in _entries.allValues)
        if (x.kind == PAD_VIEW_BUTTON) e = x;
      if (e) [self showPanelFor:e.view];
      break;
    }
    case 3: [self showEditorMenu]; break;
    case 4: [self showCreateButton]; break;
    case 5: [self showStyleWindow]; break;
    case 6: [self showPresets]; break;
    case 7: [self showMouseSettings]; break;
    default: break;
  }
}

- (void)statsFps:(int)render screen:(int)screen { [self statsUpdateFps:render screen:screen]; }
- (void)statsTitle:(NSString*)title { [self statsUpdateTitle:title]; }

- (void)setStyleAndSave:(PadStyle*)st {
  [st save];
  _style = st;
  for (PadEntry* e in _entries.allValues) e.view.style = st;
  [_menuBtn setStyle:st];
  for (PadStyledView* b in _menuItems) b.style = st;
}

- (void)setMenuHidden:(BOOL)hidden {
  [self collapseMenuNow];
  _menu.hidden = hidden;
}

- (void)setPadVisible:(BOOL)vis {
  if (!vis) [self releaseAll];
  _buttons.hidden = !vis;
}

- (PadStyledView*)menuButton:(NSString*)bg icon:(NSString*)icon size:(CGFloat)s {
  PadStyledView* b = [[PadStyledView alloc] initWithFrame:CGRectMake(0, 0, s, s)];
  b.foregroundFraction = 0.6;
  b.backgroundShape = [PadShape backgroundFor:bg];
  b.foreground = [PadForeground vector:icon];
  b.style = _style;
  return b;
}

- (void)buildMenu {
  CGFloat main = PadSdp(36), item = PadSdp(28);
  _menu = [[UIView alloc] initWithFrame:CGRectMake(0, 0, main, main)];
  _tongue = [[UIView alloc] initWithFrame:CGRectMake(0, 0, main, main)];
  _tongue.backgroundColor = PadColor(0x80000000);
  _tongue.layer.cornerRadius = fmin(PadSdp(24), main / 2);
  _tongue.clipsToBounds = YES;
  _tongue.hidden = YES;
  _menuBtn = [self menuButton:@"circle" icon:@"menus" size:main];
  _menuItems = [NSMutableArray new];
  NSArray* items = @[ @[ @"keyboard", @"pad" ], @[ @"settings", @"edit" ], @[ @"stats", @"stats" ], @[ @"mouse", @"mouse" ] ];
  for (NSArray* it in items) {
    PadStyledView* b = [self menuButton:@"square" icon:it[0] size:item];
    b.accessibilityIdentifier = it[1];
    UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(menuItemTap:)];
    [b addGestureRecognizer:tap];
    [_tongue addSubview:b];
    [_menuItems addObject:b];
  }
  [_menu addSubview:_tongue];
  [_menu addSubview:_menuBtn];
  [_root addSubview:_menu];
  UIPanGestureRecognizer* pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(menuPan:)];
  [_menuBtn addGestureRecognizer:pan];
  UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(menuTap:)];
  [_menuBtn addGestureRecognizer:tap];
  _dimmed = [NSUserDefaults.standardUserDefaults boolForKey:@"floating_menu.menu_dimmed"];
  _menu.alpha = _dimmed ? 0.2 : 1;
  [self placeMenu];
}

- (CGSize)freeArea {
  CGFloat m = _menuBtn.bounds.size.width;
  return CGSizeMake(fmax(0, _area.width - m), fmax(0, _area.height - m));
}

- (void)placeMenu {
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  CGSize free = [self freeArea];
  CGFloat x, y;
  if ([d objectForKey:@"floating_menu.menu_px"]) {
    x = [d doubleForKey:@"floating_menu.menu_px"] * free.width;
    y = [d doubleForKey:@"floating_menu.menu_py"] * free.height;
  } else {
    UIEdgeInsets safe = _host.safeAreaInsets;
    x = safe.left + 12;
    y = safe.top + 12;
  }
  CGFloat m = _menuBtn.bounds.size.width;
  _menu.frame = CGRectMake(fmax(0, fmin(x, free.width)), fmax(0, fmin(y, free.height)), m, m);
  _menuBtn.frame = CGRectMake(0, 0, m, m);
}

- (void)savePosition {
  CGSize free = [self freeArea];
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  [d setDouble:fmax(0, fmin(1, _menu.frame.origin.x / fmax(1, free.width))) forKey:@"floating_menu.menu_px"];
  [d setDouble:fmax(0, fmin(1, _menu.frame.origin.y / fmax(1, free.height))) forKey:@"floating_menu.menu_py"];
}

- (void)menuPan:(UIPanGestureRecognizer*)g {
  if (_menuOpen) return;
  if (g.state == UIGestureRecognizerStateBegan) _menuStart = _menu.frame.origin;
  CGPoint t = [g translationInView:_root];
  CGSize free = [self freeArea];
  CGRect f = _menu.frame;
  f.origin.x = fmax(0, fmin(_menuStart.x + t.x, free.width));
  f.origin.y = fmax(0, fmin(_menuStart.y + t.y, free.height));
  _menu.frame = f;
  if (g.state == UIGestureRecognizerStateEnded || g.state == UIGestureRecognizerStateCancelled) [self savePosition];
}

- (void)menuTap:(UITapGestureRecognizer*)g {
  NSTimeInterval now = NSDate.timeIntervalSinceReferenceDate;
  if (now - _lastTap < 0.25) {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(toggleMenu) object:nil];
    _dimmed = !_dimmed;
    _menu.alpha = _dimmed ? 0.2 : 1;
    [NSUserDefaults.standardUserDefaults setBool:_dimmed forKey:@"floating_menu.menu_dimmed"];
    _lastTap = 0;
    return;
  }
  _lastTap = now;
  [self performSelector:@selector(toggleMenu) withObject:nil afterDelay:0.25];
}

- (void)toggleMenu {
  CGFloat m = _menuBtn.bounds.size.width, item = PadSdp(28), gap = 4;
  CGFloat full = m + gap + _menuItems.count * (item + 2 * gap) + gap;
  CGRect btnScreen = [_menu convertRect:_menuBtn.frame toView:_root];
  if (_menuOpen) {
    [UIView animateWithDuration:0.25 animations:^{
      if (self->_menuRight) {
        self->_tongue.frame = CGRectMake(self->_menu.bounds.size.width - m, 0, m, m);
      } else {
        self->_tongue.frame = CGRectMake(0, 0, m, m);
      }
    } completion:^(BOOL done) {
      self->_tongue.hidden = YES;
      [self restoreMenuFrameKeepingButton];
    }];
    _menuOpen = NO;
    return;
  }
  _menuRight = CGRectGetMidX(btnScreen) > _area.width / 2;

  CGFloat ox = _menuRight ? CGRectGetMaxX(btnScreen) - full : btnScreen.origin.x;
  _menu.frame = CGRectMake(ox, btnScreen.origin.y, full, m);
  _menuBtn.frame = CGRectMake(_menuRight ? full - m : 0, 0, m, m);
  CGFloat x = _menuRight ? gap : m + gap;
  for (PadStyledView* b in _menuItems) {
    b.frame = CGRectMake(x + gap + (_menuRight ? 0 : 0), (m - item) / 2, item, item);
    x += item + 2 * gap;
  }
  _tongue.frame = _menuRight ? CGRectMake(full - m, 0, m, m) : CGRectMake(0, 0, m, m);
  _tongue.hidden = NO;
  [UIView animateWithDuration:0.25 animations:^{ self->_tongue.frame = CGRectMake(0, 0, full, m); }];
  _menuOpen = YES;
}

- (void)restoreMenuFrameKeepingButton {
  CGRect btnScreen = [_menu convertRect:_menuBtn.frame toView:_root];
  CGFloat m = _menuBtn.bounds.size.width;
  _menu.frame = CGRectMake(btnScreen.origin.x, btnScreen.origin.y, m, m);
  _menuBtn.frame = CGRectMake(0, 0, m, m);
}

- (void)collapseMenuNow {
  if (!_menuOpen) return;
  _menuOpen = NO;
  _tongue.hidden = YES;
  [self restoreMenuFrameKeepingButton];
}

- (void)menuItemTap:(UITapGestureRecognizer*)g {
  NSString* what = g.view.accessibilityIdentifier;
  if ([what isEqualToString:@"pad"]) [self setPadVisible:self.buttons.hidden];
  else if ([what isEqualToString:@"edit"]) {

    if (!self.editing) [self setPadVisible:YES];
    [self toggleEditMode];
  } else if ([what isEqualToString:@"stats"]) [self toggleStatsWindow];
  else if ([what isEqualToString:@"mouse"]) [self showMouseSettings];
}
@end
