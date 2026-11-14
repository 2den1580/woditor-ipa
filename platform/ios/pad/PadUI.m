#import "PadUI.h"

#include <zlib.h>
#import <objc/runtime.h>

static BOOL isRu(void) { return [NSLocale.preferredLanguages.firstObject hasPrefix:@"ru"]; }

NSString* PadStr(NSString* key) {
  static NSDictionary *ru, *en;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    ru = @{
      @"cancel" : @"Отмена", @"Saves" : @"Сохранено!", @"delete" : @"Удалить", @"Create_title" : @"Настройки новой кнопки",
      @"Create_Form" : @"Форма кнопки:", @"Create" : @"Создать", @"pad_cat_button" : @"КНОПКА", @"pad_cat_dpad" : @"КРЕСТОВИНА",
      @"pad_crosshair_new_control" : @"Новое управление:", @"pad_crosshair_color" : @"Цвет (HEX):",
      @"pad_crosshair_size" : @"Размер (6-24)", @"pad_crosshair_thickness" : @"Толщина (1-4)",
      @"pad_crosshair_opacity" : @"Видимость (0.0-1.0)", @"pad_crosshair_title" : @"Прицел", @"pad_save" : @"Сохранить",
      @"pad_save_error" : @"Ошибка сохранения настроек", @"pad_stats_waiting" : @"Ожидание данных...",
      @"pad_stats_hint" : @"Двойной тап — компактный вид", @"pad_style_title" : @"Оформление кнопок",
      @"pad_style_dark" : @"Тёмные", @"pad_style_glass" : @"Стекло", @"pad_style_light" : @"Светлое стекло",
      @"pad_style_neon" : @"Неон", @"pad_style_neon_color" : @"Цвет неона:", @"pad_shape_circle" : @"Круг",
      @"pad_shape_square" : @"Квадрат", @"pad_shape_rectangle" : @"Прямоуг.", @"pad_shape_pill" : @"Капсула",
      @"pad_shape_hexagon" : @"Шестиуг.", @"pad_shape_keycap" : @"Клавиша", @"pad_shape_shoulder_l" : @"Плечо L",
      @"pad_shape_shoulder_r" : @"Плечо R", @"pad_dpad_cross" : @"Крест", @"pad_dpad_petals" : @"Лепестки",
      @"pad_dpad_disc" : @"Диск", @"pad_dpad_four" : @"4 кнопки", @"pad_dpad_eight" : @"8 направл.",
      @"pad_dpad_stick" : @"Стик", @"pad_dpad_float" : @"Плав. стик", @"pad_key" : @"Клавиша:", @"pad_icon" : @"Значок:",
      @"pad_toggle" : @"Удержание", @"pad_toggle_hint" : @"Нажал — держится, ещё раз — отпущена",
      @"pad_dpad_hint" : @"Шлёт стрелки ↑ ↓ ← →", @"pad_dpad_kind" : @"Вид:",
      @"pad_edit_hint" : @"Режим правки\n\n• Перетаскивайте кнопки\n• Зажмите кнопку — настройки и удаление\n• Зелёная кнопка — добавить, оформление, пресеты, сохранить и выйти",
      @"pad_editor_title" : @"Редактор кнопок", @"pad_editor_subtitle" : @"Перетаскивайте кнопки. Зажмите — настройки кнопки",
      @"pad_editor_add" : @"Добавить кнопку", @"pad_editor_style" : @"Оформление", @"pad_editor_presets" : @"Пресеты",
      @"pad_editor_done" : @"Сохранить и выйти", @"pad_presets_title" : @"Пресеты",
      @"pad_presets_subtitle" : @"Раскладка, значки и оформление. Можно поделиться файлом .torypriset",
      @"pad_preset_save_current" : @"Сохранить текущий", @"pad_preset_import" : @"Импорт файла",
      @"pad_preset_standard" : @"Стандартный", @"pad_preset_standard_meta" : @"Раскладка игры", @"pad_preset_meta" : @"Кнопок: %d",
      @"pad_preset_apply" : @"Применить", @"pad_preset_name_title" : @"Название пресета",
      @"pad_preset_name_hint" : @"Латиница, цифры, - и _", @"pad_preset_apply_confirm" : @"Заменить текущую раскладку на «%@»?",
      @"pad_preset_delete_confirm" : @"Удалить пресет «%@»?", @"pad_preset_overwrite" : @"Пресет «%@» уже есть. Заменить?",
      @"pad_preset_saved" : @"Сохранено: %@", @"pad_preset_imported" : @"Добавлен: %@", @"pad_preset_exported" : @"Сохранено: %@",
      @"pad_preset_bad_file" : @"Это не файл .torypriset", @"pad_preset_empty" : @"Сохранённых пресетов пока нет", @"pad_ok" : @"OK",
      @"pad_icon_added" : @"Значок добавлен", @"pad_icon_bad" : @"Не удалось прочитать картинку",
      @"pad_icon_delete_confirm" : @"Удалить этот значок?"
    };
    en = @{
      @"cancel" : @"Cancel", @"Saves" : @"Saved!", @"delete" : @"Delete", @"Create_title" : @"New button settings",
      @"Create_Form" : @"Button shape:", @"Create" : @"Create", @"pad_cat_button" : @"BUTTON", @"pad_cat_dpad" : @"D-PAD",
      @"pad_crosshair_new_control" : @"New control:", @"pad_crosshair_color" : @"Color (HEX):",
      @"pad_crosshair_size" : @"Size (6-24)", @"pad_crosshair_thickness" : @"Thickness (1-4)",
      @"pad_crosshair_opacity" : @"Opacity (0.0-1.0)", @"pad_crosshair_title" : @"Crosshair", @"pad_save" : @"Save",
      @"pad_save_error" : @"Error saving settings", @"pad_stats_waiting" : @"Waiting for data...",
      @"pad_stats_hint" : @"Double-tap for compact view", @"pad_style_title" : @"Button style", @"pad_style_dark" : @"Dark",
      @"pad_style_glass" : @"Glass", @"pad_style_light" : @"Light glass", @"pad_style_neon" : @"Neon",
      @"pad_style_neon_color" : @"Neon color:", @"pad_shape_circle" : @"Circle", @"pad_shape_square" : @"Square",
      @"pad_shape_rectangle" : @"Rectangle", @"pad_shape_pill" : @"Pill", @"pad_shape_hexagon" : @"Hexagon",
      @"pad_shape_keycap" : @"Key", @"pad_shape_shoulder_l" : @"Shoulder L", @"pad_shape_shoulder_r" : @"Shoulder R",
      @"pad_dpad_cross" : @"Cross", @"pad_dpad_petals" : @"Petals", @"pad_dpad_disc" : @"Disc", @"pad_dpad_four" : @"4 buttons",
      @"pad_dpad_eight" : @"8-way", @"pad_dpad_stick" : @"Stick", @"pad_dpad_float" : @"Floating stick", @"pad_key" : @"Key:",
      @"pad_icon" : @"Icon:", @"pad_toggle" : @"Hold (toggle)", @"pad_toggle_hint" : @"Tap — held, tap again — released",
      @"pad_dpad_hint" : @"Sends arrow keys ↑ ↓ ← →", @"pad_dpad_kind" : @"Type:",
      @"pad_edit_hint" : @"Edit mode\n\n• Drag buttons to move them\n• Long press a button — settings and delete\n• Green button — add, style, presets, save & exit",
      @"pad_editor_title" : @"Button editor", @"pad_editor_subtitle" : @"Drag buttons. Long press — button settings",
      @"pad_editor_add" : @"Add button", @"pad_editor_style" : @"Style", @"pad_editor_presets" : @"Presets",
      @"pad_editor_done" : @"Save & exit", @"pad_presets_title" : @"Presets",
      @"pad_presets_subtitle" : @"Layout, icons and style. Share as a .torypriset file",
      @"pad_preset_save_current" : @"Save current", @"pad_preset_import" : @"Import file", @"pad_preset_standard" : @"Standard",
      @"pad_preset_standard_meta" : @"From the game", @"pad_preset_meta" : @"%d buttons", @"pad_preset_apply" : @"Apply",
      @"pad_preset_name_title" : @"Preset name", @"pad_preset_name_hint" : @"Latin letters, digits, - and _",
      @"pad_preset_apply_confirm" : @"Replace the current layout with «%@»?", @"pad_preset_delete_confirm" : @"Delete preset «%@»?",
      @"pad_preset_overwrite" : @"Preset «%@» already exists. Replace?", @"pad_preset_saved" : @"Saved: %@",
      @"pad_preset_imported" : @"Added: %@", @"pad_preset_exported" : @"Saved: %@", @"pad_preset_bad_file" : @"Not a .torypriset file",
      @"pad_preset_empty" : @"No saved presets yet", @"pad_ok" : @"OK", @"pad_icon_added" : @"Icon added",
      @"pad_icon_bad" : @"Cannot read the image", @"pad_icon_delete_confirm" : @"Delete this icon?"
    };
  });
  NSString* s = (isRu() ? ru : en)[key];
  return s ?: (en[key] ?: key);
}

static NSArray<NSString*>* g_gameDirs;
static NSMutableDictionary<NSString*, PadForeground*>* g_iconCache;
#define kBaseIcons (@[ @"ic_check", @"ic_close", @"ic_menu", @"save", @"ic_bag", @"ic_shift", @"ic_fast" ])
#define kExts (@[ @"png", @"webp", @"jpg", @"jpeg", @"gif", @"bmp" ])

void PadIconsSetGameDirs(NSArray<NSString*>* dirs) {
  g_gameDirs = dirs;
  [g_iconCache removeAllObjects];
}

NSString* PadIconsUserDir(void) {
  NSString* s = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  NSString* d = [s stringByAppendingPathComponent:@"pad_icons"];
  [NSFileManager.defaultManager createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil error:nil];
  return d;
}

static NSString* userIconPath(NSString* name) {
  return [PadIconsUserDir() stringByAppendingPathComponent:[name stringByAppendingPathExtension:@"png"]];
}

static NSString* gameIconPath(NSString* name) {
  for (NSString* d in g_gameDirs)
    for (NSString* e in kExts) {
      NSString* p = [[d stringByAppendingPathComponent:@"pad_icons"] stringByAppendingPathComponent:[name stringByAppendingPathExtension:e]];
      if ([NSFileManager.defaultManager fileExistsAtPath:p]) return p;
    }
  return nil;
}

static BOOL isMonochrome(UIImage* img) {
  const int N = 24;
  uint8_t px[N * N * 4];
  memset(px, 0, sizeof px);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef c = CGBitmapContextCreate(px, N, N, 8, N * 4, cs, kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(cs);
  CGContextDrawImage(c, CGRectMake(0, 0, N, N), img.CGImage);
  CGContextRelease(c);
  int colored = 0, opaque = 0;
  for (int i = 0; i < N * N; i++) {
    uint8_t* p = px + i * 4;
    if (p[3] < 64) continue;
    opaque++;
    int mx = MAX(p[0], MAX(p[1], p[2])), mn = MIN(p[0], MIN(p[1], p[2]));
    if (mx - mn > 40) colored++;
  }
  return opaque > 0 && colored * 10 < opaque;
}

PadForeground* PadIconFG(NSString* name) {
  if (!name.length) return nil;
  if (!g_iconCache) g_iconCache = [NSMutableDictionary new];
  PadForeground* f = g_iconCache[name];
  if (f) return f;
  NSString* p = [NSFileManager.defaultManager fileExistsAtPath:userIconPath(name)] ? userIconPath(name) : gameIconPath(name);
  UIImage* img = p ? [UIImage imageWithContentsOfFile:p] : nil;
  f = img ? [PadForeground image:img tint:isMonochrome(img)] : [PadForeground vector:name];
  if (f) g_iconCache[name] = f;
  return f;
}

NSArray<NSString*>* PadIconList(void) {
  NSMutableOrderedSet* s = [NSMutableOrderedSet new];
  for (NSString* n in kBaseIcons)
    if (PadVecFind(n)) [s addObject:n];
  NSFileManager* fm = NSFileManager.defaultManager;
  for (NSString* d in g_gameDirs)
    for (NSString* f in [[fm contentsOfDirectoryAtPath:[d stringByAppendingPathComponent:@"pad_icons"] error:nil]
             sortedArrayUsingSelector:@selector(compare:)])
      if ([kExts containsObject:f.pathExtension.lowercaseString]) [s addObject:f.stringByDeletingPathExtension];
  for (NSString* f in [[fm contentsOfDirectoryAtPath:PadIconsUserDir() error:nil] sortedArrayUsingSelector:@selector(compare:)])
    if ([f.pathExtension isEqualToString:@"png"]) [s addObject:f.stringByDeletingPathExtension];
  return s.array;
}

BOOL PadIconIsUser(NSString* name) {
  return name && [NSFileManager.defaultManager fileExistsAtPath:userIconPath(name)];
}

static UIImage* squareIcon(UIImage* src) {
  CGImageRef cg = src.CGImage;
  if (!cg) return nil;
  size_t W = CGImageGetWidth(cg), H = CGImageGetHeight(cg);
  if (!W || !H || W > 8192 || H > 8192) return nil;
  uint8_t* px = calloc(W * H, 4);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef c = CGBitmapContextCreate(px, W, H, 8, W * 4, cs, kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(cs);
  CGContextDrawImage(c, CGRectMake(0, 0, W, H), cg);
  CGContextRelease(c);
  size_t x0 = W, y0 = H, x1 = 0, y1 = 0;
  for (size_t y = 0; y < H; y++)
    for (size_t x = 0; x < W; x++)
      if (px[(y * W + x) * 4 + 3] > 8) {
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
      }
  free(px);
  if (x0 > x1) { x0 = 0; y0 = 0; x1 = W - 1; y1 = H - 1; }
  CGImageRef crop = CGImageCreateWithImageInRect(cg, CGRectMake(x0, y0, x1 - x0 + 1, y1 - y0 + 1));
  CGFloat cw = CGImageGetWidth(crop), ch = CGImageGetHeight(crop), S = 384, k = S / fmax(cw, ch);
  UIGraphicsImageRendererFormat* fmt = [UIGraphicsImageRendererFormat defaultFormat];
  fmt.scale = 1;
  fmt.opaque = NO;
  UIImage* out = [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(S, S) format:fmt]
      imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
        [[UIImage imageWithCGImage:crop] drawInRect:CGRectMake((S - cw * k) / 2, (S - ch * k) / 2, cw * k, ch * k)];
      }];
  CGImageRelease(crop);
  return out;
}

NSString* PadIconImport(UIImage* img) {
  UIImage* sq = squareIcon(img);
  NSData* png = sq ? UIImagePNGRepresentation(sq) : nil;
  if (!png) return nil;
  NSString* name = [NSString stringWithFormat:@"icon_%lld", (long long)(NSDate.date.timeIntervalSince1970 * 1000)];
  [png writeToFile:userIconPath(name) atomically:YES];
  [g_iconCache removeObjectForKey:name];
  return name;
}

void PadIconImportPng(NSString* name, NSData* png) {
  NSString* clean = [[name componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/\\"]]
      componentsJoinedByString:@"_"];
  if (!clean.length || !png.length) return;
  [png writeToFile:userIconPath(clean) atomically:YES];
  [g_iconCache removeObjectForKey:clean];
}

NSData* PadIconExportPng(NSString* name) {
  NSString* p = PadIconIsUser(name) ? userIconPath(name) : gameIconPath(name);
  if (!p) return nil;
  if ([p.pathExtension.lowercaseString isEqualToString:@"png"]) return [NSData dataWithContentsOfFile:p];
  UIImage* img = [UIImage imageWithContentsOfFile:p];
  return img ? UIImagePNGRepresentation(img) : nil;
}

void PadIconDelete(NSString* name) {
  [NSFileManager.defaultManager removeItemAtPath:userIconPath(name) error:nil];
  [g_iconCache removeObjectForKey:name];
}

UIImage* PadIconTileImage(NSString* name, CGFloat size, uint32_t color) {
  PadForeground* f = PadIconFG(name);
  if (!f) return nil;
  UIGraphicsImageRenderer* r = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(size, size)];
  return [r imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
    CGContextBeginTransparencyLayer(ctx.CGContext, NULL);
    [f draw:ctx.CGContext rect:CGRectMake(0, 0, size, size) color:PadColor(color)];
    CGContextEndTransparencyLayer(ctx.CGContext);
  }];
}

NSArray<NSString*>* PadButtonShapes(void) {
  return @[ @"circle", @"square", @"rectangle", @"pill", @"hexagon", @"keycap", @"shoulder_l", @"shoulder_r" ];
}
NSArray<NSString*>* PadDpadShapes(void) {
  return @[ @"dpad", @"dpad_1", @"dpad_disc", @"dpad_four", @"dpad_8", @"stick", @"stick_float" ];
}

NSString* PadShapeTitle(NSString* shape) {
  NSDictionary* m = @{
    @"circle" : @"pad_shape_circle", @"square" : @"pad_shape_square", @"rectangle" : @"pad_shape_rectangle",
    @"pill" : @"pad_shape_pill", @"hexagon" : @"pad_shape_hexagon", @"keycap" : @"pad_shape_keycap",
    @"shoulder_l" : @"pad_shape_shoulder_l", @"shoulder_r" : @"pad_shape_shoulder_r", @"dpad" : @"pad_dpad_cross",
    @"dpad_1" : @"pad_dpad_petals", @"dpad_disc" : @"pad_dpad_disc", @"dpad_four" : @"pad_dpad_four",
    @"dpad_8" : @"pad_dpad_eight", @"stick" : @"pad_dpad_stick", @"stick_float" : @"pad_dpad_float"
  };
  NSString* k = m[PadShapeNormalize(shape)];
  return k ? PadStr(k) : shape;
}

UIImage* PadPreview(NSString* shape, NSString* icon, NSString* text, PadStyle* st, CGSize box) {
  shape = PadShapeNormalize(shape);
  CGSize natural = PadShapeSize(shape);
  BOOL floating = [shape isEqualToString:@"stick_float"];
  if (floating) natural = CGSizeMake(natural.height, natural.height);
  CGFloat margin = floating ? box.height * 0.18 : 0;
  CGFloat f0 = fmin((box.width - 2 * margin) / natural.width, (box.height - 2 * margin) / natural.height);
  CGFloat pad = PadRenderPad(st) * fmax(0.15, f0);
  CGFloat f = fmin((box.width - 2 * (pad + margin)) / natural.width, (box.height - 2 * (pad + margin)) / natural.height);
  if (f <= 0) f = f0 * 0.5;

  CGFloat scale = UIScreen.mainScreen.scale * f;
  CGSize sz = natural;
  UIGraphicsImageRenderer* r = [[UIGraphicsImageRenderer alloc] initWithSize:box];
  return [r imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
    if (floating) {
      UIBezierPath* p = [UIBezierPath bezierPathWithRoundedRect:CGRectInset(CGRectMake(0, 0, box.width, box.height), 1, 1)
                                                   cornerRadius:6];
      CGFloat dash[] = {4, 3};
      [p setLineDash:dash count:2 phase:0];
      [PadColor(0x80FFFFFF) setStroke];
      [p stroke];
    }
    CGFloat w = sz.width * f, h = sz.height * f, P = PadRenderPad(st) * f;
    CGRect dst = CGRectMake((box.width - w) / 2 - P, (box.height - h) / 2 - P, w + 2 * P, h + 2 * P);
    if (PadShapeViewKind(shape) == PAD_VIEW_STICK) {
      UIImage* base = PadRender(PadShape.circle, nil, 1, nil, sz, scale, st, NO);
      [base drawInRect:dst];
      CGSize ks = CGSizeMake(sz.width * 0.45, sz.width * 0.45);
      UIImage* knob = PadRender(PadShape.circle, nil, 1, nil, ks, scale, st, YES);
      CGFloat kw = ks.width * f;
      CGRect kd = CGRectMake(box.width / 2 + w * 0.12 - kw / 2 - P, box.height / 2 - h * 0.1 - kw / 2 - P, kw + 2 * P, kw + 2 * P);
      [knob drawInRect:kd];
      return;
    }
    PadShape* bg = [PadShape backgroundFor:shape];
    PadForeground* fg = [PadForeground foregroundFor:shape];
    CGFloat fraction = 1;
    NSString* label = nil;
    if (PadShapeViewKind(shape) == PAD_VIEW_BUTTON) {
      fg = PadIconFG(icon);
      fraction = 0.6;
      if (!fg) label = text;
    }
    [PadRender(bg, fg, fraction, label, sz, scale, st, NO) drawInRect:dst];
  }];
}

NSArray<NSString*>* PadKeyNames(void) {
  static NSArray* names;
  if (!names) {
    NSMutableArray* a = [@[ @"LMB", @"RMB", @"MMB" ] mutableCopy];
    for (int i = 0; i <= 9; i++) [a addObject:[NSString stringWithFormat:@"%d", i]];
    for (char c = 'A'; c <= 'Z'; c++) [a addObject:[NSString stringWithFormat:@"%c", c]];
    [a addObjectsFromArray:@[ @"ALT_LEFT", @"ALT_RIGHT", @"SHIFT_LEFT", @"SHIFT_RIGHT", @"TAB", @"SPACE", @"ENTER",
                              @"ESCAPE", @"DEL", @"PAGE_UP", @"PAGE_DOWN", @"CTRL_LEFT", @"CTRL_RIGHT" ]];
    for (int i = 1; i <= 12; i++) [a addObject:[NSString stringWithFormat:@"F%d", i]];
    names = a;
  }
  return names;
}

static NSDictionary<NSString*, NSNumber*>* keyTable(void) {
  static NSDictionary* t;
  if (!t) {
    NSMutableDictionary* m = [@{
      @"LMB" : @325, @"RMB" : @326, @"MMB" : @327, @"ALT_LEFT" : @57, @"ALT_RIGHT" : @58, @"SHIFT_LEFT" : @59,
      @"SHIFT_RIGHT" : @60, @"TAB" : @61, @"SPACE" : @62, @"ENTER" : @66, @"ESCAPE" : @111, @"DEL" : @67,
      @"PAGE_UP" : @92, @"PAGE_DOWN" : @93, @"CTRL_LEFT" : @113, @"CTRL_RIGHT" : @114, @"DPAD_UP" : @19,
      @"DPAD_DOWN" : @20, @"DPAD_LEFT" : @21, @"DPAD_RIGHT" : @22, @"FORWARD_DEL" : @112, @"MOVE_HOME" : @122,
      @"MOVE_END" : @123, @"INSERT" : @124, @"BACK" : @4
    } mutableCopy];
    for (int i = 0; i <= 9; i++) m[[NSString stringWithFormat:@"%d", i]] = @(7 + i);
    for (int c = 0; c < 26; c++) m[[NSString stringWithFormat:@"%c", 'A' + c]] = @(29 + c);
    for (int i = 1; i <= 12; i++) m[[NSString stringWithFormat:@"F%d", i]] = @(130 + i);
    t = m;
  }
  return t;
}

int PadKeyCode(NSString* name) { return [keyTable()[name.uppercaseString] intValue]; }

NSString* PadKeyName(int code) {
  for (NSString* k in keyTable())
    if ([keyTable()[k] intValue] == code) return k;
  return [NSString stringWithFormat:@"%d", code];
}

UIColor* PadUIColor(uint32_t c) { return PadColor(c); }

UILabel* PadLabel(NSString* text, CGFloat size, uint32_t color, BOOL bold) {
  UILabel* l = [UILabel new];
  l.text = text;
  l.textColor = PadColor(color);
  l.font = bold ? [UIFont boldSystemFontOfSize:size] : [UIFont systemFontOfSize:size];
  l.numberOfLines = 0;
  return l;
}

@interface PadBlockTarget : NSObject
@property(nonatomic, copy) void (^block)(void);
- (void)fire;
@end
@implementation PadBlockTarget
- (void)fire {
  if (_block) _block();
}
@end

static char kTargetKey;

UIButton* PadButton(NSString* title, uint32_t color, void (^action)(void)) {
  UIButton* b = [UIButton buttonWithType:UIButtonTypeSystem];
  [b setTitle:title forState:UIControlStateNormal];
  [b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
  b.titleLabel.font = [UIFont boldSystemFontOfSize:13];
  b.titleLabel.adjustsFontSizeToFitWidth = YES;
  b.titleLabel.minimumScaleFactor = 0.6;
  b.backgroundColor = PadColor(color);
  b.layer.cornerRadius = 8;
  b.contentEdgeInsets = UIEdgeInsetsMake(6, 10, 6, 10);
  if (action) {
    PadBlockTarget* t = [PadBlockTarget new];
    t.block = action;
    objc_setAssociatedObject(b, &kTargetKey, t, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [b addTarget:t action:@selector(fire) forControlEvents:UIControlEventTouchUpInside];
  }
  return b;
}

void PadCard(UIView* v, BOOL selected) {
  v.backgroundColor = PadColor(selected ? 0xFF3A3029 : 0xFF2A2A2D);
  v.layer.cornerRadius = 10;
  v.layer.borderWidth = selected ? 2 : 1;
  v.layer.borderColor = PadColor(selected ? 0xFFE28743 : 0xFF3A3A3E).CGColor;
}

static UIStackView* stack(NSArray<UIView*>* views, CGFloat spacing, UILayoutConstraintAxis axis) {
  UIStackView* s = [[UIStackView alloc] initWithArrangedSubviews:views];
  s.axis = axis;
  s.spacing = spacing;
  s.alignment = axis == UILayoutConstraintAxisHorizontal ? UIStackViewAlignmentCenter : UIStackViewAlignmentFill;
  return s;
}

UIView* PadHStack(NSArray<UIView*>* views, CGFloat spacing) { return stack(views, spacing, UILayoutConstraintAxisHorizontal); }
UIView* PadVStack(NSArray<UIView*>* views, CGFloat spacing) { return stack(views, spacing, UILayoutConstraintAxisVertical); }

UIView* PadFixed(UIView* v, CGFloat w, CGFloat h) {
  v.translatesAutoresizingMaskIntoConstraints = NO;
  if (w > 0) [v.widthAnchor constraintEqualToConstant:w].active = YES;
  if (h > 0) [v.heightAnchor constraintEqualToConstant:h].active = YES;
  return v;
}

void PadOnTap(UIView* v, void (^action)(void)) {
  PadBlockTarget* t = [PadBlockTarget new];
  t.block = action;
  objc_setAssociatedObject(v, &kTargetKey, t, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  v.userInteractionEnabled = YES;
  [v addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:t action:@selector(fire)]];
}

UIViewController* PadTopVC(UIView* any) {
  UIViewController* vc = any.window.rootViewController;
  while (vc.presentedViewController) vc = vc.presentedViewController;
  return vc;
}

void PadToast(UIView* host, NSString* text) {
  UILabel* l = PadLabel(text, 14, 0xFFFFFFFF, NO);
  l.textAlignment = NSTextAlignmentCenter;
  l.backgroundColor = PadColor(0xE61B1B1E);
  l.layer.cornerRadius = 10;
  l.layer.borderColor = PadColor(0xFF007ACC).CGColor;
  l.layer.borderWidth = 1;
  l.clipsToBounds = YES;
  l.userInteractionEnabled = NO;
  CGSize s = [l sizeThatFits:CGSizeMake(host.bounds.size.width * 0.7, 200)];
  l.frame = CGRectMake((host.bounds.size.width - s.width - 32) / 2, host.bounds.size.height - s.height - 56, s.width + 32, s.height + 16);
  [host addSubview:l];
  [UIView animateWithDuration:0.3 delay:2.2 options:0 animations:^{ l.alpha = 0; } completion:^(BOOL d) { [l removeFromSuperview]; }];
}

void PadConfirm(UIView* host, NSString* message, NSString* okTitle, void (^yes)(void)) {
  UIAlertController* a = [UIAlertController alertControllerWithTitle:nil message:message preferredStyle:UIAlertControllerStyleAlert];
  [a addAction:[UIAlertAction actionWithTitle:PadStr(@"cancel") style:UIAlertActionStyleCancel handler:nil]];
  [a addAction:[UIAlertAction actionWithTitle:okTitle ?: PadStr(@"pad_ok") style:UIAlertActionStyleDefault
                                      handler:^(UIAlertAction* x) { yes(); }]];
  [PadTopVC(host) presentViewController:a animated:YES completion:nil];
}

@implementation PadSheet {
  UIView* _box;
}

+ (PadSheet*)sheetIn:(UIView*)host title:(NSString*)title subtitle:(NSString*)sub width:(CGFloat)w {
  PadSheet* s = [[PadSheet alloc] initWithFrame:host.bounds];
  s.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  s.backgroundColor = PadColor(0x77000000);
  UIButton* dim = [UIButton buttonWithType:UIButtonTypeCustom];
  dim.frame = s.bounds;
  dim.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [dim addTarget:s action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
  [s addSubview:dim];

  UIView* box = [UIView new];
  box.backgroundColor = PadColor(0xFF1B1B1E);
  box.layer.cornerRadius = 16;
  box.layer.borderColor = PadColor(0xFF2F2F35).CGColor;
  box.layer.borderWidth = 1;
  box.translatesAutoresizingMaskIntoConstraints = NO;
  [s addSubview:box];
  s->_box = box;
  CGFloat width = fmin(w, host.bounds.size.width - 24);
  [NSLayoutConstraint activateConstraints:@[
    [box.centerXAnchor constraintEqualToAnchor:s.centerXAnchor], [box.centerYAnchor constraintEqualToAnchor:s.centerYAnchor],
    [box.widthAnchor constraintEqualToConstant:width],
    [box.heightAnchor constraintLessThanOrEqualToAnchor:s.safeAreaLayoutGuide.heightAnchor constant:-12]
  ]];

  UILabel* t = PadLabel(title, 17, 0xFFFFFFFF, YES);
  UILabel* st = sub ? PadLabel(sub, 12, 0xFF9A9AA2, NO) : nil;
  UIButton* x = [UIButton buttonWithType:UIButtonTypeSystem];
  [x setTitle:@"✕" forState:UIControlStateNormal];
  [x setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
  x.titleLabel.font = [UIFont systemFontOfSize:18];
  [x addTarget:s action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
  UIStackView* titles = (UIStackView*)PadVStack(st ? @[ t, st ] : @[ t ], 2);
  UIStackView* header = (UIStackView*)PadHStack(@[ titles, PadFixed(x, 36, 36) ], 8);
  header.alignment = UIStackViewAlignmentTop;
  header.translatesAutoresizingMaskIntoConstraints = NO;
  [box addSubview:header];

  UIScrollView* sc = [UIScrollView new];
  sc.translatesAutoresizingMaskIntoConstraints = NO;
  sc.showsHorizontalScrollIndicator = NO;
  [box addSubview:sc];
  UIStackView* content = (UIStackView*)PadVStack(@[], 8);
  content.translatesAutoresizingMaskIntoConstraints = NO;
  [sc addSubview:content];
  s->_content = content;
  NSLayoutConstraint* hug = [sc.heightAnchor constraintEqualToAnchor:content.heightAnchor];
  hug.priority = UILayoutPriorityDefaultLow;
  [NSLayoutConstraint activateConstraints:@[
    [header.topAnchor constraintEqualToAnchor:box.topAnchor constant:12],
    [header.leadingAnchor constraintEqualToAnchor:box.leadingAnchor constant:16],
    [header.trailingAnchor constraintEqualToAnchor:box.trailingAnchor constant:-8],
    [sc.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:10],
    [sc.leadingAnchor constraintEqualToAnchor:box.leadingAnchor constant:16],
    [sc.trailingAnchor constraintEqualToAnchor:box.trailingAnchor constant:-16],
    [sc.bottomAnchor constraintEqualToAnchor:box.bottomAnchor constant:-16],
    [content.topAnchor constraintEqualToAnchor:sc.contentLayoutGuide.topAnchor],
    [content.bottomAnchor constraintEqualToAnchor:sc.contentLayoutGuide.bottomAnchor],
    [content.leadingAnchor constraintEqualToAnchor:sc.contentLayoutGuide.leadingAnchor],
    [content.trailingAnchor constraintEqualToAnchor:sc.contentLayoutGuide.trailingAnchor],
    [content.widthAnchor constraintEqualToAnchor:sc.frameLayoutGuide.widthAnchor], hug
  ]];
  [host addSubview:s];
  return s;
}

- (void)close {
  [self removeFromSuperview];
  if (_onClose) _onClose();
}
@end

static void put16(NSMutableData* d, uint16_t v) { [d appendBytes:&v length:2]; }
static void put32(NSMutableData* d, uint32_t v) { [d appendBytes:&v length:4]; }

NSData* PadZip(NSDictionary<NSString*, NSData*>* files) {
  NSMutableData* out = [NSMutableData new];
  NSMutableData* cd = [NSMutableData new];
  uint16_t n = 0;
  for (NSString* name in [files.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
    NSData* data = files[name];
    NSData* nm = [name dataUsingEncoding:NSUTF8StringEncoding];
    uint32_t crc = (uint32_t)crc32(0, data.bytes, (uInt)data.length), off = (uint32_t)out.length;
    put32(out, 0x04034b50); put16(out, 20); put16(out, 0x0800); put16(out, 0); put16(out, 0); put16(out, 0x21);
    put32(out, crc); put32(out, (uint32_t)data.length); put32(out, (uint32_t)data.length);
    put16(out, (uint16_t)nm.length); put16(out, 0);
    [out appendData:nm];
    [out appendData:data];
    put32(cd, 0x02014b50); put16(cd, 20); put16(cd, 20); put16(cd, 0x0800); put16(cd, 0); put16(cd, 0); put16(cd, 0x21);
    put32(cd, crc); put32(cd, (uint32_t)data.length); put32(cd, (uint32_t)data.length);
    put16(cd, (uint16_t)nm.length); put16(cd, 0); put16(cd, 0); put16(cd, 0); put16(cd, 0); put32(cd, 0); put32(cd, off);
    [cd appendData:nm];
    n++;
  }
  uint32_t cdOff = (uint32_t)out.length;
  [out appendData:cd];
  put32(out, 0x06054b50); put16(out, 0); put16(out, 0); put16(out, n); put16(out, n);
  put32(out, (uint32_t)cd.length); put32(out, cdOff); put16(out, 0);
  return out;
}

static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t* p) { return (uint32_t)(p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24); }

NSDictionary<NSString*, NSData*>* PadUnzip(NSData* zip) {
  const uint8_t* b = zip.bytes;
  size_t len = zip.length;
  if (len < 22) return nil;
  ssize_t eocd = -1;
  for (ssize_t i = (ssize_t)len - 22; i >= 0 && i >= (ssize_t)len - 65557; i--)
    if (rd32(b + i) == 0x06054b50) { eocd = i; break; }
  if (eocd < 0) return nil;
  uint16_t count = rd16(b + eocd + 10);
  uint32_t cdOff = rd32(b + eocd + 16);
  NSMutableDictionary* out = [NSMutableDictionary new];
  size_t p = cdOff;
  for (int i = 0; i < count; i++) {
    if (p + 46 > len || rd32(b + p) != 0x02014b50) return nil;
    uint16_t method = rd16(b + p + 10), nl = rd16(b + p + 28), el = rd16(b + p + 30), cl = rd16(b + p + 32);
    uint32_t csize = rd32(b + p + 20), usize = rd32(b + p + 24), loff = rd32(b + p + 42);
    if (p + 46 + nl > len) return nil;
    NSString* name = [[NSString alloc] initWithBytes:b + p + 46 length:nl encoding:NSUTF8StringEncoding];
    p += 46 + nl + el + cl;
    if (loff + 30 > len || rd32(b + loff) != 0x04034b50) return nil;
    size_t data = loff + 30 + rd16(b + loff + 26) + rd16(b + loff + 28);
    if (data + csize > len || usize > 64u << 20) return nil;
    if (!name.length || [name hasSuffix:@"/"] || [name containsString:@".."]) continue;
    if (method == 0) {
      out[name] = [NSData dataWithBytes:b + data length:csize];
    } else if (method == 8) {
      NSMutableData* o = [NSMutableData dataWithLength:usize];
      z_stream z = {0};
      if (inflateInit2(&z, -15) != Z_OK) return nil;
      z.next_in = (Bytef*)(b + data);
      z.avail_in = csize;
      z.next_out = o.mutableBytes;
      z.avail_out = usize;
      int r = inflate(&z, Z_FINISH);
      inflateEnd(&z);
      if (r != Z_STREAM_END && !(r == Z_BUF_ERROR && z.avail_out == 0)) return nil;
      out[name] = o;
    }
  }
  return out;
}
