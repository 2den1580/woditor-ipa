#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <mach/mach.h>
#import <objc/runtime.h>
#import <os/proc.h>

#import "PadInternal.h"

static char kWinKey;

@interface PadSliderTarget : NSObject
@property(nonatomic, copy) void (^block)(void);
- (void)fire;
@end
@implementation PadSliderTarget
- (void)fire {
  if (_block) _block();
}
@end

@interface PadDocDelegate : NSObject <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^picked)(NSURL* url);
@end
@implementation PadDocDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)c didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  if (urls.firstObject && _picked) _picked(urls.firstObject);
}
@end

static UIColor* PadCrosshairColor(NSString* hex) {
  NSString* h = [hex hasPrefix:@"#"] ? [hex substringFromIndex:1] : hex;
  unsigned long long v = 0;
  [[NSScanner scannerWithString:h] scanHexLongLong:&v];
  if (h.length == 6) v |= 0xFF000000ull;
  else if (h.length != 8) v = 0xFF00FF00ull;
  return PadColor((uint32_t)v);
}

@interface PadCrosshairPreview : UIView
@property(nonatomic, copy) NSString* color;
@property(nonatomic) int size, thick;
@property(nonatomic) float opacity;
@property(nonatomic) BOOL on;
@end
@implementation PadCrosshairPreview
- (void)drawRect:(CGRect)r {
  CGContextRef c = UIGraphicsGetCurrentContext();
  [PadColor(0xFF121214) setFill];
  UIRectFill(self.bounds);
  [PadColor(0xFF00FFFF) setStroke];
  CGContextSetLineWidth(c, 2);
  CGContextStrokeRect(c, CGRectInset(self.bounds, 1, 1));
  UIColor* col = [PadCrosshairColor(_color ?: @"#00FF00") colorWithAlphaComponent:_opacity * (_on ? 1 : 0.3)];
  [col setStroke];
  CGContextSetLineWidth(c, _thick);
  CGFloat cx = self.bounds.size.width / 2, cy = self.bounds.size.height / 2, hs = _size / 2.0;
  CGContextMoveToPoint(c, cx - hs, cy);
  CGContextAddLineToPoint(c, cx + hs, cy);
  CGContextMoveToPoint(c, cx, cy - hs);
  CGContextAddLineToPoint(c, cx, cy + hs);
  CGContextStrokePath(c);
}
@end

static NSString* g_statsTitle;
static void PadStatsTitle(NSString* t) { g_statsTitle = t; }

@interface PadFpsGraph : UIView
- (void)add:(int)r screen:(int)s;
@end
@implementation PadFpsGraph {
  NSMutableArray<NSNumber*>*_r, *_s;
}
- (instancetype)initWithFrame:(CGRect)f {
  if ((self = [super initWithFrame:f])) {
    _r = [NSMutableArray new];
    _s = [NSMutableArray new];
    self.backgroundColor = PadColor(0x33000000);
    self.layer.cornerRadius = 6;
  }
  return self;
}
- (void)add:(int)r screen:(int)s {
  [_r addObject:@(r)];
  [_s addObject:@(s)];
  while (_r.count > 60) {
    [_r removeObjectAtIndex:0];
    [_s removeObjectAtIndex:0];
  }
  [self setNeedsDisplay];
}
- (void)drawRect:(CGRect)rect {
  CGContextRef c = UIGraphicsGetCurrentContext();
  CGFloat w = self.bounds.size.width, h = self.bounds.size.height;
  [PadColor(0x33FFFFFF) setStroke];
  CGContextSetLineWidth(c, 1);
  for (int f = 30; f <= 60; f += 30) {
    CGFloat y = h - h * f / 70.0;
    CGContextMoveToPoint(c, 0, y);
    CGContextAddLineToPoint(c, w, y);
  }
  CGContextStrokePath(c);
  NSArray* sets = @[ _s, _r ];
  NSArray* cols = @[ UIColor.redColor, UIColor.greenColor ];
  for (int k = 0; k < 2; k++) {
    NSArray<NSNumber*>* d = sets[k];
    if (d.count < 2) continue;
    [cols[k] setStroke];
    CGContextSetLineWidth(c, 1.5);
    for (NSUInteger i = 0; i < d.count; i++) {
      CGFloat x = w * i / 59.0, y = h - h * fmin(70, d[i].doubleValue) / 70.0;
      if (i == 0) CGContextMoveToPoint(c, x, y);
      else CGContextAddLineToPoint(c, x, y);
    }
    CGContextStrokePath(c);
  }
}
@end

@interface PadStatsView : UIView
- (instancetype)initWithArea:(UIView*)area;
- (void)fps:(int)r screen:(int)s;
- (void)title;
- (void)restorePosition;
@end

@implementation PadStatsView {
  __weak UIView* _area;
  UILabel *_title, *_mem, *_ren, *_scr, *_mini, *_miniMem;
  PadFpsGraph* _graph;
  UIView *_full, *_compact;
  BOOL _minimized;
  CGPoint _start;
}

static UILabel* circleValue(uint32_t stroke) {
  UILabel* l = PadLabel(@"0", 14, 0xFFFFFFFF, YES);
  l.textAlignment = NSTextAlignmentCenter;
  l.layer.cornerRadius = 20;
  l.layer.borderWidth = 2;
  l.layer.borderColor = PadColor(stroke).CGColor;
  return (UILabel*)PadFixed(l, 40, 40);
}

- (instancetype)initWithArea:(UIView*)area {
  if ((self = [super initWithFrame:CGRectZero])) {
    _area = area;
    self.backgroundColor = PadColor(0xCC000000);
    self.layer.cornerRadius = 12;
    _title = PadLabel(g_statsTitle ?: PadStr(@"pad_stats_waiting"), 13, 0xFFFFFFFF, NO);
    _title.numberOfLines = 1;
    _title.adjustsFontSizeToFitWidth = YES;
    _title.minimumScaleFactor = 0.6;
    _graph = [PadFpsGraph new];
    _mem = PadLabel(@"mem: 0", 11, 0xFFD3D3D3, NO);
    UILabel* hint = PadLabel(PadStr(@"pad_stats_hint"), 11, 0xFF888888, NO);
    UIView* graphCol = PadVStack(@[ PadFixed(_graph, 150, 100), _mem, hint ], 3);
    PadFixed(graphCol, 150, 0);
    _ren = circleValue(0xFF00FF00);
    _scr = circleValue(0xFFFF0000);
    UIStackView* info = (UIStackView*)PadVStack(@[ PadLabel(@"FPS", 14, 0xFFFFFFFF, YES), PadLabel(@"Render", 12, 0xFF00FF00, NO), _ren,
                                                  PadLabel(@"Screen", 12, 0xFFFF0000, NO), _scr ], 3);
    info.alignment = UIStackViewAlignmentCenter;
    info.layoutMargins = UIEdgeInsetsMake(4, 6, 6, 6);
    info.layoutMarginsRelativeArrangement = YES;
    info.backgroundColor = PadColor(0x1AFFFFFF);
    info.layer.cornerRadius = 8;
    info.layer.borderWidth = 1;
    info.layer.borderColor = PadColor(0x44FFFFFF).CGColor;
    [info.widthAnchor constraintGreaterThanOrEqualToConstant:68].active = YES;
    UIStackView* row = (UIStackView*)PadHStack(@[ graphCol, info ], 6);
    row.alignment = UIStackViewAlignmentTop;
    _full = PadVStack(@[ PadFixed(_title, 215, 24), row ], 5);
    _mini = PadLabel(@"Ren: 0  Scr: 0", 13, 0xFFFFFFFF, NO);
    _miniMem = PadLabel(@"mem: 0", 12, 0xFFD3D3D3, NO);
    _compact = PadVStack(@[ _mini, _miniMem ], 2);
    UIStackView* all = (UIStackView*)PadVStack(@[ _full, _compact ], 0);
    all.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:all];
    [NSLayoutConstraint activateConstraints:@[
      [all.topAnchor constraintEqualToAnchor:self.topAnchor constant:8], [all.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-8],
      [all.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:8], [all.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-8]
    ]];
    _minimized = [NSUserDefaults.standardUserDefaults boolForKey:@"floating_stats.stats_minimized"];
    [self applyMode];
    [self addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(pan:)]];
    UITapGestureRecognizer* dt = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMode)];
    dt.numberOfTapsRequired = 2;
    [self addGestureRecognizer:dt];
    [self memTick];
  }
  return self;
}

- (void)applyMode {
  _full.hidden = _minimized;
  _compact.hidden = !_minimized;
  CGSize s = [self systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  CGRect f = self.frame;
  f.size = s;
  self.frame = f;
  [self restorePosition];
}

- (void)toggleMode {
  _minimized = !_minimized;
  [NSUserDefaults.standardUserDefaults setBool:_minimized forKey:@"floating_stats.stats_minimized"];
  [self applyMode];
}

- (void)restorePosition {
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  CGSize a = _area.bounds.size, s = self.frame.size;
  CGFloat fw = fmax(0, a.width - s.width), fh = fmax(0, a.height - s.height);
  CGFloat x = [d objectForKey:@"floating_stats.stats_px"] ? [d doubleForKey:@"floating_stats.stats_px"] * fw : fw - 8 - _area.safeAreaInsets.right;
  CGFloat y = [d objectForKey:@"floating_stats.stats_py"] ? [d doubleForKey:@"floating_stats.stats_py"] * fh : 8 + _area.safeAreaInsets.top;
  self.frame = CGRectMake(fmax(0, fmin(x, fw)), fmax(0, fmin(y, fh)), s.width, s.height);
}

- (void)pan:(UIPanGestureRecognizer*)g {
  if (g.state == UIGestureRecognizerStateBegan) _start = self.frame.origin;
  CGPoint t = [g translationInView:_area];
  CGSize a = _area.bounds.size, s = self.frame.size;
  CGRect f = self.frame;
  f.origin.x = fmax(0, fmin(_start.x + t.x, a.width - s.width));
  f.origin.y = fmax(0, fmin(_start.y + t.y, a.height - s.height));
  self.frame = f;
  if (g.state == UIGestureRecognizerStateEnded) {
    NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
    [d setDouble:fmin(1, f.origin.x / fmax(1, a.width - s.width)) forKey:@"floating_stats.stats_px"];
    [d setDouble:fmin(1, f.origin.y / fmax(1, a.height - s.height)) forKey:@"floating_stats.stats_py"];
  }
}

- (void)fps:(int)r screen:(int)s {
  _ren.text = @(r).stringValue;
  _scr.text = @(s).stringValue;
  NSMutableAttributedString* m = [NSMutableAttributedString new];
  [m appendAttributedString:[[NSAttributedString alloc] initWithString:@"Ren: " attributes:@{NSForegroundColorAttributeName : UIColor.whiteColor}]];
  [m appendAttributedString:[[NSAttributedString alloc] initWithString:@(r).stringValue attributes:@{NSForegroundColorAttributeName : UIColor.greenColor}]];
  [m appendAttributedString:[[NSAttributedString alloc] initWithString:@"  Scr: " attributes:@{NSForegroundColorAttributeName : UIColor.whiteColor}]];
  [m appendAttributedString:[[NSAttributedString alloc] initWithString:@(s).stringValue attributes:@{NSForegroundColorAttributeName : UIColor.redColor}]];
  _mini.attributedText = m;
  [_graph add:r screen:s];
  [self memTick];
}

- (void)title { _title.text = g_statsTitle ?: PadStr(@"pad_stats_waiting"); }

- (void)memTick {
  task_vm_info_data_t vi;
  mach_msg_type_number_t n = TASK_VM_INFO_COUNT;
  unsigned long long used = 0;
  if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &n) == KERN_SUCCESS) used = vi.phys_footprint >> 20;
  unsigned long long avail = 0;
  if (@available(iOS 13.0, *)) avail = os_proc_available_memory() >> 20;
  unsigned long long total = NSProcessInfo.processInfo.physicalMemory >> 20;
  _mem.text = [NSString stringWithFormat:@"mem: %llu MB\nfree: %llu\\%llu", used, avail, total];
  _miniMem.text = [NSString stringWithFormat:@"mem: %llu free: %llu", used, avail];
  uint32_t col = avail > 0 && avail < 200 ? 0xFFFF0000 : 0xFFD3D3D3;
  _mem.textColor = _miniMem.textColor = PadColor(col);
}
@end

static UIImage* stylePreview(PadStyle* st) {
  CGSize sz = CGSizeMake(80, 56);
  UIGraphicsImageRenderer* r = [[UIGraphicsImageRenderer alloc] initWithSize:sz];
  return [r imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
    [[UIBezierPath bezierPathWithRoundedRect:CGRectMake(0, 0, sz.width, sz.height) cornerRadius:8] addClip];
    [PadColor(0xFF5B8F3D) setFill];
    UIRectFill(CGRectMake(0, 0, sz.width / 2, sz.height));
    [PadColor(0xFF6FA0D8) setFill];
    UIRectFill(CGRectMake(0, 0, sz.width / 2, sz.height * 0.35));
    [UIColor.blackColor setFill];
    UIRectFill(CGRectMake(sz.width / 2, 0, sz.width / 2, sz.height));
    CGFloat btn = 40, pad = PadRenderPad(st);
    UIImage* b = PadRender([PadShape backgroundFor:@"circle"], nil, 0.9, @"A", CGSizeMake(btn, btn), UIScreen.mainScreen.scale, st, NO);
    [b drawInRect:CGRectMake((sz.width - btn) / 2 - pad, (sz.height - btn) / 2 - pad, btn + 2 * pad, btn + 2 * pad)];
  }];
}

@implementation WPad (Windows)

- (void)showStyleWindow {
  PadSheet* sheet = [PadSheet sheetIn:self.root title:PadStr(@"pad_style_title") subtitle:nil width:420];
  NSArray* types = @[ @(PAD_DARK), @(PAD_GLASS), @(PAD_LIGHT), @(PAD_NEON) ];
  NSArray* names = @[ @"pad_style_dark", @"pad_style_glass", @"pad_style_light", @"pad_style_neon" ];
  NSMutableArray<UIView*>* cards = [NSMutableArray new];
  NSMutableArray<UIImageView*>* previews = [NSMutableArray new];
  NSMutableArray<UIView*>* dots = [NSMutableArray new];
  UILabel* colorLabel = PadLabel(PadStr(@"pad_style_neon_color"), 12, 0xFFD3D3D3, NO);
  __weak WPad* ws = self;
  __block void (^refresh)(void);
  for (NSUInteger i = 0; i < types.count; i++) {
    UIImageView* iv = [UIImageView new];
    iv.contentMode = UIViewContentModeScaleAspectFit;
    UILabel* l = PadLabel(PadStr(names[i]), 12, 0xFFFFFFFF, NO);
    l.textAlignment = NSTextAlignmentCenter;
    UIStackView* c = (UIStackView*)PadVStack(@[ PadFixed(iv, 0, 56), l ], 6);
    c.layoutMargins = UIEdgeInsetsMake(6, 6, 6, 6);
    c.layoutMarginsRelativeArrangement = YES;
    int type = [types[i] intValue];
    PadOnTap(c, ^{
      PadStyle* ns = [PadStyle styleWithType:type neon:ws.style.neonColor];
      if (![ns isEqualToStyle:ws.style]) [ws setStyleAndSave:ns];
      refresh();
    });
    [cards addObject:c];
    [previews addObject:iv];
  }
  UIStackView* row = (UIStackView*)PadHStack(cards, 6);
  row.distribution = UIStackViewDistributionFillEqually;
  row.alignment = UIStackViewAlignmentFill;
  NSMutableArray* dotViews = [NSMutableArray new];
  for (int i = 0; i < 8; i++) {
    UIView* d = PadFixed([UIView new], 28, 28);
    d.layer.cornerRadius = 14;
    d.backgroundColor = PadColor(kPadNeonColors[i]);
    uint32_t color = kPadNeonColors[i];
    PadOnTap(d, ^{
      PadStyle* ns = [PadStyle styleWithType:PAD_NEON neon:color];
      if (![ns isEqualToStyle:ws.style]) [ws setStyleAndSave:ns];
      refresh();
    });
    [dots addObject:d];
    [dotViews addObject:d];
  }
  UIStackView* dotRow = (UIStackView*)PadHStack(dotViews, 8);
  UIView* dotWrap = PadHStack(@[ [UIView new], dotRow, [UIView new] ], 0);
  ((UIStackView*)dotWrap).distribution = UIStackViewDistributionEqualCentering;
  refresh = ^{
    PadStyle* cur = ws.style;
    for (NSUInteger i = 0; i < types.count; i++) {
      BOOL sel = cur.type == [types[i] intValue];
      UIView* c = cards[i];
      c.backgroundColor = PadColor(sel ? 0xFF2A2D30 : 0xFF232326);
      c.layer.cornerRadius = 10;
      c.layer.borderWidth = sel ? 2 : 1;
      c.layer.borderColor = PadColor(sel ? 0xFF4CAF50 : 0xFF3A3A3E).CGColor;
      previews[i].image = stylePreview([PadStyle styleWithType:[types[i] intValue] neon:cur.neonColor]);
    }
    BOOL neon = cur.type == PAD_NEON;
    for (int i = 0; i < 8; i++) {
      BOOL sel = neon && cur.neonColor == kPadNeonColors[i];
      dots[i].layer.borderWidth = sel ? 3 : 1;
      dots[i].layer.borderColor = (sel ? UIColor.whiteColor : PadColor(0xFF555555)).CGColor;
    }
    colorLabel.hidden = !neon;
    dotWrap.hidden = !neon;
  };
  for (UIView* v in @[ row, colorLabel, dotWrap ]) [sheet.content addArrangedSubview:v];
  [sheet.content setCustomSpacing:14 afterView:row];
  refresh();
}

static NSString* presetDir(void) {
  NSString* s = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  NSString* d = [s stringByAppendingPathComponent:@"pad_presets"];
  [NSFileManager.defaultManager createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil error:nil];
  return d;
}

static NSString* presetPath(NSString* name) {
  return [presetDir() stringByAppendingPathComponent:[name stringByAppendingString:@".torypriset"]];
}

static NSString* cleanName(NSString* n) {
  NSMutableString* o = [NSMutableString new];
  for (NSUInteger i = 0; i < n.length && o.length < 40; i++) {
    unichar c = [n characterAtIndex:i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') [o appendFormat:@"%C", c];
    else if (c == ' ') [o appendString:@"_"];
  }
  return o;
}

static NSDictionary* readPreset(NSData* data, NSString* file) {
  NSDictionary* files = PadUnzip(data);
  NSData* pj = files[@"preset.json"];
  NSDictionary* root = pj ? [NSJSONSerialization JSONObjectWithData:pj options:0 error:nil] : nil;
  if (![root isKindOfClass:NSDictionary.class] || ![root[@"layout"] isKindOfClass:NSDictionary.class]) return nil;
  NSMutableDictionary* icons = [NSMutableDictionary new];
  for (NSString* k in files)
    if ([k hasPrefix:@"icons/"] && [k hasSuffix:@".png"]) icons[[k substringWithRange:NSMakeRange(6, k.length - 10)]] = files[k];
  NSMutableDictionary* p = [@{@"layout" : root[@"layout"], @"icons" : icons} mutableCopy];
  NSDictionary* st = root[@"style"];
  if ([st isKindOfClass:NSDictionary.class])
    p[@"style"] = [PadStyle styleWithType:[st[@"type"] ?: @(PAD_GLASS) intValue] neon:(uint32_t)[st[@"neonColor"] ?: @(kPadNeonColors[0]) longLongValue]];
  p[@"name"] = file ? file.lastPathComponent.stringByDeletingPathExtension : ([root[@"name"] isKindOfClass:NSString.class] ? root[@"name"] : @"");
  if (file) p[@"file"] = file;
  return p;
}

static NSData* buildPreset(NSString* name, NSDictionary* layout, PadStyle* st) {
  NSMutableDictionary* root = [@{@"format" : @1, @"name" : name, @"created" : @((long long)(NSDate.date.timeIntervalSince1970 * 1000)),
                                 @"layout" : layout} mutableCopy];
  if (st) root[@"style"] = @{@"type" : @(st.type), @"neonColor" : @((int32_t)st.neonColor)};
  NSMutableDictionary* files = [NSMutableDictionary new];
  files[@"preset.json"] = [NSJSONSerialization dataWithJSONObject:root options:NSJSONWritingPrettyPrinted error:nil];
  for (NSDictionary* cfg in layout.allValues) {
    NSString* icon = [cfg isKindOfClass:NSDictionary.class] && [cfg[@"icon"] isKindOfClass:NSString.class] ? cfg[@"icon"] : nil;
    if (!icon.length) continue;
    NSData* png = PadIconExportPng(icon);
    if (png) files[[NSString stringWithFormat:@"icons/%@.png", icon]] = png;
  }
  return PadZip(files);
}

static NSArray<NSDictionary*>* listPresets(void) {
  NSMutableArray* out = [NSMutableArray new];
  NSFileManager* fm = NSFileManager.defaultManager;
  NSMutableArray* files = [NSMutableArray new];
  for (NSString* f in [fm contentsOfDirectoryAtPath:presetDir() error:nil])
    if ([f hasSuffix:@".torypriset"]) [files addObject:[presetDir() stringByAppendingPathComponent:f]];
  [files sortUsingComparator:^NSComparisonResult(NSString* a, NSString* b) {
    NSDate* da = [fm attributesOfItemAtPath:a error:nil].fileModificationDate;
    NSDate* db = [fm attributesOfItemAtPath:b error:nil].fileModificationDate;
    return [db compare:da];
  }];
  for (NSString* f in files) {
    NSDictionary* p = readPreset([NSData dataWithContentsOfFile:f], f);
    if (p) [out addObject:p];
  }
  return out;
}

static UIImage* presetThumb(NSDictionary* layout, PadStyle* st, CGSize sz, CGSize screen) {
  UIGraphicsImageRenderer* r = [[UIGraphicsImageRenderer alloc] initWithSize:sz];
  return [r imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
    [[UIBezierPath bezierPathWithRoundedRect:CGRectMake(0, 0, sz.width, sz.height) cornerRadius:8] addClip];
    [PadColor(0xFF0B0B0D) setFill];
    UIRectFill(CGRectMake(0, 0, sz.width, sz.height));
    CGFloat gw = sz.height * 4 / 3, gx = (sz.width - gw) / 2;
    [PadColor(0xFF4F7FB8) setFill];
    UIRectFill(CGRectMake(gx, 0, gw, sz.height * 0.42));
    [PadColor(0xFF4C7A35) setFill];
    UIRectFill(CGRectMake(gx, sz.height * 0.42, gw, sz.height * 0.58));
    CGFloat k = sz.width / fmax(screen.width, screen.height);
    for (NSDictionary* cfg in layout.allValues) {
      if (![cfg isKindOfClass:NSDictionary.class]) continue;
      NSString* shape = [cfg[@"shape"] isKindOfClass:NSString.class] ? cfg[@"shape"] : @"circle";
      CGSize s0 = PadShapeSize(shape);
      CGFloat scale = cfg[@"scale"] ? [cfg[@"scale"] doubleValue] : 1;
      CGFloat bw = fmax(4, s0.width * k), bh = fmax(4, s0.height * k);
      CGFloat mx = fmax(0, fmin([cfg[@"pctX"] doubleValue] * sz.width, sz.width - bw));
      CGFloat my = fmax(0, fmin([cfg[@"pctY"] doubleValue] * sz.height, sz.height - bh));
      BOOL left = cfg[@"isLeft"] ? [cfg[@"isLeft"] boolValue] : YES, top = cfg[@"isTop"] ? [cfg[@"isTop"] boolValue] : YES;
      CGFloat x = left ? mx : sz.width - mx - bw, y = top ? my : sz.height - my - bh;
      NSString* icon = [cfg[@"icon"] isKindOfClass:NSString.class] ? cfg[@"icon"] : nil;
      NSString* text = [cfg[@"keyName"] isKindOfClass:NSString.class] ? cfg[@"keyName"] : @"";
      CGSize box = CGSizeMake(fmax(4, bw * scale), fmax(4, bh * scale));
      UIImage* b = PadPreview(shape, icon.length ? icon : nil, text, st, box);
      CGFloat a = fmax(0.1, fmin(1, cfg[@"alpha"] ? [cfg[@"alpha"] doubleValue] : 1));
      [b drawInRect:CGRectMake(x + bw / 2 - box.width / 2, y + bh / 2 - box.height / 2, box.width, box.height)
          blendMode:kCGBlendModeNormal alpha:a];
    }
  }];
}

- (void)showPresets {
  PadSheet* sheet = [PadSheet sheetIn:self.root title:PadStr(@"pad_presets_title") subtitle:PadStr(@"pad_presets_subtitle")
                                width:self.area.width >= 700 ? 720 : 400];
  objc_setAssociatedObject(self, &kWinKey, sheet, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  __weak WPad* ws = self;
  __weak PadSheet* wsheet = sheet;
  UIButton* save = PadButton(PadStr(@"pad_preset_save_current"), 0xFF2E7D32, ^{ [ws presetAskName:wsheet]; });
  UIButton* imp = PadButton(PadStr(@"pad_preset_import"), 0xFF1E5FAF, ^{ [ws presetImport:wsheet]; });
  UIStackView* actions = (UIStackView*)PadHStack(@[ PadFixed(save, 0, 42), PadFixed(imp, 0, 42) ], 10);
  actions.distribution = UIStackViewDistributionFillEqually;
  [sheet.content addArrangedSubview:actions];
  UIStackView* cards = (UIStackView*)PadVStack(@[], 6);
  cards.tag = 4242;
  [sheet.content addArrangedSubview:cards];
  [self presetRebuild:sheet];
}

- (void)presetRebuild:(PadSheet*)sheet {
  UIStackView* cards = [sheet viewWithTag:4242];
  for (UIView* v in cards.arrangedSubviews) [v removeFromSuperview];
  NSMutableArray* list = [NSMutableArray arrayWithObject:@{@"layout" : PadGameDefaultLayout(self.dirs), @"name" : @"Standard", @"icons" : @{}}];
  [list addObjectsFromArray:listPresets()];
  NSInteger cols = self.area.width >= 700 ? 3 : 2;
  NSMutableArray* views = [NSMutableArray new];
  for (NSDictionary* p in list) [views addObject:[self presetCard:p sheet:sheet]];
  for (NSUInteger i = 0; i < views.count; i += cols) {
    NSMutableArray* row = [[views subarrayWithRange:NSMakeRange(i, MIN(cols, views.count - i))] mutableCopy];
    while ((NSInteger)row.count < cols) [row addObject:[UIView new]];
    UIStackView* r = (UIStackView*)PadHStack(row, 6);
    r.distribution = UIStackViewDistributionFillEqually;
    r.alignment = UIStackViewAlignmentTop;
    [cards addArrangedSubview:r];
  }
  if (list.count == 1) {
    UILabel* e = PadLabel(PadStr(@"pad_preset_empty"), 12, 0xFFB0B0B0, NO);
    e.textAlignment = NSTextAlignmentCenter;
    [cards addArrangedSubview:e];
  }
}

- (UIView*)smallIcon:(NSString*)icon color:(uint32_t)color action:(void (^)(void))action {
  UIImageView* iv = [[UIImageView alloc] initWithImage:PadIconTileImage(icon, 18, color)];
  iv.contentMode = UIViewContentModeCenter;
  iv.backgroundColor = PadColor(0xFF1F1F23);
  iv.layer.cornerRadius = 8;
  iv.layer.borderWidth = 1;
  iv.layer.borderColor = PadColor(0xFF3A3A40).CGColor;
  PadOnTap(iv, action);
  return PadFixed(iv, 34, 34);
}

- (UIView*)presetCard:(NSDictionary*)p sheet:(PadSheet*)sheet {
  BOOL standard = p[@"file"] == nil;
  CGSize scr = UIScreen.mainScreen.bounds.size;
  CGFloat aspect = fmax(scr.width, scr.height) / fmax(1, fmin(scr.width, scr.height));
  CGSize tsz = CGSizeMake(200, round(200 / aspect));
  UIImageView* thumb = [[UIImageView alloc] initWithImage:presetThumb(p[@"layout"], p[@"style"] ?: self.style, tsz, scr)];
  thumb.contentMode = UIViewContentModeScaleAspectFit;
  [thumb.heightAnchor constraintEqualToAnchor:thumb.widthAnchor multiplier:tsz.height / tsz.width].active = YES;
  UILabel* name = PadLabel(standard ? PadStr(@"pad_preset_standard") : p[@"name"], 13, 0xFFFFFFFF, YES);
  name.numberOfLines = 1;
  UILabel* meta = PadLabel(standard ? PadStr(@"pad_preset_standard_meta")
                                    : [NSString stringWithFormat:[PadStr(@"pad_preset_meta") stringByReplacingOccurrencesOfString:@"%d" withString:@"%lu"],
                                                                 (unsigned long)[p[@"layout"] count]],
                           11, 0xFF9A9AA2, NO);
  __weak WPad* ws = self;
  __weak PadSheet* wsheet = sheet;
  UIButton* apply = PadButton(PadStr(@"pad_preset_apply"), 0xFFE28743, ^{
    NSString* n = standard ? PadStr(@"pad_preset_standard") : p[@"name"];
    PadConfirm(ws.root, [NSString stringWithFormat:PadStr(@"pad_preset_apply_confirm"), n], nil, ^{
      [p[@"icons"] enumerateKeysAndObjectsUsingBlock:^(NSString* k, NSData* v, BOOL* stop) { PadIconImportPng(k, v); }];
      PadWriteLayout(p[@"layout"]);
      if (p[@"style"]) [ws setStyleAndSave:p[@"style"]];
      for (PadEntry* e in ws.entries.allValues) e.json = nil;
      [ws sync];
      [wsheet close];
    });
  });
  NSMutableArray* rowViews = [NSMutableArray arrayWithObject:PadFixed(apply, 0, 34)];
  [rowViews addObject:[self smallIcon:@"ic_ui_export" color:0xFF8FB8FF action:^{ [ws presetExport:p]; }]];
  if (!standard)
    [rowViews addObject:[self smallIcon:@"delete" color:0xFFFF6B6B action:^{
      PadConfirm(ws.root, [NSString stringWithFormat:PadStr(@"pad_preset_delete_confirm"), p[@"name"]], PadStr(@"delete"), ^{
        [NSFileManager.defaultManager removeItemAtPath:p[@"file"] error:nil];
        [ws presetRebuild:wsheet];
      });
    }]];
  UIStackView* row = (UIStackView*)PadHStack(rowViews, 6);
  UIStackView* card = (UIStackView*)PadVStack(@[ thumb, name, meta, row ], 4);
  [card setCustomSpacing:8 afterView:thumb];
  [card setCustomSpacing:8 afterView:meta];
  card.layoutMargins = UIEdgeInsetsMake(8, 8, 8, 8);
  card.layoutMarginsRelativeArrangement = YES;
  card.backgroundColor = PadColor(0xFF26262A);
  card.layer.cornerRadius = 14;
  card.layer.borderWidth = 1;
  card.layer.borderColor = PadColor(standard ? 0xFF4A3A2C : 0xFF34343A).CGColor;
  return card;
}

- (void)presetAskName:(PadSheet*)sheet {
  UIAlertController* a = [UIAlertController alertControllerWithTitle:PadStr(@"pad_preset_name_title") message:PadStr(@"pad_preset_name_hint")
                                                      preferredStyle:UIAlertControllerStyleAlert];
  [a addTextFieldWithConfigurationHandler:^(UITextField* f) {
    f.text = [NSString stringWithFormat:@"preset_%lu", (unsigned long)listPresets().count + 1];
    f.autocorrectionType = UITextAutocorrectionTypeNo;
    f.autocapitalizationType = UITextAutocapitalizationTypeNone;
    f.keyboardType = UIKeyboardTypeASCIICapable;
  }];
  __weak WPad* ws = self;
  __weak PadSheet* wsheet = sheet;
  [a addAction:[UIAlertAction actionWithTitle:PadStr(@"cancel") style:UIAlertActionStyleCancel handler:nil]];
  [a addAction:[UIAlertAction actionWithTitle:PadStr(@"pad_ok") style:UIAlertActionStyleDefault handler:^(UIAlertAction* x) {
    NSString* name = cleanName(a.textFields.firstObject.text ?: @"");
    if (!name.length) return;
    void (^save)(void) = ^{
      NSData* d = buildPreset(name, PadReadLayout(ws.dirs), ws.style);
      if ([d writeToFile:presetPath(name) atomically:YES]) {
        [ws presetRebuild:wsheet];
        PadToast(ws.root, [NSString stringWithFormat:PadStr(@"pad_preset_saved"), name]);
      } else {
        PadToast(ws.root, PadStr(@"pad_save_error"));
      }
    };
    if ([NSFileManager.defaultManager fileExistsAtPath:presetPath(name)])
      PadConfirm(ws.root, [NSString stringWithFormat:PadStr(@"pad_preset_overwrite"), name], nil, save);
    else
      save();
  }]];
  [PadTopVC(self.root) presentViewController:a animated:YES completion:nil];
}

- (void)presetExport:(NSDictionary*)p {
  BOOL standard = p[@"file"] == nil;
  NSString* name = standard ? @"standard" : p[@"name"];
  NSData* data = standard ? buildPreset(name, p[@"layout"], nil) : [NSData dataWithContentsOfFile:p[@"file"]];
  NSString* tmp = [NSTemporaryDirectory() stringByAppendingPathComponent:[name stringByAppendingString:@".torypriset"]];
  if (![data writeToFile:tmp atomically:YES]) return;
  UIDocumentPickerViewController* vc = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[ [NSURL fileURLWithPath:tmp] ] asCopy:YES];
  [PadTopVC(self.root) presentViewController:vc animated:YES completion:nil];
}

- (void)presetImport:(PadSheet*)sheet {
  UIDocumentPickerViewController* vc = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeData ] asCopy:YES];
  PadDocDelegate* d = [PadDocDelegate new];
  __weak WPad* ws = self;
  __weak PadSheet* wsheet = sheet;
  d.picked = ^(NSURL* url) {
    NSData* data = [NSData dataWithContentsOfURL:url];
    NSDictionary* p = data.length < (32u << 20) ? readPreset(data, nil) : nil;
    if (!p) {
      PadToast(ws.root, PadStr(@"pad_preset_bad_file"));
      return;
    }
    NSString* base = cleanName(p[@"name"]);
    if (!base.length) base = cleanName(url.lastPathComponent.stringByDeletingPathExtension);
    if (!base.length) base = @"preset";
    NSString* name = base;
    for (int i = 2; [NSFileManager.defaultManager fileExistsAtPath:presetPath(name)]; i++) name = [NSString stringWithFormat:@"%@_%d", base, i];
    [data writeToFile:presetPath(name) atomically:YES];
    [ws presetRebuild:wsheet];
    PadToast(ws.root, [NSString stringWithFormat:PadStr(@"pad_preset_imported"), name]);
  };
  vc.delegate = d;
  objc_setAssociatedObject(vc, &kWinKey, d, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [PadTopVC(self.root) presentViewController:vc animated:YES completion:nil];
}

- (void)showMouseSettings {
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  __block BOOL useNew = [d objectForKey:@"GameSettings.crosshair_use_new"] ? [d boolForKey:@"GameSettings.crosshair_use_new"] : YES;
  __block NSString* color = [d stringForKey:@"GameSettings.crosshair_color"] ?: @"#00FF00";
  __block int size = [d objectForKey:@"GameSettings.crosshair_size"] ? (int)[d integerForKey:@"GameSettings.crosshair_size"] : 24;
  __block int thick = [d objectForKey:@"GameSettings.crosshair_thickness"] ? (int)[d integerForKey:@"GameSettings.crosshair_thickness"] : 4;
  __block float opacity = [d objectForKey:@"GameSettings.crosshair_opacity"] ? [d floatForKey:@"GameSettings.crosshair_opacity"] : 1;

  PadSheet* sheet = [PadSheet sheetIn:self.root title:PadStr(@"pad_crosshair_title") subtitle:nil width:520];
  PadCrosshairPreview* pv = [PadCrosshairPreview new];
  void (^upd)(void) = ^{
    pv.color = color;
    pv.size = size;
    pv.thick = thick;
    pv.opacity = opacity;
    pv.on = useNew;
    [pv setNeedsDisplay];
  };
  NSMutableArray<UIView*>* lockable = [NSMutableArray new];
  UIView* (^slider)(NSString*, int, int, int, void (^)(int, UILabel*)) = ^UIView*(NSString* title, int mn, int mx, int val, void (^ch)(int, UILabel*)) {
    UILabel* t = PadLabel(title, 11, 0xFFD3D3D3, NO);
    UILabel* v = PadLabel(@"", 11, 0xFFFFFFFF, NO);
    UISlider* s = [UISlider new];
    s.minimumValue = mn;
    s.maximumValue = mx;
    s.value = val;
    s.minimumTrackTintColor = PadColor(0xFF007ACC);
    PadSliderTarget* tg = [PadSliderTarget new];
    __weak UISlider* wsl = s;
    tg.block = ^{ ch((int)lroundf(wsl.value), v); };
    objc_setAssociatedObject(s, &kWinKey, tg, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [s addTarget:tg action:@selector(fire) forControlEvents:UIControlEventValueChanged];
    ch(val, v);
    [lockable addObject:s];
    return PadVStack(@[ PadHStack(@[ t, v ], 8), s ], 2);
  };
  UIView* sizeS = slider(PadStr(@"pad_crosshair_size"), 6, 24, size, ^(int x, UILabel* l) { size = x; l.text = @(x).stringValue; upd(); });
  UIView* thickS = slider(PadStr(@"pad_crosshair_thickness"), 1, 4, thick, ^(int x, UILabel* l) { thick = x; l.text = @(x).stringValue; upd(); });
  UIView* opS = slider(PadStr(@"pad_crosshair_opacity"), 0, 10, (int)lroundf(opacity * 10), ^(int x, UILabel* l) {
    opacity = x / 10.0f;
    l.text = [NSString stringWithFormat:@"%.1f", opacity];
    upd();
  });
  UITextField* hex = [UITextField new];
  hex.text = color;
  hex.textColor = UIColor.whiteColor;
  hex.backgroundColor = PadColor(0xFF2A2A2E);
  hex.layer.cornerRadius = 6;
  hex.autocorrectionType = UITextAutocorrectionTypeNo;
  hex.autocapitalizationType = UITextAutocapitalizationTypeAllCharacters;
  hex.leftView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 8, 8)];
  hex.leftViewMode = UITextFieldViewModeAlways;
  PadSliderTarget* ht = [PadSliderTarget new];
  __weak UITextField* whex = hex;
  ht.block = ^{
    NSString* s = [whex.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
    NSRegularExpression* re = [NSRegularExpression regularExpressionWithPattern:@"^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{8})$" options:0 error:nil];
    if ([re numberOfMatchesInString:s options:0 range:NSMakeRange(0, s.length)]) {
      color = s;
      upd();
    }
  };
  objc_setAssociatedObject(hex, &kWinKey, ht, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [hex addTarget:ht action:@selector(fire) forControlEvents:UIControlEventEditingChanged];
  [lockable addObject:hex];
  NSMutableArray* palette = [NSMutableArray new];
  for (NSString* c in @[ @"#00FF00", @"#FF0000", @"#00FFFF", @"#FFFF00", @"#FFFFFF", @"#FF00FF" ]) {
    UIView* dot = PadFixed([UIView new], 21, 21);
    dot.layer.cornerRadius = 10.5;
    dot.backgroundColor = PadCrosshairColor(c);
    PadOnTap(dot, ^{
      color = c;
      whex.text = c;
      upd();
    });
    [palette addObject:dot];
    [lockable addObject:dot];
  }
  UISwitch* sw = [UISwitch new];
  sw.on = useNew;
  sw.onTintColor = PadColor(0xFF007ACC);
  void (^lock)(void) = ^{
    for (UIView* v in lockable) {
      v.userInteractionEnabled = useNew;
      v.alpha = useNew ? 1 : 0.5;
    }
  };
  PadSliderTarget* swt = [PadSliderTarget new];
  __weak UISwitch* wsw = sw;
  swt.block = ^{
    useNew = wsw.on;
    lock();
    upd();
  };
  objc_setAssociatedObject(sw, &kWinKey, swt, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [sw addTarget:swt action:@selector(fire) forControlEvents:UIControlEventValueChanged];

  UIView* left = PadVStack(@[ PadHStack(@[ PadLabel(PadStr(@"pad_crosshair_new_control"), 13, 0xFFFFFFFF, NO), [UIView new], sw ], 8),
                              sizeS, thickS, opS ], 8);
  UIView* right = PadVStack(@[ PadFixed(pv, 160, 160), PadLabel(PadStr(@"pad_crosshair_color"), 12, 0xFFD3D3D3, NO),
                               PadFixed(hex, 160, 32), PadHStack(palette, 5) ], 6);
  UIStackView* body = (UIStackView*)PadHStack(@[ left, right ], 16);
  body.alignment = UIStackViewAlignmentTop;
  __weak WPad* ws = self;
  __weak PadSheet* wsheet = sheet;
  UIButton* saveB = PadButton(PadStr(@"pad_save"), 0xFF007ACC, ^{
    NSUserDefaults* dd = NSUserDefaults.standardUserDefaults;
    [dd setBool:useNew forKey:@"GameSettings.crosshair_use_new"];
    [dd setObject:color forKey:@"GameSettings.crosshair_color"];
    [dd setInteger:size forKey:@"GameSettings.crosshair_size"];
    [dd setInteger:thick forKey:@"GameSettings.crosshair_thickness"];
    [dd setFloat:opacity forKey:@"GameSettings.crosshair_opacity"];
    if (ws.onMouseSettings) ws.onMouseSettings();
    PadToast(ws.root, PadStr(@"Saves"));
    [wsheet close];
  });
  [sheet.content addArrangedSubview:body];
  [sheet.content addArrangedSubview:PadFixed(saveB, 0, 40)];
  [sheet.content setCustomSpacing:20 afterView:body];
  lock();
  upd();
}

- (PadStatsView*)statsView {
  PadStatsView* v = objc_getAssociatedObject(self, @selector(statsView));
  return v;
}

- (void)toggleStatsWindow {
  PadStatsView* v = [self statsView];
  if (v) {
    [v removeFromSuperview];
    objc_setAssociatedObject(self, @selector(statsView), nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return;
  }
  v = [[PadStatsView alloc] initWithArea:self.root];
  [self.root addSubview:v];
  objc_setAssociatedObject(self, @selector(statsView), v, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (void)statsUpdateFps:(int)render screen:(int)screen { [[self statsView] fps:render screen:screen]; }
- (void)statsUpdateTitle:(NSString*)title { PadStatsTitle(title); [[self statsView] title]; }
- (void)statsAreaChanged { [[self statsView] restorePosition]; }
@end
