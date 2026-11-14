#import <AudioToolbox/AudioToolbox.h>
#import <GameController/GameController.h>
#import <OpenGLES/EAGL.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include "gl_api.h"
#include "host.h"
#import "pad/Pad.h"
#import "pad/PadGfx.h"

int plat_load_proc(void* (**getproc)(const char*)) { return 0; }

void audio_start(bool play);
void audio_set_paused(bool paused);
void audio_tick(void);

extern uint64_t wasm_rt_mmap32_max, wasm_rt_mmap32_headroom;

static EAGLContext* g_ctx;
static CAEAGLLayer* g_layer;
static GLuint g_fbo, g_rb;
static int g_win_w, g_win_h;
static volatile bool g_running, g_paused, g_pause_ack, g_resize = true, g_surface_ready;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static int g_game_w = 640, g_game_h = 480;
static int g_frames_loop, g_frames_shown, g_show_last;
static char g_root[1024], g_game[1024], g_save[1024], g_project[128];
static host_config g_cfg;
static volatile bool g_game_started;

@class WGameVC;
static NSString* docs_dir(void);
static WGameVC* g_vc;
static void on_main(dispatch_block_t b) { dispatch_async(dispatch_get_main_queue(), b); }

typedef struct {
  int kind;
  int a, b, c, d, e;
  char text[512];
} ev;
#define EVQ 1024
static ev g_evq[EVQ];
static int g_evhead, g_evtail;

static void push_ev(const ev* e) {
  pthread_mutex_lock(&g_lock);
  int next = (g_evtail + 1) % EVQ;
  if (next != g_evhead) {
    g_evq[g_evtail] = *e;
    g_evtail = next;
  }
  pthread_mutex_unlock(&g_lock);
}

void plat_pump_events(void) {
  for (;;) {
    ev e;
    pthread_mutex_lock(&g_lock);
    if (g_evhead == g_evtail) {
      pthread_mutex_unlock(&g_lock);
      break;
    }
    e = g_evq[g_evhead];
    g_evhead = (g_evhead + 1) % EVQ;
    pthread_mutex_unlock(&g_lock);
    switch (e.kind) {
      case 1: host_input_key(e.a, e.b, e.text, e.text, e.c, e.d, e.e); break;
      case 2: host_input_mouse(e.a, e.b, e.c, e.d, e.e); break;
      case 6: host_text_input_finish(e.text, e.a); break;
    }
  }
}

static void send_key(bool down, int dom) {
  if (!dom) return;
  ev e = {.kind = 1, .a = down, .b = dom};
  snprintf(e.text, sizeof e.text, "%d", dom);
  push_ev(&e);
}

static void send_mouse(int type, int x, int y, int button, int buttons) {
  ev e = {.kind = 2, .a = type, .b = x, .c = y, .d = button, .e = buttons};
  push_ev(&e);
}

static int dom_from_android(int k) {
  switch (k) {
    case 19: return 38;
    case 20: return 40;
    case 21: return 37;
    case 22: return 39;
    case 66: case 160: case 23: case 96: return 13;
    case 111: case 4: case 97: case 108: return 27;
    case 59: case 60: case 99: return 16;
    case 113: case 114: return 17;
    case 57: case 58: return 18;
    case 62: case 100: return 32;
    case 61: return 9;
    case 67: return 8;
    case 112: return 46;
    case 124: return 45;
    case 122: return 36;
    case 123: return 35;
    case 92: case 102: return 33;
    case 93: case 103: return 34;
    default:
      if (k >= 29 && k <= 54) return 65 + (k - 29);
      if (k >= 7 && k <= 16) return 48 + (k - 7);
      if (k >= 144 && k <= 153) return 96 + (k - 144);
      if (k >= 131 && k <= 142) return 112 + (k - 131);
      return 0;
  }
}

static int dom_from_hid(long u) {
  if (u >= 4 && u <= 29) return 65 + (int)(u - 4);
  if (u >= 30 && u <= 38) return 49 + (int)(u - 30);
  if (u == 39) return 48;
  if (u >= 58 && u <= 69) return 112 + (int)(u - 58);
  if (u >= 89 && u <= 97) return 97 + (int)(u - 89);
  if (u == 98) return 96;
  switch (u) {
    case 40: case 88: return 13;
    case 41: return 27;
    case 42: return 8;
    case 43: return 9;
    case 44: return 32;
    case 79: return 39;
    case 80: return 37;
    case 81: return 40;
    case 82: return 38;
    case 73: return 45;
    case 74: return 36;
    case 75: return 33;
    case 76: return 46;
    case 77: return 35;
    case 78: return 34;
    case 224: case 228: return 17;
    case 225: case 229: return 16;
    case 226: case 230: return 18;
    default: return 0;
  }
}

static void gl_bind_window(void) {
  if (!g_fbo) {
    glGenFramebuffers(1, &g_fbo);
    glGenRenderbuffers(1, &g_rb);
  }
  GLint prev_rb;
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &prev_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, g_rb);
  if (![g_ctx renderbufferStorage:GL_RENDERBUFFER fromDrawable:g_layer]) {
    host_log("window: renderbufferStorage failed");
    g_surface_ready = false;
    return;
  }
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &g_win_w);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &g_win_h);
  GLint prev_fb;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_rb);
  GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, prev_fb);
  glBindRenderbuffer(GL_RENDERBUFFER, prev_rb);
  host_gl_window_fbo = g_fbo;
  g_surface_ready = st == GL_FRAMEBUFFER_COMPLETE && g_win_w > 0 && g_win_h > 0;
  g_show_last = 3;
  host_log("window %dx%d%s", g_win_w, g_win_h, g_surface_ready ? "" : " (framebuffer incomplete)");
}

static void swap(void) {
  GLint prev_rb;
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &prev_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, g_rb);
  [g_ctx presentRenderbuffer:GL_RENDERBUFFER];
  glBindRenderbuffer(GL_RENDERBUFFER, prev_rb);
}

void plat_present(void) {
  if (!g_surface_ready || g_paused) return;
  host_gl_present(g_win_w, g_win_h);
  swap();
  g_frames_shown++;
}

static volatile int g_vsync_seq;
static volatile double g_vsync_ms;
static int g_vsync_seen;
static double g_vsync_prev_ms, g_vsync_period = 1000.0 / 60;
static pthread_mutex_t g_vmu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_vcond = PTHREAD_COND_INITIALIZER;

static void vsync_tick(double ts_ms) {
  pthread_mutex_lock(&g_vmu);
  g_vsync_ms = ts_ms;
  g_vsync_seq++;
  pthread_cond_signal(&g_vcond);
  pthread_mutex_unlock(&g_vmu);
}

static double wait_vsync(void) {
  pthread_mutex_lock(&g_vmu);
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_nsec += 100 * 1000000;
  if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
  while (g_vsync_seq == g_vsync_seen)
    if (pthread_cond_timedwait(&g_vcond, &g_vmu, &ts)) break;
  if (g_vsync_seq == g_vsync_seen) {
    pthread_mutex_unlock(&g_vmu);
    return -1;
  }
  int n = g_vsync_seq - g_vsync_seen;
  g_vsync_seen = g_vsync_seq;
  double t = g_vsync_ms;
  pthread_mutex_unlock(&g_vmu);
  double dt = g_vsync_prev_ms > 0 ? t - g_vsync_prev_ms : 0;
  g_vsync_prev_ms = t;
  if (n == 1 && dt > 4 && dt < 50) g_vsync_period += (dt - g_vsync_period) * 0.1;
  return dt;
}

static bool pacing_due(double iv, double* acc) {
  double dt = wait_vsync();
  if (dt < 0) dt = iv;
  *acc += dt;
  if (*acc < iv - g_vsync_period * 0.5) return false;
  *acc -= iv;
  if (*acc > iv) *acc = iv;
  if (*acc < -iv) *acc = 0;
  return true;
}

static void stats_cb(int loop, int shown);

static void handle_pause(void) {

  pthread_mutex_lock(&g_lock);
  if (g_paused && !g_pause_ack) {
    glFinish();
    g_pause_ack = true;
    pthread_cond_broadcast(&g_cond);
  }
  pthread_mutex_unlock(&g_lock);
}

static void* engine_thread(void* arg) {
  setpriority(PRIO_PROCESS, 0, -10);
  pthread_setname_np("woditor-engine");
  [EAGLContext setCurrentContext:g_ctx];
  while (g_running && (g_paused || !g_layer)) usleep(10000);
  if (g_resize) { g_resize = false; gl_bind_window(); }
  host_gl_init();
  audio_start(!g_paused);

  wasm_rt_mmap32_max = 0x80000000u;
  wasm_rt_mmap32_headroom = 0;
  g_cfg = (host_config){.root_dir = g_root, .game_dir = g_game, .project_id = g_project,
                        .location = "https://localhost/index.html", .lang = "ja_JP",
                        .game_pack = NULL, .game_pack_prefix = NULL, .cache_dir = NULL,
                        .save_dir = g_save, .mod_dirs = ""};
  host_run(&g_cfg);
  double stats_at = host_now_ms() + 1000, acc = 0;
  while (g_running) {
    handle_pause();
    if (g_paused) {
      usleep(16000);
      acc = 0;
      g_vsync_prev_ms = 0;
      continue;
    }
    if (g_resize) {
      g_resize = false;
      gl_bind_window();
    }
    double iv = host_main_loop_interval_ms();
    if (iv <= 0) iv = 1000.0 / 60;
    if (!pacing_due(iv, &acc)) continue;
    int shown = g_frames_shown;
    host_main_loop_iter();
    g_frames_loop++;
    if (g_show_last > 0) {
      g_show_last--;
      if (shown == g_frames_shown && g_surface_ready && !g_paused) {
        host_gl_present_last(g_win_w, g_win_h);
        swap();
      }
    }
    if (host_now_ms() >= stats_at) {
      stats_at = host_now_ms() + 1000;
      audio_tick();
      stats_cb(g_frames_loop, g_frames_shown);
      g_frames_loop = g_frames_shown = 0;
    }
  }
  host_log("engine thread finished");
  return NULL;
}

@interface WMouse : UIView
@property(nonatomic) BOOL crosshairMode, rightMode;
@property(nonatomic) CGPoint pos;
@property(nonatomic) BOOL visible;
- (void)touches:(NSSet<UITouch*>*)all phase:(int)phase changed:(NSSet<UITouch*>*)changed;
- (void)reloadConfig;
@end

static CGPoint to_game(CGPoint p);

@implementation WMouse {
  BOOL _holding, _multiRight, _pending;
  NSTimer *_longPress, *_autoClick;
  UIColor* _color;
  CGFloat _size, _thick, _opacity;
  NSInteger _fingers;
}

- (instancetype)initWithFrame:(CGRect)f {
  if ((self = [super initWithFrame:f])) {
    self.userInteractionEnabled = NO;
    self.backgroundColor = UIColor.clearColor;
    self.opaque = NO;
    [self reloadConfig];
  }
  return self;
}

- (void)reloadConfig {
  NSUserDefaults* d = NSUserDefaults.standardUserDefaults;
  _crosshairMode = [d objectForKey:@"GameSettings.crosshair_use_new"] ? [d boolForKey:@"GameSettings.crosshair_use_new"] : YES;
  NSString* hex = [d stringForKey:@"GameSettings.crosshair_color"] ?: @"#00FF00";
  NSString* h = [hex hasPrefix:@"#"] ? [hex substringFromIndex:1] : hex;
  unsigned long long v = 0;
  [[NSScanner scannerWithString:h] scanHexLongLong:&v];
  if (h.length == 6) v |= 0xFF000000ull;
  else if (h.length != 8) v = 0xFF00FF00ull;
  _color = PadColor((uint32_t)v);
  _size = [d objectForKey:@"GameSettings.crosshair_size"] ? [d integerForKey:@"GameSettings.crosshair_size"] : 24;
  _thick = [d objectForKey:@"GameSettings.crosshair_thickness"] ? [d integerForKey:@"GameSettings.crosshair_thickness"] : 4;
  _opacity = [d objectForKey:@"GameSettings.crosshair_opacity"] ? [d floatForKey:@"GameSettings.crosshair_opacity"] : 1;
  if (!_crosshairMode) _visible = NO;
  [self setNeedsDisplay];
}

- (void)setCrosshairMode:(BOOL)m {
  _crosshairMode = m;
  [NSUserDefaults.standardUserDefaults setBool:m forKey:@"GameSettings.crosshair_use_new"];
  if (!m) _visible = NO;
  [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)r {
  if (!_crosshairMode || !_visible) return;
  [[_color colorWithAlphaComponent:_opacity] setFill];
  UIRectFill(CGRectMake(_pos.x - _size / 2, _pos.y - _thick / 2, _size, _thick));
  UIRectFill(CGRectMake(_pos.x - _thick / 2, _pos.y - _size / 2, _thick, _size));
}

- (void)send:(int)type at:(CGPoint)p right:(BOOL)right {
  right |= _rightMode;
  int button = right ? 2 : 0;
  int buttons = type == 5 ? (right ? 2 : 1) : (type == 8 && _holding) ? (right ? 2 : 1) : 0;
  CGPoint g = to_game(p);
  send_mouse(type, (int)g.x, (int)g.y, button, buttons);
}

- (void)stopTimers {
  [_longPress invalidate];
  _longPress = nil;
  [_autoClick invalidate];
  _autoClick = nil;
}

- (void)moveTo:(CGPoint)p {
  _pos = p;
  _visible = YES;
  [self setNeedsDisplay];
}

- (void)touches:(NSSet<UITouch*>*)all phase:(int)phase changed:(NSSet<UITouch*>*)changed {
  UIView* v = self.superview;
  UITouch* first = changed.anyObject;
  CGPoint p = [first locationInView:v];
  if (!_crosshairMode) {
    int t = phase == 0 ? 5 : phase == 1 ? 8 : 6;
    CGPoint g = to_game(p);
    send_mouse(t, (int)g.x, (int)g.y, _rightMode ? 2 : 0, t == 6 ? 0 : (_rightMode ? 2 : 1));
    return;
  }
  NSInteger count = all.count;
  CGFloat radius = 10;
  if (phase == 0) {
    if (count >= 2) {
      [self stopTimers];
      _pending = NO;
      _holding = YES;
      _multiRight = YES;
      [self send:5 at:_visible ? _pos : p right:YES];
      return;
    }
    double dist = _visible ? hypot(p.x - _pos.x, p.y - _pos.y) : DBL_MAX;
    if (dist < radius) {
      _pending = YES;
      _multiRight = NO;
      __weak WMouse* ws = self;
      _longPress = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:NO block:^(NSTimer* t) {
        WMouse* s = ws;
        if (!s || !s->_pending || s->_multiRight) return;
        s->_pending = NO;
        s->_autoClick = [NSTimer scheduledTimerWithTimeInterval:0.1 repeats:YES block:^(NSTimer* t2) {
          WMouse* s2 = ws;
          if (!s2) return;
          CGPoint at = s2->_pos;
          [s2 send:5 at:at right:NO];
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 50 * NSEC_PER_MSEC), dispatch_get_main_queue(),
                         ^{ [ws send:6 at:at right:NO]; });
        }];
        [s->_autoClick fire];
      }];
    } else {
      [self moveTo:p];
      [self send:8 at:p right:NO];
    }
  } else if (phase == 1) {
    if (_multiRight) return;
    if (_pending || _autoClick) {
      if (hypot(p.x - _pos.x, p.y - _pos.y) > radius) {
        [self stopTimers];
        _pending = NO;
      }
    } else {
      [self moveTo:p];
      [self send:8 at:p right:NO];
    }
  } else {
    NSInteger left = count - changed.count;
    if (_multiRight) {
      if (left < 2) {
        [self send:6 at:_pos right:YES];
        _holding = NO;
        _multiRight = NO;
      }
      return;
    }
    [self stopTimers];
    if (_pending) {
      _pending = NO;
      CGPoint at = _pos;
      [self send:5 at:at right:NO];
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 50 * NSEC_PER_MSEC), dispatch_get_main_queue(),
                     ^{ [self send:6 at:at right:NO]; });
    }
  }
}
@end

@interface WGLView : UIView
@end
@implementation WGLView
+ (Class)layerClass { return CAEAGLLayer.class; }
@end

@interface WGameVC : UIViewController
@property(nonatomic, strong) WGLView* glView;
@property(nonatomic, strong) WMouse* mouse;
@property(nonatomic, strong) WPad* pad;
@property(nonatomic, strong) UIView* loading;
@property(nonatomic, strong) UILabel *loadingText, *stats, *toast;
@property(nonatomic) BOOL statsOn;
- (void)loadStatus:(int)stage text:(NSString*)text value:(int)value;
- (void)hideLoading;
- (void)showToast:(NSString*)text;
- (void)demo:(int)step;
@end

static CGPoint to_game(CGPoint p) {
  CGSize v = g_vc.glView.bounds.size;
  double s = fmin(v.width / g_game_w, v.height / g_game_h);
  double dx = (v.width - g_game_w * s) / 2, dy = (v.height - g_game_h * s) / 2;
  return CGPointMake(floor((p.x - dx) / s), floor((p.y - dy) / s));
}

static BOOL is_ru(void) { return [NSLocale.preferredLanguages.firstObject hasPrefix:@"ru"]; }
static NSString* tr(NSString* ru, NSString* en) { return is_ru() ? ru : en; }

@implementation WGameVC {
  CADisplayLink* _link;
  NSMutableSet<UITouch*>* _gameTouches;
  int _joyKey;
  NSMutableDictionary<NSString*, NSNumber*>* _padButtons;
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = UIColor.blackColor;
  self.view.multipleTouchEnabled = YES;
  _gameTouches = [NSMutableSet new];
  _padButtons = [NSMutableDictionary new];

  _glView = [[WGLView alloc] initWithFrame:self.view.bounds];
  _glView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  _glView.multipleTouchEnabled = YES;
  CAEAGLLayer* l = (CAEAGLLayer*)_glView.layer;
  l.opaque = YES;
  l.contentsScale = UIScreen.mainScreen.nativeScale;
  l.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @NO,
                           kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
  g_layer = l;
  [self.view addSubview:_glView];

  _mouse = [[WMouse alloc] initWithFrame:self.view.bounds];
  _mouse.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self.view addSubview:_mouse];

  NSString* ug = [docs_dir() stringByAppendingPathComponent:@"game"];
  _pad = [[WPad alloc] initWithHost:self.view gameDirs:@[ ug, [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"game"] ]];
  __weak WGameVC* ws = self;
  _pad.onKey = ^(BOOL down, int key) { [ws padKey:down key:key]; };
  _pad.onMouseSettings = ^{ [ws.mouse reloadConfig]; };

  [self buildOverlays];

  _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(onVsync:)];
  if (@available(iOS 15.0, *)) _link.preferredFrameRateRange = CAFrameRateRangeMake(60, 60, 60);
  else _link.preferredFramesPerSecond = 60;
  [_link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];

  [self setupControllers];
  if ([NSUserDefaults.standardUserDefaults boolForKey:@"padDemo"]) [self demo:0];
}

- (void)demo:(int)step {
  if (step > 8) return;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    double t = host_now_ms();
    if (step < 8) [self.pad demoStep:step];
    host_log("demo %d: %.0f ms", step, host_now_ms() - t);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
      UIGraphicsImageRenderer* r = [[UIGraphicsImageRenderer alloc] initWithSize:self.view.bounds.size];
      UIImage* img = [r imageWithActions:^(UIGraphicsImageRendererContext* c) {
        [self.view drawViewHierarchyInRect:self.view.bounds afterScreenUpdates:YES];
      }];
      NSString* f = [docs_dir() stringByAppendingPathComponent:[NSString stringWithFormat:@"demo-%d.png", step]];
      [UIImagePNGRepresentation(img) writeToFile:f atomically:YES];
      [self demo:step + 1];
    });
  });
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  g_layer.contentsScale = UIScreen.mainScreen.nativeScale;
  g_resize = true;
  [_pad areaChanged];
  [self layoutOverlays];
}

- (void)onVsync:(CADisplayLink*)link { vsync_tick(link.timestamp * 1000.0); }

- (void)buildOverlays {
  _loading = [[UIView alloc] initWithFrame:self.view.bounds];
  _loading.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  _loading.backgroundColor = PadColor(0xFF333333);
  _loading.userInteractionEnabled = NO;
  _loadingText = [UILabel new];
  _loadingText.textColor = UIColor.whiteColor;
  _loadingText.textAlignment = NSTextAlignmentCenter;
  _loadingText.numberOfLines = 0;
  _loadingText.font = [UIFont systemFontOfSize:15];
  _loadingText.text = tr(@"Запуск…", @"Starting…");
  [_loading addSubview:_loadingText];
  [self.view insertSubview:_loading aboveSubview:_mouse];

  _stats = [UILabel new];
  _stats.font = [UIFont monospacedDigitSystemFontOfSize:12 weight:UIFontWeightMedium];
  _stats.textColor = UIColor.whiteColor;
  _stats.backgroundColor = PadColor(0xB31B1B1E);
  _stats.textAlignment = NSTextAlignmentCenter;
  _stats.layer.cornerRadius = 6;
  _stats.clipsToBounds = YES;
  _stats.hidden = YES;
  _stats.userInteractionEnabled = NO;
  [self.view addSubview:_stats];

  _toast = [UILabel new];
  _toast.font = [UIFont systemFontOfSize:14];
  _toast.textColor = UIColor.whiteColor;
  _toast.backgroundColor = PadColor(0xE61B1B1E);
  _toast.textAlignment = NSTextAlignmentCenter;
  _toast.numberOfLines = 0;
  _toast.layer.cornerRadius = 10;
  _toast.layer.borderColor = PadColor(0xFF007ACC).CGColor;
  _toast.layer.borderWidth = 1;
  _toast.clipsToBounds = YES;
  _toast.alpha = 0;
  _toast.userInteractionEnabled = NO;
  [self.view addSubview:_toast];
  [self layoutOverlays];
}

- (void)layoutOverlays {
  CGRect b = self.view.bounds;
  UIEdgeInsets s = self.view.safeAreaInsets;
  _loadingText.frame = CGRectMake(20, b.size.height * 0.6, b.size.width - 40, 60);
  _stats.frame = CGRectMake(b.size.width - s.right - 150 - 8, s.top + 8, 150, 22);
  CGSize ts = [_toast sizeThatFits:CGSizeMake(b.size.width * 0.7, 200)];
  _toast.frame = CGRectMake((b.size.width - ts.width - 32) / 2, b.size.height - s.bottom - ts.height - 40,
                            ts.width + 32, ts.height + 16);
}

- (void)showToast:(NSString*)text {
  _toast.text = text;
  [self layoutOverlays];
  _toast.alpha = 1;
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(fadeToast) object:nil];
  [self performSelector:@selector(fadeToast) withObject:nil afterDelay:2.5];
}
- (void)fadeToast { [UIView animateWithDuration:0.3 animations:^{ self->_toast.alpha = 0; }]; }

- (void)toggleStats {
  _statsOn = !_statsOn;
  _stats.hidden = !_statsOn;
}

- (void)loadStatus:(int)stage text:(NSString*)text value:(int)value {
  if (!_loading.superview) return;
  NSString* t;
  if (stage == LOAD_ARCHIVE)
    t = [NSString stringWithFormat:tr(@"Обработка архива %@ (%d МБ)…\nПервый запуск большой игры может занять до минуты",
                                      @"Processing archive %@ (%d MB)…\nThe first start of a big game may take up to a minute"),
                                   text, value];
  else if (stage == LOAD_GAME)
    t = tr(@"Starting the game…", @"Starting the game…");
  else {
    NSArray* p = [text componentsSeparatedByString:@"|"];
    NSString* what = p.count > 1 ? p[1] : @"";
    if ([p[0] isEqualToString:@"read"]) t = [NSString stringWithFormat:tr(@"Чтение %@", @"Reading %@"), what];
    else if ([p[0] isEqualToString:@"unpack"]) t = [NSString stringWithFormat:tr(@"Распаковка %@", @"Unpacking %@"), what];
    else if ([p[0] isEqualToString:@"check"]) t = tr(@"Checking game files…", @"Checking game files…");
    else t = tr(@"Запуск…", @"Starting…");
    if (value >= 0) t = [t stringByAppendingFormat:@"  %d%%", value];
  }
  _loadingText.text = t;
  if (stage == LOAD_GAME) [self performSelector:@selector(hideLoading) withObject:nil afterDelay:1.5];
}

- (void)hideLoading {
  if (!_loading.superview) return;
  [UIView animateWithDuration:0.3 animations:^{ self->_loading.alpha = 0; }
                   completion:^(BOOL d) { [self->_loading removeFromSuperview]; }];
}

- (void)padKey:(BOOL)down key:(int)key {
  if (key == 326) {
    if (down) {
      _mouse.rightMode = !_mouse.rightMode;
      [self showToast:_mouse.rightMode ? tr(@"Правая кнопка мыши: вкл", @"Right mouse button: on")
                                       : tr(@"Правая кнопка мыши: выкл", @"Right mouse button: off")];
    }
    return;
  }
  if (key == 325 || key == 327) {
    CGPoint g = _mouse.visible ? to_game(_mouse.pos) : CGPointMake(g_game_w / 2, g_game_h / 2);
    send_mouse(down ? 5 : 6, (int)g.x, (int)g.y, key == 325 ? 0 : 1, down ? (key == 325 ? 1 : 4) : 0);
    return;
  }
  send_key(down, dom_from_android(key));
}

- (void)touchesBegan:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e {
  [_gameTouches unionSet:t];
  [_mouse touches:_gameTouches phase:0 changed:t];
}
- (void)touchesMoved:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [_mouse touches:_gameTouches phase:1 changed:t]; }
- (void)touchesEnded:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e {
  [_mouse touches:_gameTouches phase:2 changed:t];
  [_gameTouches minusSet:t];
}
- (void)touchesCancelled:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self touchesEnded:t withEvent:e]; }

- (BOOL)canBecomeFirstResponder { return YES; }
- (void)viewDidAppear:(BOOL)a {
  [super viewDidAppear:a];
  [self becomeFirstResponder];
}

- (BOOL)handlePresses:(NSSet<UIPress*>*)presses down:(BOOL)down {
  BOOL handled = NO;
  if (@available(iOS 13.4, *)) {
    for (UIPress* p in presses) {
      int dom = p.key ? dom_from_hid(p.key.keyCode) : 0;
      if (dom) {
        UIKeyModifierFlags m = p.key.modifierFlags;
        ev e = {.kind = 1, .a = down, .b = dom, .c = (m & UIKeyModifierShift) != 0,
                .d = (m & UIKeyModifierControl) != 0, .e = (m & UIKeyModifierAlternate) != 0};
        snprintf(e.text, sizeof e.text, "%d", dom);
        push_ev(&e);
        handled = YES;
      }
    }
  }
  return handled;
}
- (void)pressesBegan:(NSSet<UIPress*>*)p withEvent:(UIPressesEvent*)e {
  if (![self handlePresses:p down:YES]) [super pressesBegan:p withEvent:e];
}
- (void)pressesEnded:(NSSet<UIPress*>*)p withEvent:(UIPressesEvent*)e {
  if (![self handlePresses:p down:NO]) [super pressesEnded:p withEvent:e];
}
- (void)pressesCancelled:(NSSet<UIPress*>*)p withEvent:(UIPressesEvent*)e {
  if (![self handlePresses:p down:NO]) [super pressesCancelled:p withEvent:e];
}

- (void)setupControllers {
  [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(controllerConnected:)
                                             name:GCControllerDidConnectNotification object:nil];
  for (GCController* c in GCController.controllers) [self attachController:c];
}

- (void)controllerConnected:(NSNotification*)n {
  [self attachController:n.object];
  [self showToast:[NSString stringWithFormat:tr(@"Контроллер: %@", @"Controller: %@"), [n.object vendorName] ?: @"?"]];
}

- (void)padButton:(NSString*)name pressed:(BOOL)pressed dom:(int)dom {
  BOOL was = _padButtons[name].boolValue;
  if (was == pressed) return;
  _padButtons[name] = @(pressed);
  send_key(pressed, dom);
}

- (void)attachController:(GCController*)c {
  GCExtendedGamepad* g = c.extendedGamepad;
  if (!g) return;
  __weak WGameVC* ws = self;
  g.valueChangedHandler = ^(GCExtendedGamepad* p, GCControllerElement* el) {
    WGameVC* s = ws;
    if (!s) return;
    [s padButton:@"A" pressed:p.buttonA.pressed dom:13];
    [s padButton:@"B" pressed:p.buttonB.pressed dom:27];
    [s padButton:@"X" pressed:p.buttonX.pressed dom:16];
    [s padButton:@"Y" pressed:p.buttonY.pressed dom:32];
    [s padButton:@"L1" pressed:p.leftShoulder.pressed dom:33];
    [s padButton:@"R1" pressed:p.rightShoulder.pressed dom:34];
    if (@available(iOS 13.0, *)) [s padButton:@"Start" pressed:p.buttonMenu.pressed dom:27];

    float x = p.dpad.xAxis.value, y = -p.dpad.yAxis.value;
    if (fabsf(x) < 0.5f && fabsf(y) < 0.5f) { x = p.leftThumbstick.xAxis.value; y = -p.leftThumbstick.yAxis.value; }
    int key = 0;
    if (fabsf(x) > 0.5f || fabsf(y) > 0.5f) key = fabsf(x) > fabsf(y) ? (x < 0 ? 37 : 39) : (y < 0 ? 38 : 40);
    if (key != s->_joyKey) {
      if (s->_joyKey) send_key(false, s->_joyKey);
      if (key) send_key(true, key);
      s->_joyKey = key;
    }
  };
}

- (void)textInput:(BOOL)show text:(NSString*)text max:(int)maxLen {
  if (!show) {
    if (self.presentedViewController) [self dismissViewControllerAnimated:YES completion:nil];
    return;
  }
  if (self.presentedViewController) return;
  UIAlertController* a = [UIAlertController alertControllerWithTitle:nil message:nil
                                                      preferredStyle:UIAlertControllerStyleAlert];
  __block BOOL done = NO;
  [a addTextFieldWithConfigurationHandler:^(UITextField* f) {
    f.text = text;
    f.returnKeyType = UIReturnKeyDone;
    [f addTarget:self action:@selector(textChanged:) forControlEvents:UIControlEventEditingChanged];
    f.tag = maxLen;
  }];
  void (^finish)(int) = ^(int st) {
    if (done) return;
    done = YES;
    ev e = {.kind = 6, .a = st};
    snprintf(e.text, sizeof e.text, "%s", a.textFields.firstObject.text.UTF8String ?: "");
    push_ev(&e);
  };
  [a addAction:[UIAlertAction actionWithTitle:tr(@"Отмена", @"Cancel") style:UIAlertActionStyleCancel
                                      handler:^(UIAlertAction* x) { finish(2); }]];
  [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                      handler:^(UIAlertAction* x) { finish(1); }]];
  [self presentViewController:a animated:YES completion:nil];
}

- (void)textChanged:(UITextField*)f {
  if (f.tag > 0 && f.text.length > (NSUInteger)f.tag) f.text = [f.text substringToIndex:f.tag];
  ev e = {.kind = 6, .a = 0};
  snprintf(e.text, sizeof e.text, "%s", f.text.UTF8String ?: "");
  push_ev(&e);
}

- (void)gameMessage:(NSString*)text {
  if ([text hasPrefix:@"anotherGame|"]) {
    NSString* f = [text substringFromIndex:12];
    [self showToast:[NSString stringWithFormat:tr(@"Сохранение от другой игры (%@) отложено: %@.anotherGame",
                                                  @"Save data from another game (%@) moved aside: %@.anotherGame"), f, f]];
  } else if ([text hasPrefix:@"["] && [text containsString:@"Error]|"]) {
    NSArray* p = [text componentsSeparatedByString:@"|"];
    [self hideLoading];
    NSString* msg = p.count > 1 ? p[1] : @"";
    if (p.count > 2) msg = [msg stringByAppendingFormat:@"\n\n%@", [[p subarrayWithRange:NSMakeRange(2, p.count - 2)] componentsJoinedByString:@"|"]];
    UIAlertController* a = [UIAlertController alertControllerWithTitle:p[0] message:msg
                                                        preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* x) {
      send_key(true, 13);
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 400 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{ send_key(false, 13); });
    }]];
    if (!self.presentedViewController) [self presentViewController:a animated:YES completion:nil];
  }
}
@end

static NSString* S(const char* s) { return [NSString stringWithUTF8String:s ? s : ""] ?: @""; }

void plat_window_text(const char* text) {
  host_log("title: %s", text);
  NSString* t = [NSString stringWithUTF8String:text ? text : ""] ?: @"";
  on_main(^{ [g_vc.pad statsTitle:t]; });
}
void plat_game_message(const char* text) {
  host_log("game message: %s", text);
  NSString* t = S(text);
  on_main(^{ [g_vc gameMessage:t]; });
}
void plat_before_game_start(void) {
  host_log("game ready to show");
  g_game_started = true;
  on_main(^{ [g_vc hideLoading]; });
}
void plat_set_game_size(int w, int h) {
  g_game_w = w > 0 ? w : 640;
  g_game_h = h > 0 ? h : 480;
}
void plat_vibrate(int input_type, int power, int time_ms, int effect) {
  if (time_ms <= 0) return;
  on_main(^{
    UIImpactFeedbackGenerator* g = [[UIImpactFeedbackGenerator alloc]
        initWithStyle:power > 600 ? UIImpactFeedbackStyleHeavy : power > 300 ? UIImpactFeedbackStyleMedium
                                                                              : UIImpactFeedbackStyleLight];
    [g impactOccurred];
  });
}
void plat_sync_saves(int from_storage) {}
void plat_text_input(bool show, const char* text, int maxlen) {
  NSString* t = S(text);
  on_main(^{ [g_vc textInput:show text:t max:maxlen]; });
}
void plat_load_status(int stage, const char* text, int value) {
  NSString* t = S(text);
  on_main(^{ [g_vc loadStatus:stage text:t value:value]; });
}
double plat_canvas_css_width(void) {
  int w, h;
  host_gl_screen_size(&w, &h);
  return w;
}
void plat_exit(int code) {
  host_log("engine exited, code %d", code);
  g_running = false;
}

static void stats_cb(int loop, int shown) {
  on_main(^{ [g_vc.pad statsFps:loop screen:shown]; });
}

static BOOL has_game(NSString* dir) {
  NSFileManager* fm = NSFileManager.defaultManager;
  return [fm fileExistsAtPath:[dir stringByAppendingPathComponent:@"Data.wolf"]] ||
         [fm fileExistsAtPath:[dir stringByAppendingPathComponent:@"Data"]];
}

static NSString* docs_dir(void) {
  return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
}

static NSString* find_game(void) {
  NSString* userGame = [docs_dir() stringByAppendingPathComponent:@"game"];
  if (has_game(userGame)) return userGame;
  NSArray* items = [NSFileManager.defaultManager contentsOfDirectoryAtPath:userGame error:nil];
  for (NSString* n in items) {
    NSString* sub = [userGame stringByAppendingPathComponent:n];
    if (has_game(sub)) return sub;
  }
  NSString* bundleGame = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"game"];
  return has_game(bundleGame) ? bundleGame : nil;
}

static void setup_paths(void) {
  NSFileManager* fm = NSFileManager.defaultManager;
  NSString* docs = docs_dir();
  NSString* support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  NSString* root = [support stringByAppendingPathComponent:@"woditor_root"];
  NSString* userGame = [docs stringByAppendingPathComponent:@"game"];
  NSString* save = [docs stringByAppendingPathComponent:@"Save"];
  for (NSString* d in @[ root, userGame, save ])
    [fm createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil error:nil];

  NSString* hint = [userGame stringByAppendingPathComponent:@"README.txt"];
  if (![fm fileExistsAtPath:hint])
    [@"Положите сюда файлы игры WOLF RPG для Browser Woditor: Data.wolf (или Data/), Game.ini, "
     @"InitialDownloadList.ini, FontList.ini, шрифты, DefaultSoundFont.sf2 (можно папкой игры целиком). "
     @"Затем откройте приложение снова.\n"
     @"Put the WOLF RPG game files for Browser Woditor here (or the whole game folder), then reopen the app.\n"
        writeToFile:hint atomically:YES encoding:NSUTF8StringEncoding error:nil];
  snprintf(g_root, sizeof g_root, "%s", root.fileSystemRepresentation);
  snprintf(g_save, sizeof g_save, "%s", save.fileSystemRepresentation);
  NSString* pid = [NSBundle.mainBundle objectForInfoDictionaryKey:@"WoditorProjectId"] ?: @"default";
  snprintf(g_project, sizeof g_project, "%s", pid.UTF8String);

  FILE* lf = fopen([docs stringByAppendingPathComponent:@"woditor.log"].fileSystemRepresentation, "w");
  host_log_file = lf;
  host_log("iOS %s, %s", UIDevice.currentDevice.systemVersion.UTF8String, UIDevice.currentDevice.model.UTF8String);
  host_log("saves: %s", g_save);
}

@interface WPanel : UIView
+ (WPanel*)showIn:(UIView*)host title:(NSString*)title text:(NSString*)text button:(NSString*)button
           action:(void (^)(void))action;
@end

@implementation WPanel {
  void (^_action)(void);
}
+ (WPanel*)showIn:(UIView*)host title:(NSString*)title text:(NSString*)text button:(NSString*)button
           action:(void (^)(void))action {
  WPanel* p = [[WPanel alloc] initWithFrame:host.bounds];
  p.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  p.backgroundColor = PadColor(0x99000000);
  p->_action = action;
  CGFloat w = fmin(460, host.bounds.size.width - 40);
  UIView* box = [UIView new];
  box.backgroundColor = PadColor(0xFF1B1B1E);
  box.layer.cornerRadius = 12;
  box.layer.borderColor = PadColor(0xFF007ACC).CGColor;
  box.layer.borderWidth = 1;
  UILabel* t = [UILabel new];
  t.text = title;
  t.textColor = UIColor.whiteColor;
  t.font = [UIFont boldSystemFontOfSize:17];
  UIButton* x = [UIButton buttonWithType:UIButtonTypeSystem];
  [x setTitle:@"✕" forState:UIControlStateNormal];
  [x setTitleColor:PadColor(0xFFCCCCCC) forState:UIControlStateNormal];
  x.titleLabel.font = [UIFont systemFontOfSize:20];
  [x addTarget:p action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
  UILabel* m = [UILabel new];
  m.text = text;
  m.textColor = PadColor(0xFFCCCCCC);
  m.font = [UIFont systemFontOfSize:14];
  m.numberOfLines = 0;
  CGSize ms = [m sizeThatFits:CGSizeMake(w - 32, 1000)];
  t.frame = CGRectMake(16, 12, w - 64, 26);
  x.frame = CGRectMake(w - 44, 6, 38, 38);
  m.frame = CGRectMake(16, 46, w - 32, ms.height);
  CGFloat h = 46 + ms.height + 16;
  [box addSubview:t];
  [box addSubview:x];
  [box addSubview:m];
  if (button) {
    UIButton* b = [UIButton buttonWithType:UIButtonTypeSystem];
    [b setTitle:button forState:UIControlStateNormal];
    [b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    b.backgroundColor = PadColor(0xFF007ACC);
    b.layer.cornerRadius = 8;
    b.titleLabel.font = [UIFont boldSystemFontOfSize:15];
    b.frame = CGRectMake(16, h, w - 32, 40);
    [b addTarget:p action:@selector(tapped) forControlEvents:UIControlEventTouchUpInside];
    [box addSubview:b];
    h += 40 + 16;
  }
  box.frame = CGRectMake((host.bounds.size.width - w) / 2, (host.bounds.size.height - h) / 2, w, h);
  box.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin |
                         UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
  [p addSubview:box];
  [host addSubview:p];
  return p;
}
- (void)close { [self removeFromSuperview]; }
- (void)tapped {
  if (_action) _action();
}
@end

static WPanel* g_missing;

_Noreturn void plat_engine_finished(void) {
  g_running = false;
  on_main(^{
    [g_vc hideLoading];
    [WPanel showIn:g_vc.view title:tr(@"Игра закрыта", @"Game closed")
              text:tr(@"Игра завершила работу. Чтобы начать снова, закройте приложение (смахните вверх) и откройте заново.",
                      @"The game has exited. To play again, close the app (swipe it away) and open it again.")
            button:nil action:nil];
  });
  pthread_exit(NULL);
}

@interface WAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow* window;
@end

@implementation WAppDelegate
- (BOOL)application:(UIApplication*)app didFinishLaunchingWithOptions:(NSDictionary*)opts {
  setup_paths();
  self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  g_vc = [WGameVC new];
  self.window.rootViewController = g_vc;
  [self.window makeKeyAndVisible];
  g_ctx = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!g_ctx) {
    host_log("no OpenGL ES 3");
    return YES;
  }
  [self startIfReady];
  return YES;
}

- (void)startIfReady {
  if (g_running || !g_ctx) return;
  NSString* game = find_game();
  if (!game) {
    if ([NSUserDefaults.standardUserDefaults boolForKey:@"padDemo"]) return;
    if (!g_missing.superview) {
      host_log("no game — waiting for files in Documents/game");
      __weak WAppDelegate* ws = self;
      g_missing = [WPanel
          showIn:g_vc.view title:tr(@"Нет файлов игры", @"No game files")
            text:tr(@"Откройте «Файлы» → «На iPhone» → Woditor → game и положите туда файлы игры для Browser Woditor "
                    @"(Data.wolf или папку Data, Game.ini, InitialDownloadList.ini, FontList.ini, шрифты) — можно папкой игры "
                    @"целиком. С компьютера — через Finder/iTunes, «Общие файлы». Потом вернитесь сюда.",
                    @"Open Files → On My iPhone → Woditor → game and put the Browser Woditor game files there "
                    @"(Data.wolf or the Data folder, Game.ini, InitialDownloadList.ini, FontList.ini, fonts) — the whole "
                    @"game folder is fine. From a computer: Finder/iTunes, File Sharing. Then come back here.")
          button:tr(@"Проверить снова", @"Check again")
          action:^{ [ws startIfReady]; }];
    }
    return;
  }
  [g_missing removeFromSuperview];
  g_missing = nil;
  snprintf(g_game, sizeof g_game, "%s", game.fileSystemRepresentation);
  host_log("game: %s", g_game);
  g_running = true;

  pthread_attr_t at;
  pthread_attr_init(&at);
  pthread_attr_setstacksize(&at, 16u << 20);
  pthread_t th;
  pthread_create(&th, &at, engine_thread, NULL);
  pthread_attr_destroy(&at);
}

- (void)applicationWillResignActive:(UIApplication*)app {
  [g_vc.pad releaseAll];
  pthread_mutex_lock(&g_lock);
  g_pause_ack = false;
  g_paused = true;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;
  while (!g_pause_ack && g_running)
    if (pthread_cond_timedwait(&g_cond, &g_lock, &ts)) {
      host_log("engine thread busy — will pause later");
      break;
    }
  pthread_mutex_unlock(&g_lock);
  audio_set_paused(true);
}

- (void)applicationDidBecomeActive:(UIApplication*)app {
  g_paused = false;
  g_resize = true;
  audio_set_paused(false);
  [self startIfReady];
}
@end

int main(int argc, char* argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass(WAppDelegate.class));
  }
}
