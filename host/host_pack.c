#include "host.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

typedef struct host_pack_file host_pack_file;
typedef struct {
  char* name;
  char* lname;
  bool dir;
  int method;
  u32 crc;
  u64 csize, usize, lho;
  host_pack_file* open;
} pack_ent;

struct host_pack_file {
  int refs;
  pack_ent* ent;
  u64 size;
  u64 data;
  u8* mem;
  int fd;
};

#define BIG_INFLATE (4u << 20)
static char g_cache_dir[1024];
static int g_pfd = -1;
static pack_ent* g_ents;
static int g_nents, g_cap;
static int* g_by_lower;
static time_t g_mtime;

static u32 lower_cp(u32 c) {
  if (c >= 'A' && c <= 'Z') return c + 32;
  if (c < 0x80) return c;
  if ((c >= 0xC0 && c <= 0xDE && c != 0xD7)) return c + 32;
  if (c >= 0x410 && c <= 0x42F) return c + 32;
  if (c >= 0x400 && c <= 0x40F) return c + 80;
  if (c >= 0xFF21 && c <= 0xFF3A) return c + 32;
  return c;
}

static int put_utf8(u32 c, char* o) {
  if (c < 0x80) { o[0] = (char)c; return 1; }
  if (c < 0x800) { o[0] = (char)(0xC0 | c >> 6); o[1] = (char)(0x80 | (c & 63)); return 2; }
  o[0] = (char)(0xE0 | c >> 12); o[1] = (char)(0x80 | ((c >> 6) & 63)); o[2] = (char)(0x80 | (c & 63));
  return 3;
}

void host_name_lower(const char* s, char* out, size_t n) {
  size_t o = 0;
  const unsigned char* p = (const unsigned char*)s;
  while (*p && o + 4 < n) {
    u32 c;
    int len;
    if (*p < 0x80) c = *p, len = 1;
    else if ((*p & 0xE0) == 0xC0 && p[1]) c = (u32)(*p & 31) << 6 | (p[1] & 63), len = 2;
    else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) c = (u32)(*p & 15) << 12 | (u32)(p[1] & 63) << 6 | (p[2] & 63), len = 3;
    else {
      out[o++] = (char)*p++;
      continue;
    }
    u32 l = lower_cp(c);
    if (l == c) {
      memcpy(out + o, p, (size_t)len);
      o += (size_t)len;
    } else {
      o += (size_t)put_utf8(l, out + o);
    }
    p += len;
  }
  out[o] = 0;
}

bool host_name_eq_ci(const char* a, const char* b) {
  if (!strcmp(a, b)) return true;
  char la[1024], lb[1024];
  host_name_lower(a, la, sizeof la);
  host_name_lower(b, lb, sizeof lb);
  return !strcmp(la, lb);
}

static u16 rd16(const u8* p) { return (u16)(p[0] | p[1] << 8); }
static u32 rd32(const u8* p) { return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24; }
static u64 rd64(const u8* p) { return rd32(p) | (u64)rd32(p + 4) << 32; }

static bool pread_all(void* buf, u64 n, u64 off) {
  u64 got = 0;
  while (got < n) {
    ssize_t r = pread(g_pfd, (u8*)buf + got, (size_t)(n - got), (off_t)(off + got));
    if (r <= 0) return false;
    got += (u64)r;
  }
  return true;
}

static void add_ent(const char* name, size_t len, bool dir, int method, u32 crc, u64 cs, u64 us, u64 lho) {
  if (g_nents == g_cap) {
    g_cap = g_cap ? g_cap * 2 : 1024;
    g_ents = realloc(g_ents, sizeof(pack_ent) * (size_t)g_cap);
  }
  pack_ent* e = &g_ents[g_nents++];
  e->name = strndup(name, len);
  e->dir = dir;
  e->method = method;
  e->crc = crc;
  e->open = NULL;
  e->csize = cs;
  e->usize = us;
  e->lho = lho;
}

static int ent_cmp(const void* a, const void* b) {
  const pack_ent *x = a, *y = b;
  int c = strcmp(x->name, y->name);
  return c ? c : (int)y->dir - (int)x->dir;
}

static pack_ent* find(const char* rel) {
  int lo = 0, hi = g_nents - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int c = strcmp(g_ents[mid].name, rel);
    if (!c) return &g_ents[mid];
    if (c < 0) lo = mid + 1;
    else hi = mid - 1;
  }

  char l[1024];
  host_name_lower(rel, l, sizeof l);
  lo = 0, hi = g_nents - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int c = strcmp(g_ents[g_by_lower[mid]].lname, l);
    if (!c) return &g_ents[g_by_lower[mid]];
    if (c < 0) lo = mid + 1;
    else hi = mid - 1;
  }
  return NULL;
}

static int lower_cmp(const void* a, const void* b) {
  return strcmp(g_ents[*(const int*)a].lname, g_ents[*(const int*)b].lname);
}

bool host_pack_open(const char* zip, const char* prefix, const char* cache_dir) {
  snprintf(g_cache_dir, sizeof g_cache_dir, "%s", cache_dir ? cache_dir : "");
  g_pfd = open(zip, O_RDONLY | O_CLOEXEC);
  if (g_pfd < 0) return false;
  struct stat st;
  fstat(g_pfd, &st);
  g_mtime = st.st_mtime;
  u64 fsize = (u64)st.st_size;

  u64 tail = fsize < 65557 ? fsize : 65557;
  u8* t = malloc(tail);
  if (!pread_all(t, tail, fsize - tail)) goto fail_t;
  long eocd = -1;
  for (long i = (long)tail - 22; i >= 0; i--)
    if (rd32(t + i) == 0x06054b50) {
      eocd = i;
      break;
    }
  if (eocd < 0) goto fail_t;
  u64 cd_off = rd32(t + eocd + 16), cd_size = rd32(t + eocd + 12), count = rd16(t + eocd + 10);
  if (eocd >= 20 && rd32(t + eocd - 20) == 0x07064b50) {
    u8 e64[56];
    if (pread_all(e64, 56, rd64(t + eocd - 20 + 8)) && rd32(e64) == 0x06064b50) {
      count = rd64(e64 + 32);
      cd_size = rd64(e64 + 40);
      cd_off = rd64(e64 + 48);
    }
  }
  free(t);

  u8* cd = malloc(cd_size ? cd_size : 1);
  if (!pread_all(cd, cd_size, cd_off)) {
    free(cd);
    goto fail;
  }
  size_t plen = strlen(prefix);
  u8* p = cd;
  for (u64 i = 0; i < count && p + 46 <= cd + cd_size && rd32(p) == 0x02014b50; i++) {
    int method = rd16(p + 10);
    u32 crc = rd32(p + 16);
    u64 cs = rd32(p + 20), us = rd32(p + 24), lho = rd32(p + 42);
    u16 nl = rd16(p + 28), xl = rd16(p + 30), cl = rd16(p + 32);
    const char* name = (const char*)p + 46;

    for (u8* x = p + 46 + nl; x + 4 <= p + 46 + nl + xl;) {
      u16 id = rd16(x), sz = rd16(x + 2);
      if (id == 1) {
        u8* v = x + 4;
        if (us == 0xFFFFFFFFu) us = rd64(v), v += 8;
        if (cs == 0xFFFFFFFFu) cs = rd64(v), v += 8;
        if (lho == 0xFFFFFFFFu) lho = rd64(v);
      }
      x += 4 + sz;
    }
    if (nl > plen && !strncmp(name, prefix, plen)) {
      const char* rel = name + plen;
      size_t rl = nl - plen;
      bool dir = rel[rl - 1] == '/';
      if (dir) rl--;
      if (rl) add_ent(rel, rl, dir, method, crc, cs, us, lho);

      for (size_t k = 0; k < rl; k++)
        if (rel[k] == '/') add_ent(rel, k, true, 0, 0, 0, 0, 0);
    }
    p += 46 + nl + xl + cl;
  }
  free(cd);
  if (!g_nents) goto fail;
  add_ent("", 0, true, 0, 0, 0, 0, 0);
  qsort(g_ents, (size_t)g_nents, sizeof(pack_ent), ent_cmp);
  int o = 0;
  for (int i = 0; i < g_nents; i++) {
    if (o && !strcmp(g_ents[o - 1].name, g_ents[i].name)) {
      free(g_ents[i].name);
      continue;
    }
    g_ents[o++] = g_ents[i];
  }
  g_nents = o;
  g_by_lower = malloc(sizeof(int) * (size_t)g_nents);
  for (int i = 0; i < g_nents; i++) {
    char l[1024];
    host_name_lower(g_ents[i].name, l, sizeof l);
    g_ents[i].lname = strdup(l);
    g_by_lower[i] = i;
  }
  qsort(g_by_lower, (size_t)g_nents, sizeof(int), lower_cmp);
  int files = 0;
  u64 bytes = 0, packed = 0;
  for (int i = 0; i < g_nents; i++)
    if (!g_ents[i].dir) {
      files++;
      bytes += g_ents[i].usize;
      if (g_ents[i].method) packed++;
    }
  host_log("pack: %s/%s — %d files (%.1f MB), %llu compressed", zip, prefix, files, bytes / 1048576.0,
           (unsigned long long)packed);
  for (int i = 0; i < g_nents; i++)
    if (!g_ents[i].dir && g_ents[i].method && g_ents[i].usize >= BIG_INFLATE)
      host_log("pack: %s is compressed in the APK (%.1f MB) — the first start unpacks it to the cache; "
               "better store game files uncompressed", g_ents[i].name, g_ents[i].usize / 1048576.0);
  return true;

fail_t:
  free(t);
fail:
  close(g_pfd);
  g_pfd = -1;
  return false;
}

bool host_pack_active(void) { return g_pfd >= 0; }

bool host_pack_stat(const char* rel, bool* is_dir, u64* size) {
  if (g_pfd < 0) return false;
  const pack_ent* e = find(rel);
  if (!e) return false;
  if (is_dir) *is_dir = e->dir;
  if (size) *size = e->dir ? 4096 : e->usize;
  return true;
}

time_t host_pack_mtime(void) { return g_mtime; }

void host_pack_list(const char* rel_dir, void (*cb)(void* ud, const char* name), void* ud) {
  if (g_pfd < 0) return;
  const pack_ent* d = find(rel_dir);
  if (!d || !d->dir) return;
  char pre[1024];
  size_t pl = (size_t)snprintf(pre, sizeof pre, "%s%s", d->name, *d->name ? "/" : "");
  for (int i = 0; i < g_nents; i++) {
    const char* n = g_ents[i].name;
    if (!*n || strncmp(n, pre, pl)) continue;
    if (!strchr(n + pl, '/')) cb(ud, n + pl);
  }
}

static bool inflate_ent(const pack_ent* e, u64 data, u8* dst_mem, int dst_fd, bool show) {
  if (!e->usize) return true;
  z_stream z = {0};
  if (inflateInit2(&z, -MAX_WBITS) != Z_OK) return false;
  enum { IN = 1 << 20, OUT = 4 << 20 };
  u8* in = malloc(IN);
  u8* out = dst_mem ? NULL : malloc(OUT);
  u64 in_off = 0, out_off = 0;
  uLong crc = crc32(0, NULL, 0);
  int zr = Z_OK, last_pct = -1;
  const char* base = strrchr(e->name, '/') ? strrchr(e->name, '/') + 1 : e->name;
  while (zr == Z_OK) {
    if (!z.avail_in) {
      u64 n = e->csize - in_off < IN ? e->csize - in_off : IN;
      if (!n || !pread_all(in, n, data + in_off)) break;
      in_off += n;
      z.next_in = in;
      z.avail_in = (uInt)n;
    }
    u8* o = dst_mem ? dst_mem + out_off : out;
    u64 room = dst_mem ? e->usize - out_off : OUT;
    if (room > OUT) room = OUT;
    z.next_out = o;
    z.avail_out = (uInt)room;
    zr = inflate(&z, Z_NO_FLUSH);
    u64 got = room - z.avail_out;
    crc = crc32(crc, o, (uInt)got);
    if (!dst_mem && got) {
      u64 w = 0;
      while (w < got) {
        ssize_t r = write(dst_fd, out + w, (size_t)(got - w));
        if (r <= 0) {
          zr = Z_ERRNO;
          break;
        }
        w += (u64)r;
      }
    }
    out_off += got;
    int pct = e->usize ? (int)(out_off * 100 / e->usize) : 100;
    if (show && pct != last_pct) {
      host_load_status(LOAD_TEXT, pct, "unpack|%s", base);
      last_pct = pct;
    }
    if (dst_mem && out_off >= e->usize && zr == Z_OK) zr = Z_STREAM_END;
  }
  inflateEnd(&z);
  free(in);
  free(out);
  bool ok = zr == Z_STREAM_END && out_off == e->usize && (u32)crc == e->crc;
  if (!ok) host_log("pack: failed to unpack %s (zlib %d, %llu of %llu bytes, crc %s)", e->name, zr,
                    (unsigned long long)out_off, (unsigned long long)e->usize, (u32)crc == e->crc ? "ok" : "mismatch");
  return ok;
}

static int cached_file(const pack_ent* e, u64 data) {
  char safe[512], path[2048], tmp[2100];
  size_t k = 0;
  for (const char* c = e->name; *c && k + 1 < sizeof safe; c++) safe[k++] = *c == '/' ? '_' : *c;
  safe[k] = 0;
  snprintf(path, sizeof path, "%s/%s.%08x.%llu", g_cache_dir, safe, e->crc, (unsigned long long)e->usize);
  struct stat st;
  if (!stat(path, &st) && (u64)st.st_size == e->usize) {
    host_log("pack: %s — already unpacked to the cache", e->name);
    return open(path, O_RDONLY | O_CLOEXEC);
  }
  mkdir(g_cache_dir, 0755);

  DIR* d = opendir(g_cache_dir);
  if (d) {
    struct dirent* de;
    size_t sl = strlen(safe);
    while ((de = readdir(d)))
      if (!strncmp(de->d_name, safe, sl) && de->d_name[sl] == '.') {
        char old[2048];
        snprintf(old, sizeof old, "%s/%s", g_cache_dir, de->d_name);
        unlink(old);
      }
    closedir(d);
  }
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) {
    host_log("pack: cannot create %s (%s)", tmp, strerror(errno));
    return -1;
  }
  double t0 = host_now_ms();
  bool ok = inflate_ent(e, data, NULL, fd, true);
  close(fd);
  if (!ok || rename(tmp, path)) {
    unlink(tmp);
    return -1;
  }
  host_log("pack: %s unpacked to the cache in %.1f s", e->name, (host_now_ms() - t0) / 1000);
  return open(path, O_RDONLY | O_CLOEXEC);
}

host_pack_file* host_pack_fopen(const char* rel) {
  if (g_pfd < 0) return NULL;
  pack_ent* e = find(rel);
  if (!e || e->dir) return NULL;
  if (e->open) return host_pack_fdup(e->open);
  u8 lh[30];
  if (!pread_all(lh, 30, e->lho) || rd32(lh) != 0x04034b50) return NULL;
  host_pack_file* f = calloc(1, sizeof *f);
  f->refs = 1;
  f->ent = e;
  f->fd = -1;
  f->size = e->usize;
  f->data = e->lho + 30 + rd16(lh + 26) + rd16(lh + 28);
  if (e->method != 0 && e->method != 8) {
    host_log("pack: %s — unknown compression %d", rel, e->method);
    free(f);
    return NULL;
  }
  if (e->method == 8) {
    bool ok;
    if (e->usize >= BIG_INFLATE && *g_cache_dir) {
      f->fd = cached_file(e, f->data);
      ok = f->fd >= 0;
    } else {
      if (e->usize >= BIG_INFLATE) host_log("pack: %s is unpacked to memory (%.1f MB)", rel, e->usize / 1048576.0);
      f->mem = malloc(e->usize ? e->usize : 1);
      ok = f->mem && inflate_ent(e, f->data, f->mem, -1, e->usize >= BIG_INFLATE);
    }
    if (!ok) {
      free(f->mem);
      free(f);
      return NULL;
    }
  }
  e->open = f;
  return f;
}

host_pack_file* host_pack_fdup(host_pack_file* f) {
  f->refs++;
  return f;
}

void host_pack_fclose(host_pack_file* f) {
  if (!f || --f->refs) return;
  if (f->ent) f->ent->open = NULL;
  free(f->mem);
  if (f->fd >= 0) close(f->fd);
  free(f);
}

u64 host_pack_fsize(host_pack_file* f) { return f->size; }

int64_t host_pack_pread(host_pack_file* f, void* buf, u64 n, u64 off) {
  if (off >= f->size) return 0;
  if (n > f->size - off) n = f->size - off;
  if (f->mem) {
    memcpy(buf, f->mem + off, (size_t)n);
    return (int64_t)n;
  }
  if (f->fd >= 0) {
    u64 got = 0;
    while (got < n) {
      ssize_t r = pread(f->fd, (u8*)buf + got, (size_t)(n - got), (off_t)(off + got));
      if (r <= 0) return got ? (int64_t)got : -1;
      got += (u64)r;
    }
    return (int64_t)n;
  }
  return pread_all(buf, n, f->data + off) ? (int64_t)n : -1;
}
