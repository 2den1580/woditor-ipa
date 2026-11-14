#import "PadGfx.h"

#include "PadVectors.inc"

typedef struct { const char* s; } Lex;

static void skipSep(Lex* l) {
  while (*l->s == ' ' || *l->s == ',' || *l->s == '\n' || *l->s == '\t' || *l->s == '\r') l->s++;
}

static BOOL isNumStart(char ch) { return (ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.'; }

static BOOL num(Lex* l, CGFloat* out) {
  skipSep(l);
  if (!isNumStart(*l->s)) return NO;
  char* end;
  double v = strtod(l->s, &end);
  if (end == l->s) return NO;
  l->s = end;
  *out = v;
  return YES;
}

static BOOL flag(Lex* l, BOOL* out) {
  skipSep(l);
  if (*l->s != '0' && *l->s != '1') return NO;
  *out = *l->s == '1';
  l->s++;
  return YES;
}

static void addArc(CGMutablePathRef p, CGFloat x1, CGFloat y1, CGFloat rx, CGFloat ry, CGFloat phiDeg, BOOL large,
                   BOOL sweep, CGFloat x2, CGFloat y2) {
  if (rx == 0 || ry == 0) { CGPathAddLineToPoint(p, NULL, x2, y2); return; }
  if (x1 == x2 && y1 == y2) return;
  rx = fabs(rx); ry = fabs(ry);
  CGFloat phi = phiDeg * M_PI / 180, cp = cos(phi), sp = sin(phi);
  CGFloat dx = (x1 - x2) / 2, dy = (y1 - y2) / 2;
  CGFloat x1p = cp * dx + sp * dy, y1p = -sp * dx + cp * dy;
  CGFloat lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
  if (lam > 1) { CGFloat s = sqrt(lam); rx *= s; ry *= s; }
  CGFloat num_ = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p;
  CGFloat den = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
  CGFloat co = den > 0 ? sqrt(fmax(0, num_ / den)) : 0;
  if (large == sweep) co = -co;
  CGFloat cxp = co * rx * y1p / ry, cyp = -co * ry * x1p / rx;
  CGFloat cx = cp * cxp - sp * cyp + (x1 + x2) / 2, cy = sp * cxp + cp * cyp + (y1 + y2) / 2;
  CGFloat ux = (x1p - cxp) / rx, uy = (y1p - cyp) / ry, vx = (-x1p - cxp) / rx, vy = (-y1p - cyp) / ry;
  CGFloat th1 = atan2(uy, ux), dth = atan2(ux * vy - uy * vx, ux * vx + uy * vy);
  if (!sweep && dth > 0) dth -= 2 * M_PI;
  else if (sweep && dth < 0) dth += 2 * M_PI;
  int segs = (int)ceil(fabs(dth) / (M_PI / 2));
  CGFloat d = dth / segs, t = 4.0 / 3 * tan(d / 4);
  for (int i = 0; i < segs; i++) {
    CGFloat a1 = th1 + i * d, a2 = a1 + d;
    CGFloat c1 = cos(a1), s1 = sin(a1), c2 = cos(a2), s2 = sin(a2);
    CGFloat ex1 = c1 - t * s1, ey1 = s1 + t * c1, ex2 = c2 + t * s2, ey2 = s2 - t * c2;
#define TX(X, Y) (cx + rx * (X) * cp - ry * (Y) * sp)
#define TY(X, Y) (cy + rx * (X) * sp + ry * (Y) * cp)
    CGPathAddCurveToPoint(p, NULL, TX(ex1, ey1), TY(ex1, ey1), TX(ex2, ey2), TY(ex2, ey2), TX(c2, s2), TY(c2, s2));
#undef TX
#undef TY
  }
}

static CGPathRef parsePath(const char* d) {
  CGMutablePathRef p = CGPathCreateMutable();
  Lex l = {d};
  char cmd = 0;
  CGFloat x = 0, y = 0, sx = 0, sy = 0, lcx = 0, lcy = 0;
  char prev = 0;
  for (;;) {
    skipSep(&l);
    if (!*l.s) break;
    if (!isNumStart(*l.s)) cmd = *l.s++;
    else if (!cmd) break;
    BOOL rel = cmd >= 'a' && cmd <= 'z';
    char C = rel ? cmd - 32 : cmd;
    CGFloat ox = rel ? x : 0, oy = rel ? y : 0;
    CGFloat a, b, c2, d2, e, f;
    switch (C) {
      case 'M':
        if (!num(&l, &a) || !num(&l, &b)) goto done;
        x = ox + a; y = oy + b; sx = x; sy = y;
        CGPathMoveToPoint(p, NULL, x, y);
        cmd = rel ? 'l' : 'L';
        break;
      case 'L':
        if (!num(&l, &a) || !num(&l, &b)) goto done;
        x = ox + a; y = oy + b;
        CGPathAddLineToPoint(p, NULL, x, y);
        break;
      case 'H':
        if (!num(&l, &a)) goto done;
        x = ox + a;
        CGPathAddLineToPoint(p, NULL, x, y);
        break;
      case 'V':
        if (!num(&l, &a)) goto done;
        y = (rel ? y : 0) + a;
        CGPathAddLineToPoint(p, NULL, x, y);
        break;
      case 'C':
        if (!num(&l, &a) || !num(&l, &b) || !num(&l, &c2) || !num(&l, &d2) || !num(&l, &e) || !num(&l, &f)) goto done;
        CGPathAddCurveToPoint(p, NULL, ox + a, oy + b, ox + c2, oy + d2, ox + e, oy + f);
        lcx = ox + c2; lcy = oy + d2; x = ox + e; y = oy + f;
        break;
      case 'S': {
        if (!num(&l, &c2) || !num(&l, &d2) || !num(&l, &e) || !num(&l, &f)) goto done;
        CGFloat rx1 = (prev == 'C' || prev == 'S') ? 2 * x - lcx : x, ry1 = (prev == 'C' || prev == 'S') ? 2 * y - lcy : y;
        CGPathAddCurveToPoint(p, NULL, rx1, ry1, ox + c2, oy + d2, ox + e, oy + f);
        lcx = ox + c2; lcy = oy + d2; x = ox + e; y = oy + f;
        break;
      }
      case 'Q':
        if (!num(&l, &a) || !num(&l, &b) || !num(&l, &e) || !num(&l, &f)) goto done;
        CGPathAddQuadCurveToPoint(p, NULL, ox + a, oy + b, ox + e, oy + f);
        lcx = ox + a; lcy = oy + b; x = ox + e; y = oy + f;
        break;
      case 'T': {
        if (!num(&l, &e) || !num(&l, &f)) goto done;
        CGFloat qx = (prev == 'Q' || prev == 'T') ? 2 * x - lcx : x, qy = (prev == 'Q' || prev == 'T') ? 2 * y - lcy : y;
        CGPathAddQuadCurveToPoint(p, NULL, qx, qy, ox + e, oy + f);
        lcx = qx; lcy = qy; x = ox + e; y = oy + f;
        break;
      }
      case 'A': {
        CGFloat rx, ry, rot; BOOL la, sw;
        if (!num(&l, &rx) || !num(&l, &ry) || !num(&l, &rot) || !flag(&l, &la) || !flag(&l, &sw) || !num(&l, &e) ||
            !num(&l, &f))
          goto done;
        addArc(p, x, y, rx, ry, rot, la, sw, ox + e, oy + f);
        x = ox + e; y = oy + f;
        break;
      }
      case 'Z':
        CGPathCloseSubpath(p);
        x = sx; y = sy;
        break;
      default:
        goto done;
    }
    prev = C;
    if (C == 'Z') cmd = 0;
  }
done:
  return p;
}

const PadVec* PadVecFind(NSString* name) {
  if (!name.length) return NULL;
  const char* n = name.UTF8String;
  for (size_t i = 0; i < sizeof kPadVectors / sizeof kPadVectors[0]; i++)
    if (!strcmp(kPadVectors[i].name, n)) return &kPadVectors[i];
  return NULL;
}

static CGPathRef vecPath(const PadVec* v, int i) {
  static NSMutableDictionary* cache;
  if (!cache) cache = [NSMutableDictionary new];
  NSString* key = [NSString stringWithFormat:@"%s#%d", v->name, i];
  id p = cache[key];
  if (!p) {
    CGPathRef cp = parsePath(v->paths[i].d);
    p = (__bridge_transfer id)cp;
    cache[key] = p;
  }
  return (__bridge CGPathRef)p;
}

void PadVecDraw(CGContextRef c, const PadVec* v, CGRect r, UIColor* color) {
  if (!v) return;
  CGFloat sx = r.size.width / v->vw, sy = r.size.height / v->vh;
  CGFloat cr, cg, cb, ca;
  [color getRed:&cr green:&cg blue:&cb alpha:&ca];
  for (int i = 0; i < v->n; i++) {
    const PadVecPath* pp = &v->paths[i];
    CGContextSaveGState(c);
    CGContextTranslateCTM(c, r.origin.x, r.origin.y);
    CGContextScaleCTM(c, sx, sy);
    CGContextTranslateCTM(c, pp->tx, pp->ty);
    CGPathRef path = vecPath(v, i);
    if (pp->fillA > 0) {
      CGContextSetRGBFillColor(c, cr, cg, cb, ca * pp->fillA);
      CGContextAddPath(c, path);
      if (pp->evenOdd) CGContextEOFillPath(c); else CGContextFillPath(c);
    }
    if (pp->strokeA > 0 && pp->strokeW > 0) {
      CGContextSetRGBStrokeColor(c, cr, cg, cb, ca * pp->strokeA);
      CGContextSetLineWidth(c, pp->strokeW);
      CGContextSetLineCap(c, pp->cap == 1 ? kCGLineCapRound : pp->cap == 2 ? kCGLineCapSquare : kCGLineCapButt);
      CGContextSetLineJoin(c, pp->join == 1 ? kCGLineJoinRound : pp->join == 2 ? kCGLineJoinBevel : kCGLineJoinMiter);
      CGContextAddPath(c, path);
      CGContextStrokePath(c);
    }
    CGContextRestoreGState(c);
  }
}

const uint32_t kPadNeonColors[8] = {0xFF00E5FF, 0xFFFF2BD6, 0xFF39FF14, 0xFFFF9100,
                                    0xFFB388FF, 0xFFFF1744, 0xFFFFEA00, 0xFFFFFFFF};

UIColor* PadColor(uint32_t c) {
  return [UIColor colorWithRed:((c >> 16) & 255) / 255.0 green:((c >> 8) & 255) / 255.0 blue:(c & 255) / 255.0
                         alpha:((c >> 24) & 255) / 255.0];
}

static uint32_t mixColor(uint32_t a, uint32_t b, float t) {
  int r = (int)lroundf(((a >> 16) & 255) + ((int)((b >> 16) & 255) - (int)((a >> 16) & 255)) * t);
  int g = (int)lroundf(((a >> 8) & 255) + ((int)((b >> 8) & 255) - (int)((a >> 8) & 255)) * t);
  int bl = (int)lroundf((a & 255) + ((int)(b & 255) - (int)(a & 255)) * t);
  return 0xFF000000u | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)bl;
}

@implementation PadStyle
+ (instancetype)styleWithType:(int)type neon:(uint32_t)neon {
  PadStyle* s = [PadStyle new];
  s->_type = (type >= PAD_DARK && type <= PAD_NEON) ? type : PAD_GLASS;
  s->_neonColor = neon;
  return s;
}
+ (instancetype)defaultStyle { return [self styleWithType:PAD_GLASS neon:kPadNeonColors[0]]; }
+ (instancetype)load {
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  if (![d objectForKey:@"pad_style.type"]) return [self defaultStyle];
  return [self styleWithType:(int)[d integerForKey:@"pad_style.type"]
                        neon:(uint32_t)[d integerForKey:@"pad_style.neon_color"]];
}
- (void)save {
  [NSUserDefaults.standardUserDefaults setInteger:_type forKey:@"pad_style.type"];
  [NSUserDefaults.standardUserDefaults setInteger:_neonColor forKey:@"pad_style.neon_color"];
}
- (BOOL)isEqualToStyle:(PadStyle*)s {
  return s && s.type == _type && (_type != PAD_NEON || s.neonColor == _neonColor);
}
- (uint32_t)fillColor {
  switch (_type) {
    case PAD_DARK: return 0xFF202020;
    case PAD_LIGHT: return 0x2EFFFFFF;
    case PAD_NEON: return 0x730A0F1E;
    default: return 0x66000000;
  }
}
- (uint32_t)knobFillColor {
  switch (_type) {
    case PAD_DARK: return 0xFF545454;
    case PAD_LIGHT: return 0x5CFFFFFF;
    case PAD_NEON: return (_neonColor & 0x00FFFFFF) | 0x47000000;
    default: return 0x47FFFFFF;
  }
}
- (CGFloat)borderDp { return _type == PAD_NEON ? 1.6 : 0; }
- (uint32_t)borderColor {
  switch (_type) {
    case PAD_LIGHT: return 0xA6FFFFFF;
    case PAD_NEON: return _neonColor;
    default: return 0x8CFFFFFF;
  }
}
- (CGFloat)glowDp { return _type == PAD_NEON ? 3.5 : 0; }
- (uint32_t)contentColor { return _type == PAD_NEON ? mixColor(_neonColor, 0xFFFFFFFF, 0.6f) : 0xFFFFFFFF; }
- (int)contentEffect {
  switch (_type) {
    case PAD_LIGHT: return PAD_EFFECT_SHADOW;
    case PAD_NEON: return PAD_EFFECT_GLOW;
    default: return PAD_EFFECT_NONE;
  }
}
- (uint32_t)effectColor { return _type == PAD_NEON ? _neonColor : 0xD9000000; }
@end

static CGFloat g_minSide = 375;
void PadSetScreenMinSide(CGFloat pts) { if (pts > 0) g_minSide = pts; }

CGFloat PadSdp(CGFloat sdp) {
  static const int buckets[] = {300, 330, 360, 390, 420, 450, 480, 510, 540, 570, 600, 630,
                                660, 690, 720, 750, 780, 1080};
  int b = 0;
  for (size_t i = 0; i < sizeof buckets / sizeof buckets[0]; i++)
    if (g_minSide >= buckets[i]) b = buckets[i];
  return b ? floor(sdp * b / 300.0) : sdp;
}

NSString* PadShapeNormalize(NSString* shape) {
  return shape ? [shape stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet].lowercaseString
               : @"circle";
}

int PadShapeViewKind(NSString* shape) {
  NSString* s = PadShapeNormalize(shape);
  if ([s isEqualToString:@"stick"] || [s isEqualToString:@"stick_float"]) return PAD_VIEW_STICK;
  if ([s hasPrefix:@"dpad"]) return PAD_VIEW_DPAD;
  return PAD_VIEW_BUTTON;
}

CGSize PadShapeSize(NSString* shape) {
  NSString* s = PadShapeNormalize(shape);
  if ([@[ @"circle", @"hexagon", @"keycap" ] containsObject:s]) return CGSizeMake(PadSdp(45), PadSdp(45));
  if ([@[ @"rectangle", @"pill", @"shoulder_l", @"shoulder_r" ] containsObject:s])
    return CGSizeMake(PadSdp(60), PadSdp(30));
  if ([s isEqualToString:@"stick_float"]) return CGSizeMake(PadSdp(190), PadSdp(150));
  if (PadShapeViewKind(s) != PAD_VIEW_BUTTON) return CGSizeMake(PadSdp(120), PadSdp(120));
  return CGSizeMake(PadSdp(30), PadSdp(30));
}

enum { SH_VECTOR = 0, SH_PILL, SH_HEXAGON, SH_KEYCAP, SH_SHOULDER_L, SH_SHOULDER_R, SH_CIRCLE, SH_FOUR };

@implementation PadShape {
  int _kind;
  const PadVec* _vec;
}

+ (PadShape*)kind:(int)k vector:(NSString*)name {
  PadShape* s = [PadShape new];
  s->_kind = k;
  s->_vec = name ? PadVecFind(name) : NULL;
  if (k == SH_VECTOR && !s->_vec) s->_kind = SH_CIRCLE;
  return s;
}

+ (PadShape*)circle { return [self kind:SH_CIRCLE vector:nil]; }

+ (PadShape*)backgroundFor:(NSString*)shape {
  NSString* s = PadShapeNormalize(shape);
  if ([s isEqualToString:@"circle"]) return [self kind:SH_VECTOR vector:@"action_back_circle"];
  if ([s isEqualToString:@"pill"]) return [self kind:SH_PILL vector:nil];
  if ([s isEqualToString:@"hexagon"]) return [self kind:SH_HEXAGON vector:nil];
  if ([s isEqualToString:@"keycap"]) return [self kind:SH_KEYCAP vector:nil];
  if ([s isEqualToString:@"shoulder_l"]) return [self kind:SH_SHOULDER_L vector:nil];
  if ([s isEqualToString:@"shoulder_r"]) return [self kind:SH_SHOULDER_R vector:nil];
  if ([s isEqualToString:@"dpad"]) return [self kind:SH_VECTOR vector:@"dpad_back"];
  if ([s isEqualToString:@"dpad_1"]) return [self kind:SH_VECTOR vector:@"dpad_back2"];
  if ([@[ @"dpad_disc", @"dpad_8", @"stick", @"stick_float" ] containsObject:s]) return [self circle];
  if ([s isEqualToString:@"dpad_four"]) return [self kind:SH_FOUR vector:nil];
  return [self kind:SH_VECTOR vector:@"action_back_square"];
}

static CGRect faceRect(CGRect b) {
  CGFloat w = b.size.width, h = b.size.height;
  return CGRectMake(b.origin.x + w * 0.04, b.origin.y, w - 2 * w * 0.04, h * 0.86);
}

static void addRoundRect(CGMutablePathRef p, CGRect r, CGFloat tl, CGFloat tr, CGFloat br, CGFloat bl) {
  CGFloat x0 = CGRectGetMinX(r), y0 = CGRectGetMinY(r), x1 = CGRectGetMaxX(r), y1 = CGRectGetMaxY(r);
  CGFloat m = fmin(r.size.width, r.size.height) / 2;
  tl = fmin(tl, m); tr = fmin(tr, m); br = fmin(br, m); bl = fmin(bl, m);
  CGPathMoveToPoint(p, NULL, x0 + tl, y0);
  CGPathAddArcToPoint(p, NULL, x1, y0, x1, y1, tr);
  CGPathAddArcToPoint(p, NULL, x1, y1, x0, y1, br);
  CGPathAddArcToPoint(p, NULL, x0, y1, x0, y0, bl);
  CGPathAddArcToPoint(p, NULL, x0, y0, x1, y0, tl);
  CGPathCloseSubpath(p);
}

- (CGPathRef)newPath:(CGRect)b {
  CGMutablePathRef p = CGPathCreateMutable();
  CGFloat w = b.size.width, h = b.size.height, l = b.origin.x, t = b.origin.y;
  switch (_kind) {
    case SH_PILL: addRoundRect(p, b, h / 2, h / 2, h / 2, h / 2); break;
    case SH_HEXAGON: {

      CGFloat r = fmin(w / sqrt(3), h / 2), cx = l + w / 2, cy = t + h / 2, rr = r * 0.14;
      CGPoint pt[6];
      for (int i = 0; i < 6; i++) {
        double a = M_PI / 6 + i * M_PI / 3;
        pt[i] = CGPointMake(cx + r * cos(a), cy + r * sin(a));
      }
      CGPathMoveToPoint(p, NULL, (pt[0].x + pt[1].x) / 2, (pt[0].y + pt[1].y) / 2);
      for (int i = 1; i <= 6; i++) {
        CGPoint a = pt[i % 6], n = pt[(i + 1) % 6];
        CGPathAddArcToPoint(p, NULL, a.x, a.y, (a.x + n.x) / 2, (a.y + n.y) / 2, rr);
      }
      CGPathCloseSubpath(p);
      break;
    }
    case SH_KEYCAP: {
      CGRect f = faceRect(b);
      addRoundRect(p, f, h * 0.16, h * 0.16, h * 0.16, h * 0.16);
      addRoundRect(p, CGRectMake(l, t + h * 0.12, w, h * 0.88), h * 0.18, h * 0.18, h * 0.18, h * 0.18);
      break;
    }
    case SH_SHOULDER_L: addRoundRect(p, b, h * 0.72, h * 0.22, h * 0.22, h * 0.22); break;
    case SH_SHOULDER_R: addRoundRect(p, b, h * 0.22, h * 0.72, h * 0.22, h * 0.22); break;
    case SH_FOUR: {
      CGFloat s = fmin(w, h), r = s * 0.173, gap = s * 0.013, cx = l + w / 2, cy = t + h / 2;
      CGPathAddEllipseInRect(p, NULL, CGRectMake(cx - r, t + gap, 2 * r, 2 * r));
      CGPathAddEllipseInRect(p, NULL, CGRectMake(l + w - 2 * r - gap, cy - r, 2 * r, 2 * r));
      CGPathAddEllipseInRect(p, NULL, CGRectMake(cx - r, t + h - 2 * r - gap, 2 * r, 2 * r));
      CGPathAddEllipseInRect(p, NULL, CGRectMake(l + gap, cy - r, 2 * r, 2 * r));
      break;
    }
    default: {
      CGFloat s = fmin(w, h);
      CGPathAddEllipseInRect(p, NULL, CGRectMake(l + (w - s) / 2, t + (h - s) / 2, s, s));
      break;
    }
  }
  return p;
}

- (void)fill:(CGContextRef)c rect:(CGRect)r {
  if (_kind == SH_VECTOR) {
    PadVecDraw(c, _vec, r, UIColor.blackColor);
    return;
  }
  CGPathRef p = [self newPath:r];
  CGContextSetRGBFillColor(c, 0, 0, 0, 1);
  CGContextAddPath(c, p);
  CGContextFillPath(c);
  CGPathRelease(p);
}

- (CGRect)contentRect:(CGRect)r { return _kind == SH_KEYCAP ? faceRect(r) : r; }

- (void)drawDetails:(CGContextRef)c rect:(CGRect)b border:(uint32_t)color width:(CGFloat)bw {
  if (_kind != SH_KEYCAP) return;
  CGFloat h = b.size.height;
  CGRect face = faceRect(b);

  CGContextSaveGState(c);
  CGMutablePathRef lip = CGPathCreateMutable();
  addRoundRect(lip, CGRectMake(b.origin.x, b.origin.y + h * 0.12, b.size.width, h * 0.88), h * 0.18, h * 0.18,
               h * 0.18, h * 0.18);
  CGContextAddPath(c, lip);
  CGContextClip(c);
  CGMutablePathRef out = CGPathCreateMutable();
  CGPathAddRect(out, NULL, CGRectInset(b, -bw * 4 - 4, -bw * 4 - 4));
  addRoundRect(out, face, h * 0.16, h * 0.16, h * 0.16, h * 0.16);
  CGContextAddPath(c, out);
  CGContextEOClip(c);
  CGContextSetRGBFillColor(c, 0, 0, 0, 0x4D / 255.0);
  CGContextFillRect(c, CGRectInset(b, -4, -4));
  CGContextRestoreGState(c);
  CGPathRelease(lip);
  CGPathRelease(out);

  CGFloat lw = fmax(1, bw), in = lw / 2;
  CGMutablePathRef fp = CGPathCreateMutable();
  addRoundRect(fp, CGRectInset(face, in, in), h * 0.16, h * 0.16, h * 0.16, h * 0.16);
  CGContextSetStrokeColorWithColor(c, PadColor(color).CGColor);
  CGContextSetLineWidth(c, lw);
  CGContextAddPath(c, fp);
  CGContextStrokePath(c);
  CGPathRelease(fp);
}
@end

enum { FG_VECTOR = 0, FG_DISC, FG_FOUR, FG_EIGHT, FG_IMAGE };

@implementation PadForeground {
  int _kind;
  const PadVec* _vec;
  UIImage* _image;
  BOOL _tint;
}

+ (PadForeground*)image:(UIImage*)img tint:(BOOL)tint {
  if (!img) return nil;
  PadForeground* f = [self kind:FG_IMAGE];
  f->_image = img;
  f->_tint = tint;
  return f;
}

+ (PadForeground*)kind:(int)k {
  PadForeground* f = [PadForeground new];
  f->_kind = k;
  return f;
}

+ (PadForeground*)vector:(NSString*)name {
  const PadVec* v = PadVecFind(name);
  if (!v) return nil;
  PadForeground* f = [self kind:FG_VECTOR];
  f->_vec = v;
  return f;
}

+ (PadForeground*)foregroundFor:(NSString*)shape {
  NSString* s = PadShapeNormalize(shape);
  if ([s isEqualToString:@"dpad"] || [s isEqualToString:@"dpad_1"]) {
    PadForeground* f = [self vector:@"dpad_front"];
    return f ? f : [self kind:FG_DISC];
  }
  if ([s isEqualToString:@"dpad_disc"]) return [self kind:FG_DISC];
  if ([s isEqualToString:@"dpad_four"]) return [self kind:FG_FOUR];
  if ([s isEqualToString:@"dpad_8"]) return [self kind:FG_EIGHT];
  return nil;
}

static void arrow(CGContextRef c, CGFloat cx, CGFloat cy, CGFloat dist, int deg, CGFloat size, UIColor* col) {
  CGContextSaveGState(c);
  CGContextTranslateCTM(c, cx, cy);
  CGContextRotateCTM(c, deg * M_PI / 180);
  CGContextMoveToPoint(c, 0, -dist - size * 0.6);
  CGContextAddLineToPoint(c, size * 0.55, -dist + size * 0.35);
  CGContextAddLineToPoint(c, -size * 0.55, -dist + size * 0.35);
  CGContextClosePath(c);
  CGContextSetFillColorWithColor(c, col.CGColor);
  CGContextFillPath(c);
  CGContextRestoreGState(c);
}

- (void)draw:(CGContextRef)c rect:(CGRect)b color:(UIColor*)color {
  if (_kind == FG_IMAGE) {

    CGSize is = _image.size;
    CGFloat k = fmin(b.size.width / fmax(1, is.width), b.size.height / fmax(1, is.height));
    CGRect r = CGRectMake(CGRectGetMidX(b) - is.width * k / 2, CGRectGetMidY(b) - is.height * k / 2, is.width * k,
                          is.height * k);
    UIGraphicsPushContext(c);
    if (_tint) {

      CGContextBeginTransparencyLayerWithRect(c, r, NULL);
      [_image drawInRect:r];
      CGContextSetBlendMode(c, kCGBlendModeSourceIn);
      CGContextSetFillColorWithColor(c, color.CGColor);
      CGContextFillRect(c, r);
      CGContextEndTransparencyLayer(c);
    } else {
      [_image drawInRect:r];
    }
    UIGraphicsPopContext();
    return;
  }
  if (_kind == FG_VECTOR) {
    PadVecDraw(c, _vec, b, color);
    return;
  }
  CGFloat s = fmin(b.size.width, b.size.height), cx = CGRectGetMidX(b), cy = CGRectGetMidY(b);
  CGFloat a0;
  [color getRed:NULL green:NULL blue:NULL alpha:&a0];
  UIColor* (^op)(CGFloat) = ^(CGFloat o) { return [color colorWithAlphaComponent:a0 * o]; };
  CGFloat lw = fmax(1, s * 0.008);
  CGContextSetLineWidth(c, lw);
  switch (_kind) {
    case FG_DISC:
      CGContextSetStrokeColorWithColor(c, op(0.25).CGColor);
      for (int i = 0; i < 2; i++) {
        double a = (45 + 90 * i) * M_PI / 180;
        CGFloat dx = cos(a) * s * 0.45, dy = sin(a) * s * 0.45;
        CGContextMoveToPoint(c, cx - dx, cy - dy);
        CGContextAddLineToPoint(c, cx + dx, cy + dy);
        CGContextStrokePath(c);
      }
      CGContextSetFillColorWithColor(c, op(0.35).CGColor);
      CGContextFillEllipseInRect(c, CGRectMake(cx - s * 0.032, cy - s * 0.032, s * 0.064, s * 0.064));
      for (int i = 0; i < 4; i++) arrow(c, cx, cy, s * 0.36, i * 90, s * 0.105, op(1));
      break;
    case FG_FOUR: {
      CGFloat off = s / 2 - s * 0.173 - s * 0.013;
      for (int i = 0; i < 4; i++) arrow(c, cx, cy, off, i * 90, s * 0.115, op(1));
      break;
    }
    case FG_EIGHT:
      CGContextSetStrokeColorWithColor(c, op(0.22).CGColor);
      for (int i = 0; i < 8; i++) {
        double a = (22.5 + i * 45) * M_PI / 180;
        CGContextMoveToPoint(c, cx + cos(a) * s * 0.14, cy + sin(a) * s * 0.14);
        CGContextAddLineToPoint(c, cx + cos(a) * s * 0.47, cy + sin(a) * s * 0.47);
        CGContextStrokePath(c);
      }
      CGContextSetStrokeColorWithColor(c, op(0.3).CGColor);
      CGContextStrokeEllipseInRect(c, CGRectMake(cx - s * 0.13, cy - s * 0.13, s * 0.26, s * 0.26));
      for (int i = 0; i < 8; i++) {
        BOOL main = i % 2 == 0;
        arrow(c, cx, cy, s * 0.36, i * 45, main ? s * 0.105 : s * 0.07, op(main ? 1 : 0.75));
      }
      break;
  }
}
@end

static const int kErodeSteps = 16;
static const CGFloat kShadowDp = 1.5, kShadowDyDp = 0.8, kTextGlowDp = 2.5;

CGFloat PadRenderPad(PadStyle* st) {
  CGFloat extra = fmax(st.glowDp, st.contentEffect == PAD_EFFECT_NONE ? 0 : kTextGlowDp);
  return ceil(st.borderDp + extra * 2.5) + 2;
}

typedef struct { CGContextRef c; CGFloat w, h; } Canvas;

static Canvas mkCanvas(CGFloat w, CGFloat h, CGFloat scale) {
  size_t W = (size_t)ceil(w * scale), H = (size_t)ceil(h * scale);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef c = CGBitmapContextCreate(NULL, W, H, 8, 0, cs, kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(cs);
  CGContextTranslateCTM(c, 0, H);
  CGContextScaleCTM(c, scale, -scale);
  return (Canvas){c, w, h};
}

static CGImageRef take(Canvas k) {
  CGImageRef img = CGBitmapContextCreateImage(k.c);
  CGContextRelease(k.c);
  return img;
}

static void drawFull(Canvas k, CGImageRef img, CGFloat dx, CGFloat dy) {
  CGContextSaveGState(k.c);
  CGContextTranslateCTM(k.c, dx, dy + k.h);
  CGContextScaleCTM(k.c, 1, -1);
  CGContextDrawImage(k.c, CGRectMake(0, 0, k.w, k.h), img);
  CGContextRestoreGState(k.c);
}

static CGImageRef tinted(CGImageRef shape, uint32_t color, CGFloat w, CGFloat h, CGFloat scale) {
  Canvas t = mkCanvas(w, h, scale);
  drawFull(t, shape, 0, 0);
  CGContextSetBlendMode(t.c, kCGBlendModeSourceIn);
  CGContextSetFillColorWithColor(t.c, PadColor(color).CGColor);
  CGContextFillRect(t.c, CGRectMake(0, 0, w, h));
  return take(t);
}

static void setShadow(CGContextRef c, uint32_t color, CGFloat radiusDp, CGFloat dyDp, CGFloat scale) {
  CGContextSetShadowWithColor(c, CGSizeMake(0, -dyDp * scale), (1.15 * radiusDp + 1) * scale, PadColor(color).CGColor);
}

UIImage* PadRender(PadShape* bg, PadForeground* fg, CGFloat fgFraction, NSString* text, CGSize size, CGFloat scale,
                   PadStyle* st, BOOL knob) {
  CGFloat pad = PadRenderPad(st), w = fmax(1, size.width), h = fmax(1, size.height);
  CGFloat bw = w + 2 * pad, bh = h + 2 * pad;
  CGRect bounds = CGRectMake(pad, pad, w, h);
  Canvas out = mkCanvas(bw, bh, scale);
  CGFloat border = st.borderDp;

  if (bg) {
    Canvas sk = mkCanvas(bw, bh, scale);
    [bg fill:sk.c rect:bounds];
    CGImageRef shape = take(sk);
    CGImageRef fill = tinted(shape, knob ? st.knobFillColor : st.fillColor, bw, bh, scale);
    drawFull(out, fill, 0, 0);
    CGImageRelease(fill);
    if (border > 0) {

      Canvas ek = mkCanvas(bw, bh, scale);
      drawFull(ek, shape, 0, 0);
      CGContextSetBlendMode(ek.c, kCGBlendModeDestinationIn);
      for (int i = 0; i < kErodeSteps; i++) {
        double a = M_PI * 2 * i / kErodeSteps;
        drawFull(ek, shape, border * cos(a), border * sin(a));
      }
      CGImageRef eroded = take(ek);
      Canvas rk = mkCanvas(bw, bh, scale);
      drawFull(rk, shape, 0, 0);
      CGContextSetBlendMode(rk.c, kCGBlendModeDestinationOut);
      drawFull(rk, eroded, 0, 0);
      CGImageRef ring = take(rk);
      CGImageRef ringT = tinted(ring, st.borderColor, bw, bh, scale);
      if (st.glowDp > 0) {
        CGContextSaveGState(out.c);
        setShadow(out.c, st.borderColor, st.glowDp, 0, scale);
        drawFull(out, ringT, 0, 0);
        drawFull(out, ringT, 0, 0);
        CGContextRestoreGState(out.c);
      }
      drawFull(out, ringT, 0, 0);
      CGImageRelease(eroded);
      CGImageRelease(ring);
      CGImageRelease(ringT);
    }
    [bg drawDetails:out.c rect:bounds border:border > 0 ? st.borderColor : 0x40FFFFFF width:border > 0 ? border : 1];
    CGImageRelease(shape);
  }

  CGRect area = bg ? [bg contentRect:bounds] : bounds;
  CGFloat aw = fmax(1, area.size.width), ah = fmax(1, area.size.height);

  if (fg) {
    CGFloat fw = round(aw * fgFraction), fh = round(ah * fgFraction);
    if (fgFraction < 1) fw = fh = fmin(fw, fh);
    CGRect fr = CGRectMake(round(CGRectGetMidX(area) - fw / 2), round(CGRectGetMidY(area) - fh / 2), fw, fh);
    CGContextSaveGState(out.c);
    if (st.contentEffect == PAD_EFFECT_GLOW) setShadow(out.c, st.effectColor, kTextGlowDp, 0, scale);
    else if (st.contentEffect == PAD_EFFECT_SHADOW) setShadow(out.c, st.effectColor, kShadowDp, kShadowDyDp, scale);
    CGContextBeginTransparencyLayer(out.c, NULL);
    [fg draw:out.c rect:fr color:PadColor(st.contentColor)];
    CGContextEndTransparencyLayer(out.c);
    CGContextRestoreGState(out.c);
  }

  if (text.length) {
    CGFloat sz = fmax(8, round(ah * 0.42));
    UIFont* font = [UIFont boldSystemFontOfSize:sz];
    NSDictionary* at = @{NSFontAttributeName : font};
    while (sz > 2 && (font.lineHeight > ah * 0.85 || [text sizeWithAttributes:at].width > aw * 0.85)) {
      sz -= 2;
      font = [UIFont boldSystemFontOfSize:sz];
      at = @{NSFontAttributeName : font};
    }
    at = @{NSFontAttributeName : font, NSForegroundColorAttributeName : PadColor(st.contentColor)};
    CGFloat tw = [text sizeWithAttributes:at].width;

    CGPoint p = CGPointMake(CGRectGetMidX(area) - tw / 2, CGRectGetMidY(area) - font.ascender / 2);
    UIGraphicsPushContext(out.c);
    CGContextSaveGState(out.c);
    if (st.contentEffect == PAD_EFFECT_GLOW) setShadow(out.c, st.effectColor, kTextGlowDp, 0, scale);
    else if (st.contentEffect == PAD_EFFECT_SHADOW) setShadow(out.c, st.effectColor, kShadowDp, kShadowDyDp, scale);
    [text drawAtPoint:p withAttributes:at];
    CGContextRestoreGState(out.c);
    UIGraphicsPopContext();
  }

  CGImageRef img = take(out);
  UIImage* ui = [UIImage imageWithCGImage:img scale:scale orientation:UIImageOrientationUp];
  CGImageRelease(img);
  return ui;
}
