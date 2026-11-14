#include "host.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
  W_EACCES = 2, W_EAGAIN = 6, W_EBADF = 8, W_EBUSY = 10, W_EEXIST = 20, W_EFBIG = 22, W_EINVAL = 28, W_EIO = 29,
  W_EISDIR = 31, W_ELOOP = 32, W_EMFILE = 33, W_ENAMETOOLONG = 37, W_ENOENT = 44, W_ENOMEM = 48, W_ENOSPC = 51,
  W_ENOSYS = 52, W_ENOTDIR = 54, W_ENOTEMPTY = 55, W_ENOTTY = 59, W_EPERM = 63, W_ERANGE = 68, W_EROFS = 69,
  W_ESPIPE = 70, W_EXDEV = 75,
};

static int werr(int e) {
  switch (e) {
    case EACCES: return W_EACCES;
    case EAGAIN: return W_EAGAIN;
    case EBADF: return W_EBADF;
    case EBUSY: return W_EBUSY;
    case EEXIST: return W_EEXIST;
    case EFBIG: return W_EFBIG;
    case EINVAL: return W_EINVAL;
    case EISDIR: return W_EISDIR;
    case ELOOP: return W_ELOOP;
    case EMFILE: return W_EMFILE;
    case ENAMETOOLONG: return W_ENAMETOOLONG;
    case ENOENT: return W_ENOENT;
    case ENOMEM: return W_ENOMEM;
    case ENOSPC: return W_ENOSPC;
    case ENOTDIR: return W_ENOTDIR;
    case ENOTEMPTY: return W_ENOTEMPTY;
    case EPERM: return W_EPERM;
    case ERANGE: return W_ERANGE;
    case EROFS: return W_EROFS;
    case ESPIPE: return W_ESPIPE;
    case EXDEV: return W_EXDEV;
    default: return W_EIO;
  }
}
#define RET_ERR(e) return (u32)(-(int32_t)(e))

enum {
  WO_ACCMODE = 3, WO_CREAT = 0100, WO_EXCL = 0200, WO_TRUNC = 01000, WO_APPEND = 02000, WO_DIRECTORY = 0200000,
};

static char g_root[1024];
static char g_game[1024];
static char g_cache[256];
static char g_cwd[1024] = "/";
static char g_save_virt[512];
static char g_persist[512];
static char g_save_real[1024];

static int canon(const char* path, char* out, size_t n) {
  char tmp[2048];
  if (path[0] == '/') snprintf(tmp, sizeof tmp, "%s", path);
  else snprintf(tmp, sizeof tmp, "%s/%s", g_cwd, path);
  char* parts[256];
  int np = 0;
  for (char* tok = strtok(tmp, "/"); tok; tok = strtok(NULL, "/")) {
    if (!strcmp(tok, ".") || !*tok) continue;
    if (!strcmp(tok, "..")) {
      if (np) np--;
      continue;
    }
    if (np == 256) return -W_ENAMETOOLONG;
    parts[np++] = tok;
  }
  size_t o = 0;
  if (!np) {
    snprintf(out, n, "/");
    return 0;
  }
  for (int i = 0; i < np; i++) {
    int w = snprintf(out + o, n - o, "/%s", parts[i]);
    if (w < 0 || (size_t)w >= n - o) return -W_ENAMETOOLONG;
    o += (size_t)w;
  }
  return 0;
}

typedef struct {
  char* dir;
  char** names;
  char** lnames;
  int n;
} dir_cache;
#define DC_MAX 64
static dir_cache g_dc[DC_MAX];
static int g_dc_next;

static void dc_free(dir_cache* d) {
  for (int i = 0; i < d->n; i++) {
    free(d->names[i]);
    free(d->lnames[i]);
  }
  free(d->names);
  free(d->lnames);
  free(d->dir);
  memset(d, 0, sizeof *d);
}

static void dc_clear_all(void) {
  for (int i = 0; i < DC_MAX; i++)
    if (g_dc[i].dir) dc_free(&g_dc[i]);
}

static void dc_invalidate(const char* real) {
  char parent[2048];
  snprintf(parent, sizeof parent, "%s", real);
  char* sl = strrchr(parent, '/');
  if (sl) *sl = 0;
  for (int i = 0; i < DC_MAX; i++)
    if (g_dc[i].dir && (!strcmp(g_dc[i].dir, parent) || !strcmp(g_dc[i].dir, real))) dc_free(&g_dc[i]);
}

static const dir_cache* dc_get(const char* dir) {
  for (int i = 0; i < DC_MAX; i++)
    if (g_dc[i].dir && !strcmp(g_dc[i].dir, dir)) return &g_dc[i];
  DIR* d = opendir(dir);
  if (!d) return NULL;
  dir_cache* e = &g_dc[g_dc_next];
  g_dc_next = (g_dc_next + 1) % DC_MAX;
  if (e->dir) dc_free(e);
  e->dir = strdup(dir);
  int cap = 0;
  struct dirent* de;
  while ((de = readdir(d))) {
    if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
    if (e->n == cap) {
      cap = cap ? cap * 2 : 16;
      e->names = realloc(e->names, sizeof(char*) * (size_t)cap);
      e->lnames = realloc(e->lnames, sizeof(char*) * (size_t)cap);
    }
    char l[1024];
    host_name_lower(de->d_name, l, sizeof l);
    e->names[e->n] = strdup(de->d_name);
    e->lnames[e->n++] = strdup(l);
  }
  closedir(d);
  return e;
}

static const char* ci_match(const char* dir, const char* name) {
  const dir_cache* d = dc_get(dir);
  if (!d) return NULL;
  char l[1024];
  host_name_lower(name, l, sizeof l);
  for (int i = 0; i < d->n; i++)
    if (!strcmp(d->lnames[i], l)) return d->names[i];
  return NULL;
}

static void ci_resolve(const char* base, const char* rel, char* out, size_t n) {
  struct stat st;
  snprintf(out, n, "%s%s", base, rel);
  if (!lstat(out, &st)) return;
  size_t o = (size_t)snprintf(out, n, "%s", base);
  bool found = true;
  for (const char* p = rel; *p && o < n;) {
    while (*p == '/') p++;
    if (!*p) break;
    size_t len = strcspn(p, "/");
    char comp[512];
    snprintf(comp, sizeof comp, "%.*s", (int)len, p);
    p += len;
    const char* use = comp;
    if (found) {
      snprintf(out + o, n - o, "/%s", comp);
      if (lstat(out, &st)) {
        out[o] = 0;
        const char* m = ci_match(out, comp);
        if (m) use = m;
        else found = false;
      }
    }
    o += (size_t)snprintf(out + o, n - o, "/%s", use);
  }
}

static const char* ci_prefix(const char* virt, const char* prefix) {
  size_t pl = strlen(prefix);
  if (strncmp(virt, prefix, pl)) {
    if (strlen(virt) < pl) return NULL;
    char a[1024];
    snprintf(a, sizeof a, "%.*s", (int)pl, virt);
    if (!host_name_eq_ci(a, prefix)) return NULL;
  }
  if (virt[pl] != '/' && virt[pl]) return NULL;
  return virt + pl;
}

static void real_root(const char* virt, char* out, size_t n) {
  const char* rest;
  if (*g_save_real && (rest = ci_prefix(virt, g_save_virt))) {
    ci_resolve(g_save_real, rest, out, n);
    return;
  }
  ci_resolve(g_root, virt, out, n);
}

static int game_path(const char* virt, char* out, size_t n) {
  const char* rest = ci_prefix(virt, g_cache);
  if (!rest) return 0;
  ci_resolve(g_game, rest, out, n);
  return 1;
}

static const char* pack_rel(const char* virt) {
  if (!host_pack_active()) return NULL;
  const char* rest = ci_prefix(virt, g_cache);
  if (!rest) return NULL;
  return *rest ? rest + 1 : "";
}

#define MAX_MODS 32
static char g_mod_dirs[MAX_MODS][1024];
static int g_nmods;

static bool ends_ci(const char* s, const char* suffix) {
  size_t a = strlen(s), b = strlen(suffix);
  return a >= b && host_name_eq_ci(s + a - b, suffix);
}

static bool mod_blocked(const char* rel) {
  const char* base = strrchr(rel, '/');
  base = base ? base + 1 : rel;
  if (host_name_eq_ci(base, "BrowserWoditor.dat")) return true;
  if (!strchr(rel + 1, '/'))
    return host_name_eq_ci(base, "mod.json") || ends_ci(base, ".wolf") || ends_ci(base, ".dxa");
  return false;
}

static const char* mod_rel(const char* virt) {
  const char* rest = ci_prefix(virt, g_cache);
  if (rest) {
    if (!*rest || ci_prefix(virt, g_save_virt)) return NULL;
    return rest;
  }

  size_t len = strcspn(virt + 1, "/");
  if (virt[0] != '/' || len != 4 || !virt[1 + len]) return NULL;
  char first[8];
  snprintf(first, sizeof first, "%.*s", (int)len, virt + 1);
  return host_name_eq_ci(first, "data") ? virt : NULL;
}

static bool mod_find(const char* rel, char* out, size_t n) {
  if (!g_nmods || !rel || mod_blocked(rel)) return false;
  struct stat st;
  for (int i = 0; i < g_nmods; i++) {
    ci_resolve(g_mod_dirs[i], rel, out, n);
    if (!stat(out, &st) && S_ISREG(st.st_mode)) {

      static uint32_t seen[1024];
      static int nseen;
      uint32_t h = 2166136261u;
      for (const char* p = out; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
      bool dup = false;
      for (int k = 0; k < nseen && !dup; k++) dup = seen[k] == h;
      if (!dup && nseen < 1024) {
        seen[nseen++] = h;
        const char* name = strrchr(g_mod_dirs[i], '/');
        host_log("mod %s: %s", name ? name + 1 : g_mod_dirs[i], rel);
      }
      return true;
    }
  }
  return false;
}

enum { LOC_NONE, LOC_DISK, LOC_PACK };
static int locate(const char* virt, char* out, size_t n, const char** rel) {
  struct stat st;
  if (g_nmods && mod_find(mod_rel(virt), out, n)) return LOC_DISK;
  real_root(virt, out, n);
  if (!lstat(out, &st)) return LOC_DISK;
  char g[2048];
  if (game_path(virt, g, sizeof g) && !stat(g, &st)) {
    snprintf(out, n, "%s", g);
    return LOC_DISK;
  }
  const char* pr = pack_rel(virt);
  if (pr && host_pack_stat(pr, NULL, NULL)) {
    if (rel) *rel = pr;
    return LOC_PACK;
  }
  return LOC_NONE;
}

static void mkdirs_for(const char* real) {
  char tmp[2048];
  snprintf(tmp, sizeof tmp, "%s", real);
  for (char* p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      if (!mkdir(tmp, 0755)) dc_invalidate(tmp);
      *p = '/';
    }
  }
}

static void rm_tree(const char* path) {
  struct stat st;
  if (lstat(path, &st)) return;
  if (S_ISDIR(st.st_mode)) {
    DIR* d = opendir(path);
    struct dirent* e;
    while (d && (e = readdir(d))) {
      if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
      char c[2048];
      snprintf(c, sizeof c, "%s/%s", path, e->d_name);
      rm_tree(c);
    }
    if (d) closedir(d);
    rmdir(path);
  } else {
    unlink(path);
  }
}

static int upper_count(const char* s) {
  int n = 0;
  for (; *s; s++) n += *s >= 'A' && *s <= 'Z';
  return n;
}

static void ci_merge_tree(const char* dir) {
  DIR* d = opendir(dir);
  if (!d) return;
  char* names[512];
  int n = 0;
  struct dirent* e;
  while ((e = readdir(d)) && n < 512)
    if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) names[n++] = strdup(e->d_name);
  closedir(d);
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; names[i] && j < n; j++) {
      if (!names[j] || !host_name_eq_ci(names[i], names[j])) continue;
      char a[2048], b[2048];
      snprintf(a, sizeof a, "%s/%s", dir, names[i]);
      snprintf(b, sizeof b, "%s/%s", dir, names[j]);
      struct stat sa, sb;
      if (lstat(a, &sa) || lstat(b, &sb)) continue;
      int keep_j;
      if (S_ISDIR(sa.st_mode) && S_ISDIR(sb.st_mode)) keep_j = upper_count(names[j]) > upper_count(names[i]);
      else keep_j = sb.st_mtime > sa.st_mtime;
      const char* keep = keep_j ? b : a;
      const char* drop = keep_j ? a : b;
      if (S_ISDIR(sa.st_mode) && S_ISDIR(sb.st_mode)) {
        DIR* dd = opendir(drop);
        while (dd && (e = readdir(dd))) {
          if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
          char from[2048], to[2048];
          snprintf(from, sizeof from, "%s/%s", drop, e->d_name);
          snprintf(to, sizeof to, "%s/%s", keep, e->d_name);
          struct stat st;
          if (lstat(to, &st)) rename(from, to);
        }
        if (dd) closedir(dd);
      }
      host_log("FS: \"%s\" and \"%s\" differ only in case, keeping %s", names[i], names[j], keep);
      rm_tree(drop);
      if (keep_j) {
        free(names[i]);
        names[i] = NULL;
      } else {
        free(names[j]);
        names[j] = NULL;
      }
    }
  }
  for (int i = 0; i < n; i++) {
    if (!names[i]) continue;
    char c[2048];
    struct stat st;
    snprintf(c, sizeof c, "%s/%s", dir, names[i]);
    if (!lstat(c, &st) && S_ISDIR(st.st_mode)) ci_merge_tree(c);
    free(names[i]);
  }
}

int host_fs_game_file_available(const char* virt) {
  char c[1024], real[2048];
  if (canon(virt, c, sizeof c)) return 0;
  struct stat st;
  const char* rel;
  bool dir;
  switch (locate(c, real, sizeof real, &rel)) {
    case LOC_DISK: return !stat(real, &st) && S_ISREG(st.st_mode);
    case LOC_PACK: return host_pack_stat(rel, &dir, NULL) && !dir;
    default: return 0;
  }
}

static int read_source_pack(const char* rel, u32* out_ptr, u32* out_len) {
  host_pack_file* pf = host_pack_fopen(rel);
  if (!pf) return 0;
  u64 len = host_pack_fsize(pf);
  u32 ptr = len <= 0x7FFFFFFF ? host_malloc(len ? (u32)len : 1) : 0;
  int64_t got = ptr ? host_pack_pread(pf, hp(ptr), len, 0) : -1;
  host_pack_fclose(pf);
  if (got < 0) {
    if (ptr) host_free(ptr);
    return 0;
  }
  *out_ptr = ptr;
  *out_len = (u32)got;
  return 1;
}

int host_fs_read_source(const char* rel, u32* out_ptr, u32* out_len) {
  char path[2048], mrel[1100];
  snprintf(mrel, sizeof mrel, "/%s", rel);
  if (!mod_find(mrel, path, sizeof path)) snprintf(path, sizeof path, "%s/%s", g_game, rel);
  int fd = open(path, O_RDONLY);
  if (fd < 0) return read_source_pack(rel, out_ptr, out_len);
  struct stat st;
  fstat(fd, &st);
  if (st.st_size > 0x7FFFFFFF) {
    close(fd);
    return 0;
  }
  u32 len = (u32)st.st_size;
  u32 ptr = host_malloc(len ? len : 1);
  if (!ptr) {
    close(fd);
    return 0;
  }
  size_t got = 0;
  while (got < len) {
    ssize_t r = read(fd, hp(ptr) + got, len - got);
    if (r <= 0) break;
    got += (size_t)r;
  }
  close(fd);
  *out_ptr = ptr;
  *out_len = (u32)got;
  return 1;
}

enum { K_FREE, K_FILE, K_DIR, K_TTY, K_NULL, K_RANDOM, K_PACK };

typedef struct {
  int kind;
  int fd;
  int flags;
  char vpath[1024];
  char** dents;
  int ndents;
  host_pack_file* pf;
  u64 pos;
  u64 size;
  bool big;
} wfd;

#define BIG_FILE (32u << 20)
#define READ_CHUNK (16u << 20)

static const char* base_name(const char* path) {
  const char* b = strrchr(path, '/');
  return b ? b + 1 : path;
}

static void read_progress(wfd* f, u64 pos) {
  if (f->big && f->size) host_load_status(LOAD_TEXT, (int)(pos * 100 / f->size), "read|%s", base_name(f->vpath));
}

#define MAX_FD 512
static wfd g_fds[MAX_FD];
static u64 g_dir_pos[MAX_FD];

static int alloc_fd(int from) {
  for (int i = from; i < MAX_FD; i++)
    if (g_fds[i].kind == K_FREE) return i;
  return -1;
}

static wfd* get_fd(u32 fd) {
  if (fd >= MAX_FD || g_fds[fd].kind == K_FREE) return NULL;
  return &g_fds[fd];
}

static void free_dents(wfd* f) {
  for (int i = 0; i < f->ndents; i++) free(f->dents[i]);
  free(f->dents);
  f->dents = NULL;
  f->ndents = 0;
}

void host_fs_init(const host_config* cfg) {
  snprintf(g_root, sizeof g_root, "%s", cfg->root_dir);
  snprintf(g_game, sizeof g_game, "%s", cfg->game_dir);
  snprintf(g_cache, sizeof g_cache, "/data0/%s-GameCache", cfg->project_id);
  char real[2048];
  snprintf(real, sizeof real, "%s/data0", g_root);
  ci_merge_tree(real);

  const char* dirs[] = {"/tmp", "/home/web_user", "/dev", "/data0", g_cache, NULL};
  for (int i = 0; dirs[i]; i++) {
    real_root(dirs[i], real, sizeof real);
    mkdirs_for(real);
    mkdir(real, 0755);
  }
  char save[512];
  snprintf(save, sizeof save, "/data0/%s-Save", cfg->project_id);
  real_root(save, real, sizeof real);
  mkdir(real, 0755);
  snprintf(g_persist, sizeof g_persist, "%s", save);
  snprintf(g_save_virt, sizeof g_save_virt, "%s/Save", g_cache);
  if (cfg->save_dir && *cfg->save_dir) {
    snprintf(g_save_real, sizeof g_save_real, "%s", cfg->save_dir);
    char probe[1100];
    snprintf(probe, sizeof probe, "%s/", g_save_real);
    mkdirs_for(probe);

    snprintf(real, sizeof real, "%s%s", g_root, g_save_virt);
    mkdir(real, 0755);
    host_log("FS: saves (%s) -> %s", g_save_virt, g_save_real);
  }
  g_nmods = 0;
  for (const char* m = cfg->mod_dirs; m && *m && g_nmods < MAX_MODS;) {
    size_t len = strcspn(m, "\n");
    if (len && len < sizeof g_mod_dirs[0]) {
      snprintf(g_mod_dirs[g_nmods], sizeof g_mod_dirs[0], "%.*s", (int)len, m);
      host_log("FS: mod %d — %s", g_nmods + 1, g_mod_dirs[g_nmods]);
      g_nmods++;
    }
    m += len;
    if (*m) m++;
  }
  g_fds[0].kind = g_fds[1].kind = g_fds[2].kind = K_TTY;
  host_log("FS: root %s, game %s, cache %s", g_root, g_game, g_cache);
  if (cfg->game_pack && *cfg->game_pack &&
      !host_pack_open(cfg->game_pack, cfg->game_pack_prefix ? cfg->game_pack_prefix : "", cfg->cache_dir))
    host_log("FS: no game files in %s (%s)", cfg->game_pack, cfg->game_pack_prefix ? cfg->game_pack_prefix : "");
}

static bool fslog(void) {
  static int on = -1;
  if (on < 0) on = getenv("WODITOR_FSLOG") != NULL;
  return on;
}

static u32 do_open(const char* path, int flags, int mode) {
  char v[1024];
  int e = canon(path, v, sizeof v);
  if (e) RET_ERR(-e);
  if (fslog()) host_log("open %s flags=%o", v, flags);
  int idx = alloc_fd(3);
  if (idx < 0) RET_ERR(W_EMFILE);
  wfd* f = &g_fds[idx];
  memset(f, 0, sizeof *f);
  snprintf(f->vpath, sizeof f->vpath, "%s", v);
  f->flags = flags;

  if (!strcmp(v, "/dev/null")) { f->kind = K_NULL; return (u32)idx; }
  if (!strncmp(v, "/dev/tty", 8) || !strcmp(v, "/dev/stdout") || !strcmp(v, "/dev/stderr")) { f->kind = K_TTY; return (u32)idx; }
  if (!strcmp(v, "/dev/urandom") || !strcmp(v, "/dev/random")) { f->kind = K_RANDOM; return (u32)idx; }

  int acc = flags & WO_ACCMODE;
  bool for_write = acc != 0 || (flags & (WO_CREAT | WO_TRUNC));
  char real[2048];
  int hflags = (acc == 1 ? O_WRONLY : acc == 2 ? O_RDWR : O_RDONLY) | O_CLOEXEC;
  if (flags & WO_CREAT) hflags |= O_CREAT;
  if (flags & WO_EXCL) hflags |= O_EXCL;
  if (flags & WO_TRUNC) hflags |= O_TRUNC;
  if (flags & WO_APPEND) hflags |= O_APPEND;
  if (flags & WO_DIRECTORY) hflags |= O_DIRECTORY;

  if (for_write) {
    real_root(v, real, sizeof real);
    struct stat st;
    char g[2048];

    const char* pr;
    bool pdir;
    if (lstat(real, &st) && game_path(v, g, sizeof g) && !stat(g, &st) && S_ISREG(st.st_mode) && !(flags & WO_TRUNC)) {
      mkdirs_for(real);
      int in = open(g, O_RDONLY), out = open(real, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dc_invalidate(real);
      char buf[65536];
      ssize_t r;
      while (in >= 0 && out >= 0 && (r = read(in, buf, sizeof buf)) > 0) (void)!write(out, buf, (size_t)r);
      if (in >= 0) close(in);
      if (out >= 0) close(out);
    } else if (lstat(real, &st) && (pr = pack_rel(v)) && host_pack_stat(pr, &pdir, NULL) && !pdir &&
               !(flags & WO_TRUNC)) {
      mkdirs_for(real);
      host_pack_file* in = host_pack_fopen(pr);
      int out = open(real, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dc_invalidate(real);
      char buf[65536];
      int64_t r;
      for (u64 off = 0; in && out >= 0 && (r = host_pack_pread(in, buf, sizeof buf, off)) > 0; off += (u64)r)
        (void)!write(out, buf, (size_t)r);
      host_pack_fclose(in);
      if (out >= 0) close(out);
    }
    if (flags & WO_CREAT) mkdirs_for(real);
  } else {
    const char* pr;
    if (locate(v, real, sizeof real, &pr) == LOC_PACK) {
      bool dir;
      host_pack_stat(pr, &dir, NULL);
      if (dir) {
        f->kind = K_DIR;
        f->fd = -1;
        g_dir_pos[idx] = 0;
        return (u32)idx;
      }
      if (flags & WO_DIRECTORY) RET_ERR(W_ENOTDIR);
      f->pf = host_pack_fopen(pr);
      if (!f->pf) RET_ERR(W_EIO);
      f->kind = K_PACK;
      f->fd = -1;
      f->size = host_pack_fsize(f->pf);
      f->big = f->size >= BIG_FILE;
      return (u32)idx;
    }
  }

  struct stat st;
  if (!stat(real, &st) && S_ISDIR(st.st_mode)) {
    if (for_write && acc) RET_ERR(W_EISDIR);
    f->kind = K_DIR;
    f->fd = -1;
    g_dir_pos[idx] = 0;
    return (u32)idx;
  }
  if (flags & WO_DIRECTORY) {
    f->kind = K_FREE;
    RET_ERR(errno == ENOENT || stat(real, &st) ? W_ENOENT : W_ENOTDIR);
  }
  int hfd = open(real, hflags, mode ? mode : 0644);
  if (hfd >= 0 && (hflags & O_CREAT)) dc_invalidate(real);
  if (hfd < 0) {
    f->kind = K_FREE;
    RET_ERR(werr(errno));
  }
  f->kind = K_FILE;
  f->fd = hfd;
  if (!fstat(hfd, &st) && S_ISREG(st.st_mode)) {
    f->size = (u64)st.st_size;
    f->big = f->size >= BIG_FILE && !for_write;
  }
  return (u32)idx;
}

u32 w2c_env_0x5F_syscall_openat(struct w2c_env* env, u32 dirfd, u32 path, u32 flags, u32 varargs) {
  int mode = varargs ? (int)hr32(varargs) : 0;
  const char* p = hstr(path);
  if (fslog() && p[0] != '/') host_log("openat dirfd=%d %s", (int)dirfd, p);
  char full[2048];
  if (p[0] != '/' && (int32_t)dirfd != -100) {
    wfd* d = get_fd(dirfd);
    if (!d) RET_ERR(W_EBADF);
    snprintf(full, sizeof full, "%s/%s", d->vpath, p);
    p = full;
  }
  return do_open(p, (int)flags, mode);
}

u32 w2c_wasi__snapshot__preview1_fd_close(struct w2c_wasi__snapshot__preview1* w, u32 fd) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("close fd=%u %s", fd, f ? f->vpath : "?");
  if (!f) return W_EBADF;
  if (fd <= 2) return 0;
  if (f->kind == K_FILE) close(f->fd);
  if (f->kind == K_PACK) host_pack_fclose(f->pf);
  free_dents(f);
  f->kind = K_FREE;
  return 0;
}

u32 w2c_wasi__snapshot__preview1_fd_read(struct w2c_wasi__snapshot__preview1* w, u32 fd, u32 iov, u32 iovcnt, u32 pnum) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("read fd=%u %s iov=%u len0=%u", fd, f ? f->vpath : "?", iovcnt, iovcnt ? hr32(iov + 4) : 0);
  if (!f) return W_EBADF;
  u32 total = 0;
  for (u32 i = 0; i < iovcnt; i++) {
    u32 ptr = hr32(iov + 8 * i), len = hr32(iov + 8 * i + 4);
    if (!len) continue;
    if (f->kind == K_FILE) {
      size_t got = 0;
      while (got < len) {
        size_t want = len - got;
        if (f->big && want > READ_CHUNK) want = READ_CHUNK;
        ssize_t r = read(f->fd, hp(ptr) + got, want);
        if (r < 0) {
          if (errno == EINTR) continue;
          return (u32)werr(errno);
        }
        if (r == 0) break;
        got += (size_t)r;
        if (f->big && len >= READ_CHUNK) read_progress(f, (u64)lseek(f->fd, 0, SEEK_CUR));
      }
      total += (u32)got;
      if (got < len) break;
    } else if (f->kind == K_PACK) {
      u32 got = 0;
      while (got < len) {
        u32 want = len - got;
        if (f->big && want > READ_CHUNK) want = READ_CHUNK;
        int64_t r = host_pack_pread(f->pf, hp(ptr) + got, want, f->pos);
        if (r < 0) return W_EIO;
        if (r == 0) break;
        f->pos += (u64)r;
        got += (u32)r;
        if (f->big && len >= READ_CHUNK) read_progress(f, f->pos);
      }
      total += got;
      if (got < len) break;
    } else if (f->kind == K_RANDOM) {
      for (u32 k = 0; k < len; k++) hw8(ptr + k, (uint8_t)(rand() & 0xFF));
      total += len;
    } else if (f->kind == K_DIR) {
      return W_EISDIR;
    } else {
      break;
    }
  }
  hw32(pnum, total);

  if (f->big && total >= READ_CHUNK) host_load_status(LOAD_ARCHIVE, (int)(f->size >> 20), "%s", base_name(f->vpath));
  return 0;
}

u32 w2c_wasi__snapshot__preview1_fd_write(struct w2c_wasi__snapshot__preview1* w, u32 fd, u32 iov, u32 iovcnt, u32 pnum) {
  wfd* f = get_fd(fd);
  if (fslog() && fd > 2) host_log("write fd=%u %s iov=%u", fd, f ? f->vpath : "?", iovcnt);
  if (!f) return W_EBADF;
  u32 total = 0;
  for (u32 i = 0; i < iovcnt; i++) {
    u32 ptr = hr32(iov + 8 * i), len = hr32(iov + 8 * i + 4);
    if (!len) continue;
    if (f->kind == K_FILE) {
      size_t put = 0;
      while (put < len) {
        ssize_t r = write(f->fd, hp(ptr) + put, len - put);
        if (r < 0) {
          if (errno == EINTR) continue;
          return (u32)werr(errno);
        }
        put += (size_t)r;
      }
    } else if (f->kind == K_TTY) {
      host_console_write(hp(ptr), len);
    } else if (f->kind == K_DIR) {
      return W_EISDIR;
    } else if (f->kind == K_PACK) {
      return W_EBADF;
    }
    total += len;
  }
  hw32(pnum, total);
  return 0;
}

static int load_dents(wfd* f);

u32 w2c_wasi__snapshot__preview1_fd_seek(struct w2c_wasi__snapshot__preview1* w, u32 fd, u64 offset, u32 whence, u32 newOffset) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("seek fd=%d off=%lld whence=%u", (int)fd, (long long)offset, whence);
  if (!f) return W_EBADF;
  if (f->kind == K_FILE) {
    off_t r = lseek(f->fd, (off_t)(int64_t)offset, whence == 0 ? SEEK_SET : whence == 1 ? SEEK_CUR : SEEK_END);
    if (r < 0) return (u32)werr(errno);
    hw64(newOffset, (u64)r);
    return 0;
  }
  if (f->kind == K_PACK) {
    int64_t base = whence == 0 ? 0 : whence == 1 ? (int64_t)f->pos : (int64_t)host_pack_fsize(f->pf);
    int64_t np = base + (int64_t)offset;
    if (np < 0) return W_EINVAL;
    f->pos = (u64)np;
    hw64(newOffset, f->pos);
    return 0;
  }
  if (f->kind == K_DIR) {
    if (fslog()) host_log("seek dir fd=%u off=%llu whence=%u", fd, (unsigned long long)offset, whence);

    if (whence == 0) {
      if (offset == 0) free_dents(f);
      g_dir_pos[fd] = offset;
    }
    hw64(newOffset, g_dir_pos[fd]);
    return 0;
  }
  if (f->kind == K_TTY) return W_ESPIPE;
  hw64(newOffset, 0);
  return 0;
}

u32 w2c_wasi__snapshot__preview1_fd_fdstat_get(struct w2c_wasi__snapshot__preview1* w, u32 fd, u32 pbuf) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("fdstat fd=%d", (int)fd);
  if (!f) return W_EBADF;
  uint8_t type = f->kind == K_TTY ? 2 : f->kind == K_DIR ? 3 : 4;
  hw8(pbuf, type);
  hw16(pbuf + 2, 0);
  hw64(pbuf + 8, 0);
  hw64(pbuf + 16, 0);
  return 0;
}

static void write_stat(u32 buf, const struct stat* st) {
  memset(hp(buf), 0, 96);
  hw32(buf, (u32)st->st_dev);
  hw32(buf + 4, (u32)st->st_mode);
  hw32(buf + 8, (u32)st->st_nlink);
  hw32(buf + 20, (u32)st->st_rdev);
  hw64(buf + 24, (u64)st->st_size);
  hw32(buf + 32, 4096);
  hw32(buf + 36, (u32)((st->st_size + 511) / 512));
  hw64(buf + 40, (u64)st->st_atime);
  hw64(buf + 56, (u64)st->st_mtime);
  hw64(buf + 72, (u64)st->st_ctime);
  hw64(buf + 88, (u64)st->st_ino);
}

static void pack_stat(u32 buf, bool dir, u64 size, const char* name) {
  struct stat st = {0};
  st.st_mode = dir ? (S_IFDIR | 0555) : (S_IFREG | 0444);
  st.st_nlink = 1;
  st.st_size = (off_t)size;
  st.st_atime = st.st_mtime = st.st_ctime = host_pack_mtime();
  u64 h = 1469598103934665603ULL;
  for (const char* p = name; *p; p++) h = (h ^ (u8)*p) * 1099511628211ULL;
  st.st_ino = (ino_t)h;
  write_stat(buf, &st);
}

static void tty_stat(u32 buf) {
  struct stat st = {0};
  st.st_mode = S_IFCHR | 0666;
  st.st_nlink = 1;
  write_stat(buf, &st);
}

static u32 stat_virt(const char* path, u32 buf, bool nofollow) {
  char v[1024], real[2048];
  int e = canon(path, v, sizeof v);
  if (e) RET_ERR(-e);
  if (fslog()) host_log("stat %s", v);
  if (!strncmp(v, "/dev/", 5)) {
    tty_stat(buf);
    return 0;
  }
  const char* pr;
  if (locate(v, real, sizeof real, &pr) == LOC_PACK) {
    bool dir;
    u64 size;
    host_pack_stat(pr, &dir, &size);
    pack_stat(buf, dir, size, pr);
    return 0;
  }
  struct stat st;
  if ((nofollow ? lstat(real, &st) : stat(real, &st)) < 0) RET_ERR(werr(errno));
  write_stat(buf, &st);
  return 0;
}

u32 w2c_env_0x5F_syscall_stat64(struct w2c_env* env, u32 path, u32 buf) { return stat_virt(hstr(path), buf, false); }
u32 w2c_env_0x5F_syscall_lstat64(struct w2c_env* env, u32 path, u32 buf) { return stat_virt(hstr(path), buf, true); }

u32 w2c_env_0x5F_syscall_fstat64(struct w2c_env* env, u32 fd, u32 buf) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("fstat fd=%d %s", (int)fd, f ? f->vpath : "?");
  if (!f) RET_ERR(W_EBADF);
  if (f->kind == K_FILE) {
    struct stat st;
    if (fstat(f->fd, &st) < 0) RET_ERR(werr(errno));
    write_stat(buf, &st);
    return 0;
  }
  if (f->kind == K_PACK) {
    pack_stat(buf, false, host_pack_fsize(f->pf), f->vpath);
    return 0;
  }
  if (f->kind == K_DIR) return stat_virt(f->vpath, buf, false);
  tty_stat(buf);
  return 0;
}

static const char* at_path(u32 dirfd, const char* p, char* full, size_t n) {
  if (p[0] == '/' || (int32_t)dirfd == -100) return p;
  wfd* d = get_fd(dirfd);
  if (!d) return NULL;
  snprintf(full, n, "%s/%s", d->vpath, p);
  return full;
}

u32 w2c_env_0x5F_syscall_newfstatat(struct w2c_env* env, u32 dirfd, u32 path, u32 buf, u32 flags) {
  const char* p = hstr(path);
  if (fslog()) host_log("fstatat dirfd=%d '%s' flags=%u", (int)dirfd, p, flags);
  if (!*p && (flags & 4096)) return w2c_env_0x5F_syscall_fstat64(env, dirfd, buf);
  char full[2048];
  p = at_path(dirfd, p, full, sizeof full);
  if (!p) RET_ERR(W_EBADF);
  return stat_virt(p, buf, (flags & 256) != 0);
}

static int dent_cmp(const void* a, const void* b) { return strcmp(*(char* const*)a, *(char* const*)b); }

static void add_dent(wfd* f, const char* name, int* cap) {
  for (int i = 0; i < f->ndents; i++)
    if (host_name_eq_ci(f->dents[i], name)) return;
  if (f->ndents == *cap) {
    *cap = *cap ? *cap * 2 : 64;
    f->dents = realloc(f->dents, sizeof(char*) * (size_t)*cap);
  }
  f->dents[f->ndents++] = strdup(name);
}

static char** g_known;
static int g_nknown;

static void known_add(const char* name, size_t len) {
  while (len && (name[len - 1] == ' ' || name[len - 1] == '\t' || name[len - 1] == '\r')) len--;
  while (len && (*name == ' ' || *name == '\t')) name++, len--;
  if (!len) return;
  size_t first = strcspn(name, "/\\");
  if (first < len) len = first;
  char* n = malloc(len + 1);
  memcpy(n, name, len);
  n[len] = 0;
  g_known = realloc(g_known, sizeof(char*) * (size_t)(g_nknown + 1));
  g_known[g_nknown++] = n;
}

static char* read_game_text(const char* name) {
  char virt[1024], real[2048];
  snprintf(virt, sizeof virt, "%s/%s", g_cache, name);
  const char* pr;
  int loc = locate(virt, real, sizeof real, &pr);
  char* buf = NULL;
  if (loc == LOC_PACK) {
    host_pack_file* pf = host_pack_fopen(pr);
    if (!pf) return NULL;
    u64 n = host_pack_fsize(pf);
    if (n < (1 << 20) && (buf = malloc(n + 1))) {
      if (host_pack_pread(pf, buf, n, 0) == (int64_t)n) buf[n] = 0;
      else { free(buf); buf = NULL; }
    }
    host_pack_fclose(pf);
  } else if (loc == LOC_DISK) {
    FILE* fp = fopen(real, "rb");
    if (!fp) return NULL;
    if ((buf = malloc((1 << 20) + 1))) buf[fread(buf, 1, 1 << 20, fp)] = 0;
    fclose(fp);
  }
  return buf;
}

static void known_load(void) {
  if (g_known) return;
  known_add("InitialDownloadList.ini", 23);
  known_add("FontList.ini", 12);
  char* t = read_game_text("InitialDownloadList.ini");
  for (char* p = t; p && *p;) {
    size_t len = strcspn(p, "\n");
    known_add(p, len);
    p += len + (p[len] ? 1 : 0);
  }
  free(t);
  t = read_game_text("FontList.ini");
  for (char* p = t; p && *p;) {
    size_t len = strcspn(p, "\n"), eq = strcspn(p, "=");
    if (eq < len) known_add(p, eq);
    p += len + (p[len] ? 1 : 0);
  }
  free(t);
}

static bool known_name(const char* name) {
  known_load();
  for (int i = 0; i < g_nknown; i++)
    if (host_name_eq_ci(g_known[i], name)) return true;
  return false;
}

typedef struct { wfd* f; int* cap; bool filter; } dent_ctx;
static void add_dent_cb(void* ud, const char* name) {
  dent_ctx* c = ud;
  if (c->filter && !known_name(name)) return;
  add_dent(c->f, name, c->cap);
}

static int load_dents(wfd* f) {
  if (f->dents) return 0;
  int cap = 0;
  add_dent(f, ".", &cap);
  add_dent(f, "..", &cap);
  if (host_name_eq_ci(f->vpath, g_persist)) return 0;
  char paths[2][2048];
  int np = 0;

  bool filter = host_name_eq_ci(f->vpath, g_cache);
  real_root(f->vpath, paths[np++], sizeof paths[0]);
  if (game_path(f->vpath, paths[np], sizeof paths[0])) np++;
  for (int i = 0; i < np; i++) {
    DIR* d = opendir(paths[i]);
    if (!d) continue;
    struct dirent* e;
    while ((e = readdir(d)))
      if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..") && (!filter || known_name(e->d_name)))
        add_dent(f, e->d_name, &cap);
    closedir(d);
  }
  const char* pr = pack_rel(f->vpath);
  bool pdir;
  if (pr && host_pack_stat(pr, &pdir, NULL) && pdir) {
    dent_ctx c = {f, &cap, filter};
    host_pack_list(pr, add_dent_cb, &c);
  }
  qsort(f->dents + 2, (size_t)(f->ndents - 2), sizeof(char*), dent_cmp);
  return 0;
}

u32 w2c_env_0x5F_syscall_getdents64(struct w2c_env* env, u32 fd, u32 dirp, u32 count) {
  wfd* f = get_fd(fd);
  if (!f) RET_ERR(W_EBADF);
  if (f->kind != K_DIR) RET_ERR(W_ENOTDIR);
  if (!f->dents) load_dents(f);
  u32 pos = 0;
  int idx = (int)(g_dir_pos[fd] / 280);
  int end = idx + (int)(count / 280);
  if (end > f->ndents) end = f->ndents;
  for (; idx < end; idx++) {
    const char* name = f->dents[idx];
    uint8_t type = 4;
    if (strcmp(name, ".") && strcmp(name, "..")) {
      char child[2048], real[2048];
      snprintf(child, sizeof child, "%s/%s", strcmp(f->vpath, "/") ? f->vpath : "", name);
      const char* pr;
      int loc = locate(child, real, sizeof real, &pr);
      struct stat st;
      bool pdir;
      if (loc == LOC_PACK) {
        host_pack_stat(pr, &pdir, NULL);
        type = pdir ? 4 : 8;
      } else {
        if (lstat(real, &st)) continue;
        type = S_ISDIR(st.st_mode) ? 4 : S_ISLNK(st.st_mode) ? 10 : S_ISCHR(st.st_mode) ? 2 : 8;
      }
    }
    hw64(dirp + pos, (u64)(idx + 1));
    hw64(dirp + pos + 8, (u64)(idx + 1) * 280);
    hw16(dirp + pos + 16, 280);
    hw8(dirp + pos + 18, type);
    host_write_str(dirp + pos + 19, name, 256);
    if (fslog()) host_log("  dent[%d] %s type=%d", idx, name, type);
    pos += 280;
  }
  g_dir_pos[fd] = (u64)idx * 280;
  if (fslog()) host_log("getdents fd=%u %s count=%u -> %u bytes", fd, f->vpath, count, pos);
  return pos;
}

u32 w2c_env_0x5F_syscall_mkdirat(struct w2c_env* env, u32 dirfd, u32 path, u32 mode) {
  char full[2048], v[1024], real[2048];
  const char* p = at_path(dirfd, hstr(path), full, sizeof full);
  if (!p) RET_ERR(W_EBADF);
  int e = canon(p, v, sizeof v);
  if (e) RET_ERR(-e);
  if (fslog()) host_log("mkdir %s", v);

  real_root(v, real, sizeof real);
  struct stat st;
  char g[2048];
  if (game_path(v, g, sizeof g) && !stat(g, &st)) RET_ERR(W_EEXIST);
  const char* pr = pack_rel(v);
  if (pr && host_pack_stat(pr, NULL, NULL)) RET_ERR(W_EEXIST);
  mkdirs_for(real);
  if (mkdir(real, 0755) < 0) RET_ERR(werr(errno));
  dc_invalidate(real);
  return 0;
}

u32 w2c_env_0x5F_syscall_rmdir(struct w2c_env* env, u32 path) {
  char v[1024], real[2048];
  int e = canon(hstr(path), v, sizeof v);
  if (e) RET_ERR(-e);
  real_root(v, real, sizeof real);
  if (rmdir(real) < 0) RET_ERR(werr(errno));
  dc_clear_all();
  return 0;
}

u32 w2c_env_0x5F_syscall_unlinkat(struct w2c_env* env, u32 dirfd, u32 path, u32 flags) {
  char full[2048], v[1024], real[2048];
  const char* p = at_path(dirfd, hstr(path), full, sizeof full);
  if (!p) RET_ERR(W_EBADF);
  int e = canon(p, v, sizeof v);
  if (e) RET_ERR(-e);
  real_root(v, real, sizeof real);
  if (fslog()) host_log("unlink %s flags=%u", v, flags);
  if (flags && flags != 512) RET_ERR(W_EINVAL);
  if ((flags ? rmdir(real) : unlink(real)) < 0) RET_ERR(werr(errno));
  dc_clear_all();
  return 0;
}

u32 w2c_env_0x5F_syscall_renameat(struct w2c_env* env, u32 olddirfd, u32 oldpath, u32 newdirfd, u32 newpath) {
  char f1[2048], f2[2048], v1[1024], v2[1024], r1[2048], r2[2048];
  const char* p1 = at_path(olddirfd, hstr(oldpath), f1, sizeof f1);
  const char* p2 = at_path(newdirfd, hstr(newpath), f2, sizeof f2);
  if (!p1 || !p2) RET_ERR(W_EBADF);
  if (canon(p1, v1, sizeof v1) || canon(p2, v2, sizeof v2)) RET_ERR(W_ENAMETOOLONG);
  real_root(v1, r1, sizeof r1);
  real_root(v2, r2, sizeof r2);
  if (fslog()) host_log("rename %s -> %s (%s -> %s)", v1, v2, r1, r2);
  mkdirs_for(r2);
  if (rename(r1, r2) < 0) {

    struct stat st;
    if (errno != EXDEV || lstat(r1, &st) || !S_ISREG(st.st_mode)) RET_ERR(werr(errno));
    int in = open(r1, O_RDONLY), out = open(r2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[65536];
    ssize_t r;
    bool ok = in >= 0 && out >= 0;
    while (ok && (r = read(in, buf, sizeof buf)) > 0) ok = write(out, buf, (size_t)r) == r;
    if (in >= 0) close(in);
    if (out >= 0) close(out);
    if (!ok) RET_ERR(W_EIO);
    unlink(r1);
  }
  dc_clear_all();
  return 0;
}

bool host_fs_move_aside(const char* virt_src, const char* virt_dst) {
  char v1[1024], v2[1024], r1[2048], r2[2048];
  if (canon(virt_src, v1, sizeof v1) || canon(virt_dst, v2, sizeof v2)) return false;
  real_root(v1, r1, sizeof r1);
  real_root(v2, r2, sizeof r2);
  struct stat st;
  if (lstat(r1, &st) || !S_ISREG(st.st_mode)) return false;
  unlink(r2);
  bool ok = !rename(r1, r2);
  if (!ok && errno == EXDEV) {
    int in = open(r1, O_RDONLY), out = open(r2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[65536];
    ssize_t r;
    ok = in >= 0 && out >= 0;
    while (ok && (r = read(in, buf, sizeof buf)) > 0) ok = write(out, buf, (size_t)r) == r;
    if (in >= 0) close(in);
    if (out >= 0) close(out);
    if (ok) unlink(r1);
  }
  dc_clear_all();
  return ok;
}

u32 w2c_env_0x5F_syscall_chdir(struct w2c_env* env, u32 path) {
  if (fslog()) host_log("chdir %s", hstr(path));
  char v[1024], real[2048];
  int e = canon(hstr(path), v, sizeof v);
  if (e) RET_ERR(-e);
  const char* pr;
  bool pdir = false;
  struct stat st;
  if (locate(v, real, sizeof real, &pr) == LOC_PACK) {
    host_pack_stat(pr, &pdir, NULL);
    if (!pdir) RET_ERR(W_ENOTDIR);
  } else {
    if (stat(real, &st) < 0) RET_ERR(werr(errno));
    if (!S_ISDIR(st.st_mode)) RET_ERR(W_ENOTDIR);
  }
  snprintf(g_cwd, sizeof g_cwd, "%s", v);
  return 0;
}

u32 w2c_env_0x5F_syscall_getcwd(struct w2c_env* env, u32 buf, u32 size) {
  if (fslog()) host_log("getcwd size=%u", size);
  if (!size) RET_ERR(W_EINVAL);
  u32 need = (u32)strlen(g_cwd) + 1;
  if (size < need) RET_ERR(W_ERANGE);
  host_write_str(buf, g_cwd, size);
  return need;
}

u32 w2c_env_0x5F_syscall_ftruncate64(struct w2c_env* env, u32 fd, u64 length) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("ftruncate fd=%d len=%llu", (int)fd, (unsigned long long)length);
  if (!f || f->kind != K_FILE) RET_ERR(W_EBADF);
  if (ftruncate(f->fd, (off_t)length) < 0) RET_ERR(werr(errno));
  return 0;
}

u32 w2c_env_0x5F_syscall_chmod(struct w2c_env* env, u32 path, u32 mode) {
  if (fslog()) host_log("chmod %s", hstr(path));
  return 0;
}
u32 w2c_env_0x5F_syscall_fchmod(struct w2c_env* env, u32 fd, u32 mode) {
  if (fslog()) host_log("fchmod fd=%d", (int)fd);
  return 0;
}
u32 w2c_env_0x5F_syscall_linkat(struct w2c_env* env, u32 a, u32 b, u32 c, u32 d, u32 e) { RET_ERR(W_EPERM); }
u32 w2c_env_0x5F_syscall_symlinkat(struct w2c_env* env, u32 a, u32 b, u32 c) { RET_ERR(W_EPERM); }
u32 w2c_env_0x5F_syscall_readlinkat(struct w2c_env* env, u32 dirfd, u32 path, u32 buf, u32 bufsize) {
  if (fslog()) host_log("readlink %s", hstr(path));
  RET_ERR(W_EINVAL);
}

u32 w2c_env_0x5F_syscall_fcntl64(struct w2c_env* env, u32 fd, u32 cmd, u32 varargs) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("fcntl fd=%u cmd=%u", fd, cmd);
  if (!f) RET_ERR(W_EBADF);
  switch (cmd) {
    case 0: {
      int arg = (int)hr32(varargs);
      int n = alloc_fd(arg < 3 ? 3 : arg);
      if (n < 0) RET_ERR(W_EMFILE);
      g_fds[n] = *f;
      g_fds[n].dents = NULL;
      g_fds[n].ndents = 0;
      if (f->kind == K_FILE) g_fds[n].fd = dup(f->fd);
      if (f->kind == K_PACK) g_fds[n].pf = host_pack_fdup(f->pf);
      return (u32)n;
    }
    case 1: case 2: case 13: case 14: return 0;
    case 3: return (u32)f->flags;
    case 4: return 0;
    case 12: hw16(hr32(varargs), 2); return 0;
    default: RET_ERR(W_EINVAL);
  }
}

u32 w2c_env_0x5F_syscall_ioctl(struct w2c_env* env, u32 fd, u32 op, u32 varargs) {
  wfd* f = get_fd(fd);
  if (fslog()) host_log("ioctl fd=%d op=%u", (int)fd, op);
  if (!f) RET_ERR(W_EBADF);
  if (f->kind != K_TTY) RET_ERR(W_ENOTTY);
  if (op == 21505) {
    u32 p = hr32(varargs);
    memset(hp(p), 0, 60);
    return 0;
  }
  if (op == 21523) {
    u32 p = hr32(varargs);
    hw16(p, 24);
    hw16(p + 2, 80);
    return 0;
  }
  return 0;
}

u32 w2c_env_0x5Fmmap_js(struct w2c_env* env, u32 len, u32 prot, u32 flags, u32 fd, u64 offset, u32 allocated, u32 addr) {
  wfd* f = get_fd(fd);
  if (!f || (f->kind != K_FILE && f->kind != K_PACK)) RET_ERR(W_EBADF);
  u32 ptr = w2c_woditor_emscripten_builtin_memalign(g_inst, 65536, len);
  if (!ptr) RET_ERR(W_ENOMEM);
  memset(hp(ptr), 0, len);
  if (f->kind == K_PACK) (void)host_pack_pread(f->pf, hp(ptr), len, offset);
  else (void)!pread(f->fd, hp(ptr), len, (off_t)offset);
  hw32(allocated, 1);
  hw32(addr, ptr);
  return 0;
}

u32 w2c_env_0x5Fmunmap_js(struct w2c_env* env, u32 addr, u32 len, u32 prot, u32 flags, u32 fd, u64 offset) {
  wfd* f = get_fd(fd);
  if (!f || (f->kind != K_FILE && f->kind != K_PACK)) RET_ERR(W_EBADF);
  if (f->kind == K_PACK) return 0;
  if ((prot & 2) && !(flags & 2)) (void)!pwrite(f->fd, hp(addr), len, (off_t)offset);
  return 0;
}
