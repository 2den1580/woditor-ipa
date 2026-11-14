#import "PadViews.h"

@implementation PadStyledView {
  CALayer* _img;
  BOOL _dirty;
  CGSize _renderedSize;
  CGFloat _renderedScale;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _style = PadStyle.defaultStyle;
    _foregroundFraction = 0.9;
    _padScale = 1;
    _img = [CALayer layer];
    _img.contentsGravity = kCAGravityResize;
    [self.layer addSublayer:_img];
    self.clipsToBounds = NO;
    self.multipleTouchEnabled = YES;
    _dirty = YES;
  }
  return self;
}

- (void)markDirty {
  _dirty = YES;
  [self setNeedsLayout];
}

- (void)setStyle:(PadStyle*)style {
  if (!style || [style isEqualToStyle:_style]) return;
  _style = style;
  [self markDirty];
}
- (void)setBackgroundShape:(PadShape*)s { _backgroundShape = s; [self markDirty]; }
- (void)setForeground:(PadForeground*)f { _foreground = f; _foregroundText = nil; [self markDirty]; }
- (void)setForegroundText:(NSString*)t {
  if ([t isEqualToString:_foregroundText] && !_foreground) return;
  _foregroundText = [t copy];
  _foreground = nil;
  [self markDirty];
}
- (void)setForegroundFraction:(CGFloat)f { if (f != _foregroundFraction) { _foregroundFraction = f; [self markDirty]; } }
- (void)setPadScale:(CGFloat)s {
  _padScale = s;
  self.transform = CGAffineTransformMakeScale(s, s);
  [self setNeedsLayout];
}

- (CGFloat)renderScale {
  CGFloat s = fmax(1, fmin(3, fabs(_padScale)));
  return s * UIScreen.mainScreen.scale;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  CGSize sz = self.bounds.size;
  if (sz.width < 1 || sz.height < 1) return;
  CGFloat rs = [self renderScale];
  if (_dirty || !CGSizeEqualToSize(sz, _renderedSize) || rs != _renderedScale) {
    UIImage* im = PadRender(_backgroundShape, _foreground, _foregroundFraction, _foregroundText, sz, rs, _style, NO);
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    _img.contents = (__bridge id)im.CGImage;
    CGFloat pad = PadRenderPad(_style);
    _img.frame = CGRectInset(self.bounds, -pad, -pad);
    [CATransaction commit];
    _renderedSize = sz;
    _renderedScale = rs;
    _dirty = NO;
  }
}
@end

@implementation PadButtonView {
  BOOL _latched;
  CAShapeLayer* _dot;
  NSInteger _touches;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _onKeyDown = ^(int k) {};
    _onKeyUp = ^(int k) {};
  }
  return self;
}

- (void)setKey:(int)key {
  if (key == _key) return;
  [self releaseLatch];
  _key = key;
}

- (void)setToggle:(BOOL)toggle {
  if (toggle == _toggle) return;
  if (!toggle) [self releaseLatch];
  _toggle = toggle;
  [self updateDot];
}

- (void)releaseLatch {
  if (_latched) {
    _latched = NO;
    _onKeyUp(_key);
    [self updateDot];
  }
}

- (void)releaseKeys {
  if (_touches > 0 && !_toggle) _onKeyUp(_key);
  _touches = 0;
  [self releaseLatch];
  self.layer.sublayerTransform = CATransform3DIdentity;
}

- (void)updateDot {
  if (!_toggle) {
    [_dot removeFromSuperlayer];
    _dot = nil;
    return;
  }
  if (!_dot) {
    _dot = [CAShapeLayer layer];
    [self.layer addSublayer:_dot];
  }
  CGFloat cx = self.bounds.size.width / 2, cy = fmax(5, self.bounds.size.height * 0.17);
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  if (_latched) {
    _dot.path = [UIBezierPath bezierPathWithArcCenter:CGPointMake(cx, cy) radius:4.75 startAngle:0 endAngle:2 * M_PI
                                            clockwise:YES].CGPath;
    _dot.fillColor = PadColor(0xFF69F0AE).CGColor;
    _dot.strokeColor = PadColor(0x4D69F0AE).CGColor;
    _dot.lineWidth = 2.5;
  } else {
    _dot.path = [UIBezierPath bezierPathWithArcCenter:CGPointMake(cx, cy) radius:3 startAngle:0 endAngle:2 * M_PI
                                            clockwise:YES].CGPath;
    _dot.fillColor = UIColor.clearColor.CGColor;
    _dot.strokeColor = PadColor(0x8CFFFFFF).CGColor;
    _dot.lineWidth = 1.3;
  }
  [CATransaction commit];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (_toggle) [self updateDot];
}

- (void)pressAnim:(BOOL)down {
  CGSize s = self.bounds.size;
  self.layer.sublayerTransform = down ? CATransform3DMakeTranslation(-s.width * 0.02, -s.height * 0.02, 0)
                                      : CATransform3DIdentity;
}

- (void)setEditMode:(BOOL)e {
  if (e) [self releaseKeys];
  _editMode = e;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if (self.editMode) return;
  BOOL first = _touches == 0;
  _touches += touches.count;
  if (!first) return;
  [self pressAnim:YES];
  if (!_toggle) _onKeyDown(_key);
  else if (_latched) [self releaseLatch];
  else {
    _latched = YES;
    _onKeyDown(_key);
    [self updateDot];
  }
}

- (void)endTouches:(NSSet<UITouch*>*)touches {
  if (_touches <= 0) return;
  _touches -= touches.count;
  if (_touches > 0) return;
  _touches = 0;
  [self pressAnim:NO];
  if (!_toggle) _onKeyUp(_key);
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self endTouches:touches]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self endTouches:touches]; }

- (void)willMoveToWindow:(UIWindow*)w {
  if (!w) [self releaseKeys];
  [super willMoveToWindow:w];
}
@end

typedef enum { D_UP, D_UP_RIGHT, D_RIGHT, D_DOWN_RIGHT, D_DOWN, D_DOWN_LEFT, D_LEFT, D_UP_LEFT, D_NONE } Dir;

enum { K_UP = 19, K_DOWN = 20, K_LEFT = 21, K_RIGHT = 22 };

static Dir dirFromCross(CGFloat x, CGFloat y, CGFloat w, CGFloat h, BOOL diag) {
  if (w <= 0 || h <= 0) return D_NONE;
  CGFloat nx = (x - w / 2) / (w / 2), ny = (y - h / 2) / (h / 2), arm = 0.40;
  BOOL inV = fabs(nx) <= arm, inH = fabs(ny) <= arm;
  if (!inV && !inH) return D_NONE;
  if (fabs(nx) < 0.05 && fabs(ny) < 0.05) return D_NONE;
  if (diag && fabs(fabs(nx) - fabs(ny)) < 0.15) {
    if (nx > 0 && ny < 0) return D_UP_RIGHT;
    if (nx < 0 && ny < 0) return D_UP_LEFT;
    if (nx > 0 && ny > 0) return D_DOWN_RIGHT;
    if (nx < 0 && ny > 0) return D_DOWN_LEFT;
  }
  if (fabs(nx) > fabs(ny)) return nx > 0 ? D_RIGHT : D_LEFT;
  return ny > 0 ? D_DOWN : D_UP;
}

static Dir dirFromAngle(CGFloat dx, CGFloat dy, CGFloat radius, int sectors, CGFloat dead) {
  if (radius <= 0) return D_NONE;
  if (hypot(dx, dy) < radius * dead) return D_NONE;
  double deg = atan2(dy, dx) * 180 / M_PI;
  if (deg < 0) deg += 360;
  if (sectors == 8) {
    static const Dir eight[] = {D_RIGHT, D_DOWN_RIGHT, D_DOWN, D_DOWN_LEFT, D_LEFT, D_UP_LEFT, D_UP, D_UP_RIGHT};
    return eight[(int)floor(fmod(deg + 22.5, 360) / 45)];
  }
  static const Dir four[] = {D_RIGHT, D_DOWN, D_LEFT, D_UP};
  return four[(int)floor(fmod(deg + 45, 360) / 90)];
}

static int keysOf(Dir d, int* k) {
  switch (d) {
    case D_UP: k[0] = K_UP; return 1;
    case D_DOWN: k[0] = K_DOWN; return 1;
    case D_LEFT: k[0] = K_LEFT; return 1;
    case D_RIGHT: k[0] = K_RIGHT; return 1;
    case D_UP_RIGHT: k[0] = K_UP; k[1] = K_RIGHT; return 2;
    case D_UP_LEFT: k[0] = K_UP; k[1] = K_LEFT; return 2;
    case D_DOWN_RIGHT: k[0] = K_DOWN; k[1] = K_RIGHT; return 2;
    case D_DOWN_LEFT: k[0] = K_DOWN; k[1] = K_LEFT; return 2;
    default: return 0;
  }
}

@interface PadDPadView ()
- (void)setDir:(Dir)d;
- (void)fingerDown:(CGPoint)p;
- (void)fingerAt:(CGPoint)p;
- (void)fingerUp;
@end

@implementation PadDPadView {
  Dir _dir;
  UITouch* _finger;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _dir = D_NONE;
    _shape = @"dpad";
    self.foregroundFraction = 1;
  }
  return self;
}

- (void)setForegroundText:(NSString*)t {}

- (void)setShape:(NSString*)shape { _shape = PadShapeNormalize(shape); }

- (Dir)directionAt:(CGPoint)p {
  CGFloat w = self.bounds.size.width, h = self.bounds.size.height;
  if ([_shape isEqualToString:@"dpad_disc"] || [_shape isEqualToString:@"dpad_four"])
    return dirFromAngle(p.x - w / 2, p.y - h / 2, fmin(w, h) / 2, 4, 0.18);
  if ([_shape isEqualToString:@"dpad_8"]) return dirFromAngle(p.x - w / 2, p.y - h / 2, fmin(w, h) / 2, 8, 0.18);
  return dirFromCross(p.x, p.y, w, h, self.diagonal);
}

- (void)setDir:(Dir)nd {
  if (nd == _dir) return;
  int was[2], now[2];
  int nw = keysOf(_dir, was), nn = keysOf(nd, now);
  _dir = nd;
  for (int i = 0; i < nw; i++) {
    BOOL keep = NO;
    for (int j = 0; j < nn; j++) keep |= now[j] == was[i];
    if (!keep) self.onKeyUp(was[i]);
  }
  for (int i = 0; i < nn; i++) {
    BOOL had = NO;
    for (int j = 0; j < nw; j++) had |= was[j] == now[i];
    if (!had) self.onKeyDown(now[i]);
  }
}

- (void)releaseKeys {
  _finger = nil;
  [self setDir:D_NONE];
}

- (void)fingerAt:(CGPoint)p { [self setDir:[self directionAt:p]]; }
- (void)fingerUp { [self setDir:D_NONE]; }

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if (self.editMode || _finger) return;
  _finger = touches.anyObject;
  [self fingerDown:[_finger locationInView:self]];
}
- (void)fingerDown:(CGPoint)p { [self fingerAt:p]; }

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if (_finger && [touches containsObject:_finger]) [self fingerAt:[_finger locationInView:self]];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if (_finger && [touches containsObject:_finger]) {
    _finger = nil;
    [self fingerUp];
  }
}
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self touchesEnded:touches withEvent:event]; }
@end

static const CGFloat kKnob = 0.45, kDeadZone = 0.3;

@implementation PadStickView {
  BOOL _floating, _touching;
  CGFloat _ox, _oy, _kx, _ky;
  CALayer *_base, *_knob;
  CAShapeLayer *_ring, *_zone;
  CGFloat _renderedBase, _renderedScale;
  PadStyle* _renderedStyle;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _base = [CALayer layer];
    _knob = [CALayer layer];
    _ring = [CAShapeLayer layer];
    _ring.fillColor = UIColor.clearColor.CGColor;
    _ring.lineWidth = 1.5;
    _zone = [CAShapeLayer layer];
    _zone.fillColor = UIColor.clearColor.CGColor;
    _zone.strokeColor = PadColor(0x73FFFFFF).CGColor;
    _zone.lineWidth = 1.5;
    _zone.lineDashPattern = @[ @7, @6 ];
    for (CALayer* l in @[ _zone, _base, _knob, _ring ]) [self.layer addSublayer:l];
    self.backgroundShape = nil;
  }
  return self;
}

- (void)setBackgroundShape:(PadShape*)s { [super setBackgroundShape:nil]; }
- (void)setForeground:(PadForeground*)f {}

- (void)setShape:(NSString*)shape {
  [super setShape:shape];
  _floating = [PadShapeNormalize(shape) isEqualToString:@"stick_float"];
  [self setNeedsLayout];
}

- (void)setShowZone:(BOOL)show {
  _showZone = show;
  [self setNeedsLayout];
}

- (void)setStyle:(PadStyle*)style {
  [super setStyle:style];
  [self setNeedsLayout];
}

- (CGFloat)baseSize {
  CGFloat m = fmin(self.bounds.size.width, self.bounds.size.height);
  return _floating ? fmin(PadSdp(120), m) : m;
}
- (CGFloat)maxTravel { CGFloat b = [self baseSize]; return b / 2 - b * kKnob / 2; }

- (void)layoutSubviews {
  [super layoutSubviews];
  CGFloat base = [self baseSize], knob = base * kKnob;
  if (base < 1) return;
  CGFloat rs = fmax(1, fmin(3, fabs(self.padScale))) * UIScreen.mainScreen.scale;
  PadStyle* st = self.style;
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  if (!_base.contents || _renderedBase != base || _renderedScale != rs || ![st isEqualToStyle:_renderedStyle]) {
    PadShape* c = PadShape.circle;
    _base.contents = (__bridge id)PadRender(c, nil, 1, nil, CGSizeMake(base, base), rs, st, NO).CGImage;
    _knob.contents = (__bridge id)PadRender(c, nil, 1, nil, CGSizeMake(knob, knob), rs, st, YES).CGImage;
    _ring.strokeColor = PadColor((st.contentColor & 0x00FFFFFF) | 0x73000000).CGColor;
    _renderedBase = base;
    _renderedScale = rs;
    _renderedStyle = st;
  }
  CGFloat pad = PadRenderPad(st);
  CGFloat cx = _touching ? _ox : self.bounds.size.width / 2, cy = _touching ? _oy : self.bounds.size.height / 2;
  _base.frame = CGRectMake(cx - base / 2 - pad, cy - base / 2 - pad, base + 2 * pad, base + 2 * pad);
  CGFloat kcx = cx + _kx, kcy = cy + _ky;
  _knob.frame = CGRectMake(kcx - knob / 2 - pad, kcy - knob / 2 - pad, knob + 2 * pad, knob + 2 * pad);
  _ring.path = [UIBezierPath bezierPathWithArcCenter:CGPointMake(kcx, kcy) radius:knob * 0.28 startAngle:0
                                            endAngle:2 * M_PI clockwise:YES].CGPath;

  CGFloat op = _floating && !_touching ? 0.5 : 1;
  _base.opacity = _knob.opacity = _ring.opacity = op;
  _zone.hidden = !(_floating && _showZone);
  _zone.path = [UIBezierPath bezierPathWithRoundedRect:CGRectInset(self.bounds, 1, 1) cornerRadius:12].CGPath;
  [CATransaction commit];
}

static CGFloat clampf(CGFloat v, CGFloat lo, CGFloat hi) { return lo > hi ? (lo + hi) / 2 : fmax(lo, fmin(hi, v)); }

- (void)fingerDown:(CGPoint)p {
  _touching = YES;
  if (_floating) {
    CGFloat r = [self baseSize] / 2;
    _ox = clampf(p.x, r, self.bounds.size.width - r);
    _oy = clampf(p.y, r, self.bounds.size.height - r);
  } else {
    _ox = self.bounds.size.width / 2;
    _oy = self.bounds.size.height / 2;
  }
  [self fingerAt:p];
}

- (void)fingerAt:(CGPoint)p {
  CGFloat dx = p.x - _ox, dy = p.y - _oy, max = [self maxTravel], len = hypot(dx, dy);
  if (len > max && len > 0) {
    _kx = dx * max / len;
    _ky = dy * max / len;
  } else {
    _kx = dx;
    _ky = dy;
  }
  [self setDir:dirFromAngle(dx, dy, max, 8, kDeadZone)];
  [self setNeedsLayout];
}

- (void)fingerUp {
  _touching = NO;
  _kx = _ky = 0;
  [self setDir:D_NONE];
  [self setNeedsLayout];
}

- (void)releaseKeys {
  [super releaseKeys];
  _touching = NO;
  _kx = _ky = 0;
  [self setNeedsLayout];
}
@end
