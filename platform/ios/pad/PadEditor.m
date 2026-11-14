#import <PhotosUI/PhotosUI.h>
#import <objc/runtime.h>

#import "PadInternal.h"

@interface PadEditState : NSObject
@property(nonatomic, strong) UILabel* hint;
@property(nonatomic, strong) UIButton* fab;
@property(nonatomic, strong) UIView *panel, *scrim, *menu;
@property(nonatomic, weak) UIView* panelButton;
@property(nonatomic) CGPoint dragStart;
@end
@implementation PadEditState
@end

static char kEditKey;

@interface PadBlockSlider : NSObject
@property(nonatomic, copy) void (^change)(void);
@property(nonatomic, copy) void (^end)(void);
- (void)changed;
- (void)ended;
@end
@implementation PadBlockSlider
- (void)changed {
  if (_change) _change();
}
- (void)ended {
  if (_end) _end();
}
@end

static NSString* cfgString(NSDictionary* c, NSString* k) {
  id v = c[k];
  return [v isKindOfClass:NSString.class] ? v : @"";
}

@implementation WPad (Editor)

- (PadEditState*)es {
  PadEditState* s = objc_getAssociatedObject(self, &kEditKey);
  if (!s) {
    s = [PadEditState new];
    objc_setAssociatedObject(self, &kEditKey, s, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return s;
}

- (NSString*)idOfView:(UIView*)v {
  for (NSString* k in self.entries)
    if (self.entries[k].view == v) return k;
  return nil;
}

- (void)saveButton:(NSString*)keyId values:(NSDictionary*)vals {
  NSMutableDictionary* root = PadReadLayout(self.dirs);
  NSMutableDictionary* cfg = [root[keyId] isKindOfClass:NSDictionary.class] ? [root[keyId] mutableCopy] : [NSMutableDictionary new];
  [vals enumerateKeysAndObjectsUsingBlock:^(id k, id v, BOOL* stop) {
    if (v == NSNull.null) [cfg removeObjectForKey:k];
    else cfg[k] = v;
  }];
  root[keyId] = cfg;
  PadWriteLayout(root);
}

- (void)toggleEditMode {
  PadEditState* s = [self es];
  self.editing = !self.editing;
  if (self.editing) {

    self.root.backgroundColor = PadColor(0x99555555);
    self.root.blockAll = YES;
    [self.root bringSubviewToFront:self.buttons];
    UILabel* h = PadLabel(PadStr(@"pad_edit_hint"), 16, 0xFFCCCCCC, NO);
    h.textAlignment = NSTextAlignmentCenter;
    h.backgroundColor = PadColor(0xB0000000);
    h.layer.cornerRadius = 12;
    h.clipsToBounds = YES;
    CGFloat w = self.area.width - 80;
    CGSize hs = [h sizeThatFits:CGSizeMake(w - 32, 1000)];
    h.frame = CGRectMake(40, (self.area.height - hs.height - 24) / 2, w, hs.height + 24);
    h.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    [self.buttons insertSubview:h atIndex:0];
    s.hint = h;

    UIButton* fab = [UIButton buttonWithType:UIButtonTypeCustom];
    [fab setTitle:@"+" forState:UIControlStateNormal];
    fab.titleLabel.font = [UIFont boldSystemFontOfSize:26];
    fab.backgroundColor = PadColor(0xFF2E7D32);
    fab.layer.cornerRadius = 26;
    UIEdgeInsets safe = self.root.safeAreaInsets;
    fab.frame = CGRectMake(self.area.width - safe.right - 16 - 52, safe.top + 16, 52, 52);
    [fab addTarget:self action:@selector(fabTap) forControlEvents:UIControlEventTouchUpInside];
    [fab addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(fabPan:)]];
    [self.buttons addSubview:fab];
    s.fab = fab;

    for (PadEntry* e in self.entries.allValues) [self makeDraggable:e.view];
    [self setMenuHidden:YES];
  } else {
    self.root.backgroundColor = UIColor.clearColor;
    self.root.blockAll = NO;
    [self.root sendSubviewToBack:self.buttons];
    [self removePanel];
    [self dismissEditorMenu];
    [s.hint removeFromSuperview];
    [s.fab removeFromSuperview];
    s.hint = nil;
    s.fab = nil;
    for (PadEntry* e in self.entries.allValues) [self makeStatic:e.view];
    [self sync];
    PadToast(self.root, PadStr(@"Saves"));
    [self setMenuHidden:NO];
  }
}

- (void)editorAfterSync {
  if (!self.editing) return;
  PadEditState* s = [self es];
  if (s.panelButton && ![self idOfView:s.panelButton]) [self removePanel];
  for (PadEntry* e in self.entries.allValues) [self makeDraggable:e.view];
  if (s.fab) [self.buttons bringSubviewToFront:s.fab];
  if (s.panel) [self.buttons bringSubviewToFront:s.panel];
}

- (void)editorAreaChanged { [self removePanel]; }

- (void)makeDraggable:(PadButtonView*)v {
  v.editMode = YES;
  if ([v isKindOfClass:PadStickView.class]) ((PadStickView*)v).showZone = YES;
  for (UIGestureRecognizer* g in v.gestureRecognizers)
    if ([g.name isEqualToString:@"padEdit"]) return;
  UIPanGestureRecognizer* pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(buttonPan:)];
  pan.name = @"padEdit";
  UILongPressGestureRecognizer* lp = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(buttonLong:)];
  lp.name = @"padEdit";
  lp.minimumPressDuration = 0.5;
  lp.allowableMovement = 15;
  [v addGestureRecognizer:pan];
  [v addGestureRecognizer:lp];
}

- (void)makeStatic:(PadButtonView*)v {
  v.editMode = NO;
  if ([v isKindOfClass:PadStickView.class]) ((PadStickView*)v).showZone = NO;
  for (UIGestureRecognizer* g in [v.gestureRecognizers copy])
    if ([g.name isEqualToString:@"padEdit"]) [v removeGestureRecognizer:g];
}

- (void)moveView:(UIView*)v center:(CGPoint)c {
  CGSize b = v.bounds.size;
  c.x = fmax(b.width / 2, fmin(self.area.width - b.width / 2, c.x));
  c.y = fmax(b.height / 2, fmin(self.area.height - b.height / 2, c.y));
  v.center = c;
}

- (void)buttonPan:(UIPanGestureRecognizer*)g {
  PadEditState* s = [self es];
  UIView* v = g.view;
  if (g.state == UIGestureRecognizerStateBegan) {
    [self removePanel];
    s.dragStart = v.center;
  }
  CGPoint t = [g translationInView:self.buttons];
  [self moveView:v center:CGPointMake(s.dragStart.x + t.x, s.dragStart.y + t.y)];
  if (g.state == UIGestureRecognizerStateEnded || g.state == UIGestureRecognizerStateCancelled) [self savePosition:v];
}

- (void)savePosition:(UIView*)v {
  NSString* keyId = [self idOfView:v];
  if (!keyId) return;
  CGSize b = v.bounds.size;
  CGFloat x = v.center.x - b.width / 2, y = v.center.y - b.height / 2, W = self.area.width, H = self.area.height;
  BOOL left = v.center.x < W / 2, top = v.center.y < H / 2;
  double px = fmax(0, left ? x / W : (W - (x + b.width)) / W);
  double py = fmax(0, top ? y / H : (H - (y + b.height)) / H);
  [self saveButton:keyId values:@{@"pctX" : @(px), @"pctY" : @(py), @"isLeft" : @(left), @"isTop" : @(top)}];
  self.entries[keyId].json = nil;
  [self sync];
}

- (void)buttonLong:(UILongPressGestureRecognizer*)g {
  if (g.state == UIGestureRecognizerStateBegan) [self showPanelFor:(PadButtonView*)g.view];
}

- (void)fabTap { [self showEditorMenu]; }

- (void)fabPan:(UIPanGestureRecognizer*)g {
  PadEditState* s = [self es];
  if (g.state == UIGestureRecognizerStateBegan) {
    [self removePanel];
    s.dragStart = g.view.center;
  }
  CGPoint t = [g translationInView:self.buttons];
  [self moveView:g.view center:CGPointMake(s.dragStart.x + t.x, s.dragStart.y + t.y)];
}

- (void)removePanel {
  PadEditState* s = [self es];
  [s.panel removeFromSuperview];
  s.panel = nil;
  s.panelButton = nil;
}

- (UIView*)sliderRow:(NSString*)icon min:(float)mn max:(float)mx value:(float)val
            onChange:(void (^)(float v, UILabel* l))change onEnd:(void (^)(float v))end {
  UIImageView* iv = [[UIImageView alloc] initWithImage:PadIconTileImage(icon, 18, 0xFFFFFFFF)];
  UISlider* sl = [UISlider new];
  sl.minimumValue = mn;
  sl.maximumValue = mx;
  sl.value = val;
  sl.minimumTrackTintColor = PadColor(0xFF4CAF50);
  sl.thumbTintColor = PadColor(0xFF4CAF50);
  UILabel* l = PadLabel([NSString stringWithFormat:@"%d%%", (int)val], 12, 0xFFFFFFFF, NO);
  l.textAlignment = NSTextAlignmentRight;
  PadBlockSlider* t = [PadBlockSlider new];
  t.change = ^{ change(roundf(sl.value), l); };
  t.end = ^{ end(roundf(sl.value)); };
  objc_setAssociatedObject(sl, &kEditKey, t, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [sl addTarget:t action:@selector(changed) forControlEvents:UIControlEventValueChanged];
  [sl addTarget:t action:@selector(ended) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
  return PadHStack(@[ PadFixed(iv, 18, 18), sl, PadFixed(l, 40, 0) ], 6);
}

- (UIButton*)keyButton:(int)current onPick:(void (^)(int code, NSString* name))pick {
  UIButton* b = PadButton(current ? PadKeyName(current) : @"—", 0xFF1F1F1F, nil);
  b.layer.borderColor = PadColor(0xFF3A3A3A).CGColor;
  b.layer.borderWidth = 1;
  b.layer.cornerRadius = 6;
  NSMutableArray* names = [PadKeyNames() mutableCopy];
  NSString* curName = current ? PadKeyName(current) : nil;
  if (curName && ![names containsObject:curName]) [names addObject:curName];
  NSMutableArray* acts = [NSMutableArray new];
  __weak UIButton* wb = b;
  for (NSString* n in names) {
    UIAction* a = [UIAction actionWithTitle:n image:nil identifier:nil handler:^(UIAction* x) {
      [wb setTitle:n forState:UIControlStateNormal];
      int code = PadKeyCode(n);
      if (!code && [n isEqualToString:curName]) code = current;
      pick(code, n);
    }];
    if ([n isEqualToString:curName]) a.state = UIMenuElementStateOn;
    [acts addObject:a];
  }
  b.menu = [UIMenu menuWithTitle:PadStr(@"pad_key") children:acts];
  b.showsMenuAsPrimaryAction = YES;
  return b;
}

- (void)showPanelFor:(PadButtonView*)button {
  [self removePanel];
  NSString* keyId = [self idOfView:button];
  if (!keyId) return;
  PadEditState* s = [self es];
  NSDictionary* cfg = PadReadLayout(self.dirs)[keyId];
  NSNumber* a0 = cfg[@"alpha"] ?: @1;
  NSNumber* s0 = cfg[@"scale"] ?: @1;
  int alpha = MAX(10, (int)(a0.doubleValue * 100));
  int scale = MIN(200, MAX(25, (int)(s0.doubleValue * 100)));
  int keyCode = [cfg[@"keyCode"] intValue];
  NSString* shape = PadShapeNormalize(cfgString(cfg, @"shape"));
  NSString* icon = cfgString(cfg, @"icon").length ? cfgString(cfg, @"icon") : nil;
  BOOL dpad = [button isKindOfClass:PadDPadView.class] || PadShapeViewKind(shape) != PAD_VIEW_BUTTON;
  __weak WPad* ws = self;
  __weak PadButtonView* wbtn = button;

  NSMutableArray* rows = [NSMutableArray new];
  [rows addObject:[self sliderRow:@"eye" min:10 max:100 value:alpha
                         onChange:^(float v, UILabel* l) { l.text = [NSString stringWithFormat:@"%d%%", (int)v]; wbtn.alpha = v / 100; }
                            onEnd:^(float v) { [ws saveButton:keyId values:@{@"alpha" : @(v / 100.0)}]; }]];
  [rows addObject:[self sliderRow:@"ruler" min:25 max:200 value:scale
                         onChange:^(float v, UILabel* l) { l.text = [NSString stringWithFormat:@"%d%%", (int)v]; wbtn.padScale = v / 100; }
                            onEnd:^(float v) { [ws saveButton:keyId values:@{@"scale" : @(v / 100.0)}]; }]];
  if (dpad) {
    UILabel* kind = PadLabel([NSString stringWithFormat:@"%@ %@", PadStr(@"pad_dpad_kind"), PadShapeTitle(shape)], 12, 0xFFB0B0B0, NO);
    [rows addObject:kind];
    UIScrollView* sc = [UIScrollView new];
    sc.showsHorizontalScrollIndicator = NO;
    CGFloat x = 0;
    for (NSString* k in PadDpadShapes()) {
      UIImageView* t = [[UIImageView alloc] initWithImage:PadPreview(k, nil, nil, self.style, CGSizeMake(40, 32))];
      t.contentMode = UIViewContentModeCenter;
      t.frame = CGRectMake(x, 0, 46, 40);
      PadCard(t, [k isEqualToString:shape]);
      PadOnTap(t, ^{
        [ws saveButton:keyId values:@{@"shape" : k}];
        [ws removePanel];
        [ws sync];
      });
      [sc addSubview:t];
      x += 50;
    }
    sc.contentSize = CGSizeMake(x, 40);
    [rows addObject:PadFixed(sc, 0, 40)];
  } else {
    [rows addObject:PadLabel(PadStr(@"pad_key"), 12, 0xFFB0B0B0, NO)];
    [rows addObject:PadFixed([self keyButton:keyCode onPick:^(int code, NSString* name) {
      [ws saveButton:keyId values:@{@"keyCode" : @(code), @"keyName" : name.uppercaseString}];
      [ws sync];
    }], 0, 36)];
    [rows addObject:PadLabel(PadStr(@"pad_icon"), 12, 0xFFB0B0B0, NO)];
    [rows addObject:PadFixed([PadIconPicker pickerWithTile:32 selected:icon onSelect:^(NSString* name) {
      [ws saveButton:keyId values:@{@"icon" : name ?: (id)NSNull.null}];
      [ws sync];
    }], 0, 32)];
    UISwitch* sw = [UISwitch new];
    sw.on = [cfg[@"toggle"] boolValue];
    sw.onTintColor = PadColor(0xFF4CAF50);
    PadBlockSlider* t = [PadBlockSlider new];
    __weak UISwitch* wsw = sw;
    t.change = ^{
      [ws saveButton:keyId values:@{@"toggle" : wsw.on ? @YES : (id)NSNull.null}];
      [ws sync];
    };
    objc_setAssociatedObject(sw, &kEditKey, t, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [sw addTarget:t action:@selector(changed) forControlEvents:UIControlEventValueChanged];
    [rows addObject:PadHStack(@[ PadLabel(PadStr(@"pad_toggle"), 12, 0xFFFFFFFF, NO), [UIView new], sw ], 6)];
  }
  UIButton* del = PadButton([@"🗑  " stringByAppendingString:PadStr(@"delete")], 0xFF291D1D, ^{
    NSMutableDictionary* root = PadReadLayout(ws.dirs);
    [root removeObjectForKey:keyId];
    PadWriteLayout(root);
    [ws removePanel];
    [ws sync];
  });
  [del setTitleColor:PadColor(0xFFFF5252) forState:UIControlStateNormal];
  del.layer.borderColor = PadColor(0xFF5C2C2C).CGColor;
  del.layer.borderWidth = 1;
  [rows addObject:PadFixed(del, 0, 38)];

  UIStackView* st = (UIStackView*)PadVStack(rows, 6);
  UIView* box = [UIView new];
  box.backgroundColor = PadColor(0xFF262626);
  box.layer.cornerRadius = 14;
  box.layer.borderColor = PadColor(0xFF444444).CGColor;
  box.layer.borderWidth = 1;
  st.translatesAutoresizingMaskIntoConstraints = NO;
  [box addSubview:st];
  CGFloat W = 220;
  [NSLayoutConstraint activateConstraints:@[
    [st.topAnchor constraintEqualToAnchor:box.topAnchor constant:12], [st.bottomAnchor constraintEqualToAnchor:box.bottomAnchor constant:-12],
    [st.leadingAnchor constraintEqualToAnchor:box.leadingAnchor constant:12], [st.trailingAnchor constraintEqualToAnchor:box.trailingAnchor constant:-12],
    [st.widthAnchor constraintEqualToConstant:W - 24]
  ]];
  CGSize size = [box systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  CGRect bf = button.frame;
  CGFloat x = CGRectGetMaxX(bf) + 8 + W + 10 < self.area.width ? CGRectGetMaxX(bf) + 8 : bf.origin.x - W - 8;
  CGFloat y = fmax(0, fmin(bf.origin.y - 20, self.area.height - size.height));
  box.frame = CGRectMake(fmax(0, x), y, W, size.height);
  [self.buttons addSubview:box];
  s.panel = box;
  s.panelButton = button;
}

- (void)dismissEditorMenu {
  PadEditState* s = [self es];
  [s.menu removeFromSuperview];
  [s.scrim removeFromSuperview];
  s.menu = s.scrim = nil;
  [UIView animateWithDuration:0.14 animations:^{ s.fab.transform = CGAffineTransformIdentity; }];
}

- (UIView*)menuTile:(NSString*)label icon:(NSString*)icon color:(uint32_t)color action:(void (^)(void))action {
  UIView* t = [UIView new];
  t.backgroundColor = PadColor(0xFF26262A);
  t.layer.cornerRadius = 14;
  t.layer.borderColor = PadColor(0xFF303036).CGColor;
  t.layer.borderWidth = 1;
  UIView* circle = PadFixed([UIView new], 42, 42);
  circle.backgroundColor = PadColor(color);
  circle.layer.cornerRadius = 21;
  UIImageView* iv = [[UIImageView alloc] initWithImage:PadIconTileImage(icon, 22, 0xFFFFFFFF)];
  iv.translatesAutoresizingMaskIntoConstraints = NO;
  [circle addSubview:iv];
  [iv.centerXAnchor constraintEqualToAnchor:circle.centerXAnchor].active = YES;
  [iv.centerYAnchor constraintEqualToAnchor:circle.centerYAnchor].active = YES;
  UILabel* l = PadLabel(label, 12, 0xFFFFFFFF, NO);
  l.textAlignment = NSTextAlignmentCenter;
  UIStackView* st = (UIStackView*)PadVStack(@[ circle, l ], 8);
  st.alignment = UIStackViewAlignmentCenter;
  st.translatesAutoresizingMaskIntoConstraints = NO;
  st.userInteractionEnabled = NO;
  [t addSubview:st];
  [NSLayoutConstraint activateConstraints:@[
    [st.topAnchor constraintEqualToAnchor:t.topAnchor constant:12], [st.bottomAnchor constraintEqualToAnchor:t.bottomAnchor constant:-10],
    [st.leadingAnchor constraintEqualToAnchor:t.leadingAnchor constant:8], [st.trailingAnchor constraintEqualToAnchor:t.trailingAnchor constant:-8]
  ]];
  PadOnTap(t, action);
  return t;
}

- (void)showEditorMenu {
  PadEditState* s = [self es];
  if (s.menu) {
    [self dismissEditorMenu];
    return;
  }
  [self removePanel];
  __weak WPad* ws = self;
  UIButton* scrim = [UIButton buttonWithType:UIButtonTypeCustom];
  scrim.frame = self.buttons.bounds;
  scrim.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  scrim.backgroundColor = PadColor(0x4D000000);
  [scrim addTarget:self action:@selector(dismissEditorMenu) forControlEvents:UIControlEventTouchUpInside];
  [self.buttons addSubview:scrim];

  UIView* t1 = [self menuTile:PadStr(@"pad_editor_add") icon:@"ic_ui_plus" color:0xFF2E7D32 action:^{ [ws dismissEditorMenu]; [ws showCreateButton]; }];
  UIView* t2 = [self menuTile:PadStr(@"pad_editor_style") icon:@"palette" color:0xFF7E57C2 action:^{ [ws dismissEditorMenu]; [ws showStyleWindow]; }];
  UIView* t3 = [self menuTile:PadStr(@"pad_editor_presets") icon:@"ic_ui_folder" color:0xFF1E88E5 action:^{ [ws dismissEditorMenu]; [ws showPresets]; }];
  UIView* t4 = [self menuTile:PadStr(@"pad_editor_done") icon:@"ic_check" color:0xFFE28743 action:^{ [ws dismissEditorMenu]; [ws toggleEditMode]; }];
  UIStackView* r1 = (UIStackView*)PadHStack(@[ t1, t2 ], 6);
  UIStackView* r2 = (UIStackView*)PadHStack(@[ t3, t4 ], 6);
  for (UIStackView* r in @[ r1, r2 ]) {
    r.distribution = UIStackViewDistributionFillEqually;
    r.alignment = UIStackViewAlignmentFill;
  }
  UILabel* title = PadLabel(PadStr(@"pad_editor_title"), 15, 0xFFFFFFFF, YES);
  UILabel* sub = PadLabel(PadStr(@"pad_editor_subtitle"), 11, 0xFF9A9AA2, NO);
  UIStackView* st = (UIStackView*)PadVStack(@[ title, sub, r1, r2 ], 6);
  [st setCustomSpacing:10 afterView:sub];
  UIView* panel = [UIView new];
  panel.backgroundColor = PadColor(0xF21B1B1E);
  panel.layer.cornerRadius = 18;
  panel.layer.borderColor = PadColor(0xFF34343A).CGColor;
  panel.layer.borderWidth = 1;
  st.translatesAutoresizingMaskIntoConstraints = NO;
  [panel addSubview:st];
  CGFloat W = 296;
  [NSLayoutConstraint activateConstraints:@[
    [st.topAnchor constraintEqualToAnchor:panel.topAnchor constant:14], [st.bottomAnchor constraintEqualToAnchor:panel.bottomAnchor constant:-14],
    [st.leadingAnchor constraintEqualToAnchor:panel.leadingAnchor constant:14], [st.trailingAnchor constraintEqualToAnchor:panel.trailingAnchor constant:-14],
    [st.widthAnchor constraintEqualToConstant:W - 28]
  ]];
  CGSize size = [panel systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  UIEdgeInsets safe = self.root.safeAreaInsets;
  CGRect fb = s.fab.frame;
  CGFloat x = fmin(fmax(8, CGRectGetMaxX(fb) - W), self.area.width - safe.right - W - 8);
  CGFloat y = CGRectGetMaxY(fb) + 8;
  if (y + size.height > self.area.height - 8) y = fmax(8, fb.origin.y - size.height - 8);
  panel.frame = CGRectMake(x, y, W, size.height);
  [self.buttons addSubview:panel];
  [self.buttons bringSubviewToFront:s.fab];
  s.menu = panel;
  s.scrim = scrim;
  panel.alpha = 0;
  panel.transform = CGAffineTransformMakeScale(0.92, 0.92);
  [UIView animateWithDuration:0.14 animations:^{
    panel.alpha = 1;
    panel.transform = CGAffineTransformIdentity;
    s.fab.transform = CGAffineTransformMakeRotation(M_PI / 4);
  }];
}

- (void)showCreateButton {
  __weak WPad* ws = self;
  PadSheet* sheet = [PadSheet sheetIn:self.root title:PadStr(@"Create_title") subtitle:nil width:420];
  __block BOOL dpadTab = NO;
  __block NSString *buttonShape = @"circle", *dpadShape = @"dpad", *icon = nil;
  __block int keyCode = 0;
  __block NSString* keyName = nil;
  PadStyle* st = self.style;

  UIButton* tabB = PadButton(PadStr(@"pad_cat_button"), 0xFF333333, nil);
  UIButton* tabD = PadButton(PadStr(@"pad_cat_dpad"), 0xFF333333, nil);
  UIStackView* tabs = (UIStackView*)PadHStack(@[ PadFixed(tabB, 0, 38), PadFixed(tabD, 0, 38) ], 8);
  tabs.distribution = UIStackViewDistributionFillEqually;
  UILabel* shapeLabel = PadLabel(@"", 12, 0xFFB0B0B0, NO);

  NSMutableArray<UIView*>* bTiles = [NSMutableArray new];
  NSMutableArray<UIView*>* dTiles = [NSMutableArray new];
  NSMutableArray<UIImageView*>* bPreviews = [NSMutableArray new];
  UIView* (^tile)(NSString*, BOOL) = ^UIView*(NSString* shape, BOOL dp) {
    UIImageView* iv = [[UIImageView alloc] initWithImage:PadPreview(shape, nil, @"A", st, CGSizeMake(64, 48))];
    iv.contentMode = UIViewContentModeScaleAspectFit;
    if (!dp) [bPreviews addObject:iv];
    UILabel* l = PadLabel(PadShapeTitle(shape), 10, 0xFFFFFFFF, NO);
    l.numberOfLines = 1;
    l.textAlignment = NSTextAlignmentCenter;
    l.adjustsFontSizeToFitWidth = YES;
    UIStackView* v = (UIStackView*)PadVStack(@[ PadFixed(iv, 0, 48), l ], 3);
    v.layoutMargins = UIEdgeInsetsMake(4, 4, 4, 4);
    v.layoutMarginsRelativeArrangement = YES;
    return v;
  };
  for (NSString* sh in PadButtonShapes()) [bTiles addObject:tile(sh, NO)];
  for (NSString* sh in PadDpadShapes()) [dTiles addObject:tile(sh, YES)];
  UIView* (^grid)(NSArray<UIView*>*) = ^UIView*(NSArray<UIView*>* items) {
    NSMutableArray* rows = [NSMutableArray new];
    for (NSUInteger i = 0; i < items.count; i += 4) {
      NSMutableArray* row = [[items subarrayWithRange:NSMakeRange(i, MIN(4, items.count - i))] mutableCopy];
      while (row.count < 4) [row addObject:[UIView new]];
      UIStackView* r = (UIStackView*)PadHStack(row, 6);
      r.distribution = UIStackViewDistributionFillEqually;
      r.alignment = UIStackViewAlignmentFill;
      [rows addObject:r];
    }
    return PadVStack(rows, 6);
  };
  UIView* bGrid = grid(bTiles);
  UIView* dGrid = grid(dTiles);
  UILabel* dHint = PadLabel(PadStr(@"pad_dpad_hint"), 12, 0xFFFFFFFF, NO);

  UIButton* keyBtn = [self keyButton:0 onPick:^(int code, NSString* name) { keyCode = code; keyName = name; }];
  [keyBtn setTitle:@"LMB" forState:UIControlStateNormal];
  keyCode = 325;
  keyName = @"LMB";
  PadIconPicker* picker = [PadIconPicker pickerWithTile:36 selected:nil onSelect:^(NSString* name) {
    icon = name;
    for (NSUInteger i = 0; i < bPreviews.count; i++) bPreviews[i].image = PadPreview(PadButtonShapes()[i], icon, @"A", st, CGSizeMake(64, 48));
  }];
  UISwitch* toggle = [UISwitch new];
  toggle.onTintColor = PadColor(0xFF4CAF50);
  UIStackView* toggleTexts = (UIStackView*)PadVStack(@[ PadLabel(PadStr(@"pad_toggle"), 13, 0xFFFFFFFF, NO),
                                                        PadLabel(PadStr(@"pad_toggle_hint"), 11, 0xFF9A9A9A, NO) ], 1);
  UIView* options = PadVStack(@[ PadLabel(PadStr(@"pad_key"), 12, 0xFFB0B0B0, NO), PadFixed(keyBtn, 0, 40),
                                 PadLabel(PadStr(@"pad_icon"), 12, 0xFFB0B0B0, NO), PadFixed(picker, 0, 36),
                                 PadHStack(@[ toggleTexts, toggle ], 8) ], 6);

  void (^refresh)(void) = ^{
    tabB.backgroundColor = PadColor(dpadTab ? 0xFF333333 : 0xFFE28743);
    tabD.backgroundColor = PadColor(dpadTab ? 0xFFE28743 : 0xFF333333);
    [tabB setTitleColor:dpadTab ? UIColor.grayColor : UIColor.whiteColor forState:UIControlStateNormal];
    [tabD setTitleColor:dpadTab ? UIColor.whiteColor : UIColor.grayColor forState:UIControlStateNormal];
    tabB.layer.borderWidth = dpadTab ? 0 : 2;
    tabD.layer.borderWidth = dpadTab ? 2 : 0;
    tabB.layer.borderColor = tabD.layer.borderColor = UIColor.whiteColor.CGColor;
    bGrid.hidden = dpadTab;
    dGrid.hidden = !dpadTab;
    dHint.hidden = !dpadTab;
    options.hidden = dpadTab;
    shapeLabel.text = dpadTab ? PadStr(@"pad_dpad_kind") : PadStr(@"Create_Form");
    for (NSUInteger i = 0; i < bTiles.count; i++) PadCard(bTiles[i], [PadButtonShapes()[i] isEqualToString:buttonShape]);
    for (NSUInteger i = 0; i < dTiles.count; i++) PadCard(dTiles[i], [PadDpadShapes()[i] isEqualToString:dpadShape]);
  };
  for (NSUInteger i = 0; i < bTiles.count; i++) {
    NSString* sh = PadButtonShapes()[i];
    PadOnTap(bTiles[i], ^{ buttonShape = sh; refresh(); });
  }
  for (NSUInteger i = 0; i < dTiles.count; i++) {
    NSString* sh = PadDpadShapes()[i];
    PadOnTap(dTiles[i], ^{ dpadShape = sh; refresh(); });
  }
  PadOnTap(tabB, ^{ dpadTab = NO; refresh(); });
  PadOnTap(tabD, ^{ dpadTab = YES; refresh(); });

  __weak PadSheet* wsheet = sheet;
  UIButton* cancel = PadButton(PadStr(@"cancel").uppercaseString, 0xFF555555, ^{ [wsheet close]; });
  UIButton* create = PadButton(PadStr(@"Create").uppercaseString, 0xFF2E7D32, ^{
    NSMutableDictionary* cfg = [@{@"shape" : dpadTab ? dpadShape : buttonShape, @"alpha" : @1.0, @"scale" : @1.0,
                                  @"pctX" : @0.45, @"pctY" : @0.45, @"isLeft" : @YES, @"isTop" : @YES} mutableCopy];
    if (dpadTab) cfg[@"keyName"] = @"";
    else {
      cfg[@"keyName"] = keyName.uppercaseString ?: @"UNK";
      cfg[@"keyCode"] = @(keyCode);
      if (icon) cfg[@"icon"] = icon;
      if (toggle.on) cfg[@"toggle"] = @YES;
    }
    NSMutableDictionary* root = PadReadLayout(ws.dirs);
    root[NSUUID.UUID.UUIDString.lowercaseString] = cfg;
    PadWriteLayout(root);
    [wsheet close];
    [ws sync];
  });
  UIStackView* bottom = (UIStackView*)PadHStack(@[ PadFixed(cancel, 0, 44), PadFixed(create, 0, 44) ], 10);
  bottom.distribution = UIStackViewDistributionFillEqually;
  for (UIView* v in @[ tabs, shapeLabel, bGrid, dGrid, dHint, options, bottom ]) [sheet.content addArrangedSubview:v];
  [sheet.content setCustomSpacing:14 afterView:options];
  refresh();
}
@end

@interface PadIconPicker () <PHPickerViewControllerDelegate>
@end

@implementation PadIconPicker {
  CGFloat _tile;
  NSString* _selected;
  void (^_cb)(NSString*);
  NSMutableArray<UIView*>* _tiles;
  NSMutableArray* _names;
}

+ (PadIconPicker*)pickerWithTile:(CGFloat)tile selected:(NSString*)sel onSelect:(void (^)(NSString*))cb {
  PadIconPicker* p = [PadIconPicker new];
  p->_tile = tile;
  p->_selected = sel;
  p->_cb = cb;
  p.showsHorizontalScrollIndicator = NO;
  [p rebuild];
  return p;
}

- (void)rebuild {
  for (UIView* v in self.subviews) [v removeFromSuperview];
  _tiles = [NSMutableArray new];
  _names = [NSMutableArray arrayWithObject:NSNull.null];
  [_names addObjectsFromArray:PadIconList()];
  CGFloat x = 0, T = _tile;
  __weak PadIconPicker* wp = self;
  for (id n in _names) {
    UIView* t = [[UIView alloc] initWithFrame:CGRectMake(x, 0, T, T)];
    if (n == NSNull.null) {
      UILabel* l = PadLabel(@"Aa", 13, 0xFFFFFFFF, YES);
      l.textAlignment = NSTextAlignmentCenter;
      l.frame = t.bounds;
      [t addSubview:l];
    } else {
      CGFloat in = MAX(4, T / 5);
      UIImageView* iv = [[UIImageView alloc] initWithImage:PadIconTileImage(n, T - 2 * in, 0xFFFFFFFF)];
      iv.frame = CGRectInset(t.bounds, in, in);
      [t addSubview:iv];
      if (PadIconIsUser(n)) {
        UILongPressGestureRecognizer* lp = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longTile:)];
        [t addGestureRecognizer:lp];
      }
    }
    id name = n;
    PadOnTap(t, ^{ [wp select:name == NSNull.null ? nil : name]; });
    [self addSubview:t];
    [_tiles addObject:t];
    x += T + 4;
  }
  UIView* plus = [[UIView alloc] initWithFrame:CGRectMake(x, 0, T, T)];
  UILabel* pl = PadLabel(@"+", 20, 0xFF69F0AE, YES);
  pl.textAlignment = NSTextAlignmentCenter;
  pl.frame = plus.bounds;
  [plus addSubview:pl];
  PadCard(plus, NO);
  PadOnTap(plus, ^{ [wp pickImage]; });
  [self addSubview:plus];
  x += T;
  self.contentSize = CGSizeMake(x, T);
  [self refresh];
  NSUInteger i = _selected ? [_names indexOfObject:_selected] : NSNotFound;
  if (i != NSNotFound && i > 0)
    dispatch_async(dispatch_get_main_queue(), ^{
      PadIconPicker* sp = wp;
      if (!sp || i >= sp->_tiles.count) return;
      UIView* t = sp->_tiles[i];
      CGFloat ox = fmax(0, fmin(t.frame.origin.x - (sp.bounds.size.width - t.frame.size.width) / 2,
                                sp.contentSize.width - sp.bounds.size.width));
      [sp setContentOffset:CGPointMake(ox, 0) animated:NO];
    });
}

- (void)refresh {
  for (NSUInteger i = 0; i < _tiles.count; i++) {
    id n = _names[i];
    PadCard(_tiles[i], n == NSNull.null ? _selected == nil : [n isEqual:_selected]);
  }
}

- (void)select:(NSString*)name {
  _selected = name;
  [self refresh];
  if (_cb) _cb(name);
}

- (void)longTile:(UILongPressGestureRecognizer*)g {
  if (g.state != UIGestureRecognizerStateBegan) return;
  NSUInteger i = [_tiles indexOfObject:g.view];
  if (i == NSNotFound) return;
  NSString* name = _names[i];
  __weak PadIconPicker* wp = self;
  PadConfirm(self, PadStr(@"pad_icon_delete_confirm"), PadStr(@"delete"), ^{
    PadIconPicker* sp = wp;
    PadIconDelete(name);
    if (!sp) return;
    BOOL was = [name isEqual:sp->_selected];
    [sp rebuild];
    if (was) [sp select:nil];
  });
}

- (void)pickImage {
  PHPickerConfiguration* c = [PHPickerConfiguration new];
  c.filter = PHPickerFilter.imagesFilter;
  c.selectionLimit = 1;
  PHPickerViewController* vc = [[PHPickerViewController alloc] initWithConfiguration:c];
  vc.delegate = self;
  [PadTopVC(self) presentViewController:vc animated:YES completion:nil];
}

- (void)picker:(PHPickerViewController*)picker didFinishPicking:(NSArray<PHPickerResult*>*)results {
  [picker dismissViewControllerAnimated:YES completion:nil];
  NSItemProvider* ip = results.firstObject.itemProvider;
  if (![ip canLoadObjectOfClass:UIImage.class]) return;
  __weak PadIconPicker* wp = self;
  [ip loadObjectOfClass:UIImage.class completionHandler:^(id<NSItemProviderReading> obj, NSError* err) {
    dispatch_async(dispatch_get_main_queue(), ^{
      PadIconPicker* p = wp;
      if (!p) return;
      NSString* name = [obj isKindOfClass:UIImage.class] ? PadIconImport((UIImage*)obj) : nil;
      UIView* host = p.window ?: p;
      if (!name) {
        PadToast(host, PadStr(@"pad_icon_bad"));
        return;
      }
      [p rebuild];
      [p select:name];
      PadToast(host, PadStr(@"pad_icon_added"));
    });
  }];
}
@end
