#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

static void die(const char *m){ printf(1, "FAIL: %s\n", m); exit(); }
static void ok(const char *m) { printf(1, "PASS: %s\n", m); }

int
main(void)
{
  int fd;
  char buf[64];

  // ── Test 1: create a real file ────────────────────────────
  fd = open("tgt", O_CREATE | O_WRONLY);
  if(fd < 0) die("open tgt create");
  if(write(fd, "hello\n", 6) != 6) die("write tgt");
  close(fd);

  // ── Test 2: create a symlink pointing to it ───────────────
  if(symlink("tgt", "lnk") != 0) die("symlink tgt lnk");
  ok("symlink created");

  // ── Test 3: read through the symlink ─────────────────────
  fd = open("lnk", O_RDONLY);
  if(fd < 0) die("open lnk");
  int n = read(fd, buf, sizeof(buf) - 1);
  if(n < 0) die("read lnk");
  buf[n] = 0;
  close(fd);
  if(strcmp(buf, "hello\n") != 0) die("content mismatch through symlink");
  ok("read through symlink");

  // ── Test 4: cycle detection ───────────────────────────────
  // a -> b -> a  (infinite loop — must fail with depth limit)
  if(symlink("b", "a") != 0) die("symlink b a");
  if(symlink("a", "b") != 0) die("symlink a b");
  fd = open("a", O_RDONLY);
  if(fd >= 0){
    close(fd);
    die("expected open(a) to fail due to loop");
  }
  ok("loop detected (open failed)");

  printf(1, "testsymlink done\n");
  exit();
}