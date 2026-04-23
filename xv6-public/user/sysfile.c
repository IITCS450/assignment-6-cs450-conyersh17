
int
sys_symlink(void)
{
  char target[MAXPATH], linkpath[MAXPATH];
  struct inode *ip;

  if(argstr(0, target,   MAXPATH) < 0 ||
     argstr(1, linkpath, MAXPATH) < 0)
    return -1;

  begin_op();

  ip = create(linkpath, T_SYMLINK, 0, 0);
  if(ip == 0){
    end_op();
    return -1;
  }

  if(writei(ip, 0, (uint64)target, 0, strlen(target) + 1) < 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  iunlockput(ip);
  end_op();
  return 0;
}



#define MAX_SYMLINK_DEPTH 10
#define MAXPATH 128

int
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    // follow symlinks (up to 10 deep to detect cycles)
    int depth = 0;
    while(ip->type == T_SYMLINK){
      if(depth++ >= 10){
        iunlockput(ip);
        end_op();
        return -1;  // cycle detected
      }
      char target[MAXPATH];
      int n = readi(ip, target, 0, sizeof(target)-1);
      iunlockput(ip);
      if(n < 0){
        end_op();
        return -1;
      }
      target[n] = 0;
      if((ip = namei(target)) == 0){
        end_op();
        return -1;
      }
      ilock(ip);
    }
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  // ── symlink resolution loop ───────────────────────────────
  {
    char symtarget[MAXPATH];
    int  depth = 0;

    while(ip->type == T_SYMLINK){
      if(depth >= MAX_SYMLINK_DEPTH){
        iunlockput(ip);
        end_op();
        return -1;
      }

      int nr = readi(ip, 0, (uint64)symtarget, 0, MAXPATH - 1);
      iunlockput(ip);          
        end_op();
        return -1;
      }
      symtarget[nr] = '\0';  
      ip = namei(symtarget);
      if(ip == 0){
        end_op();
        return -1;
      }
      ilock(ip);
      depth++;
    }
  }

  if(ip->type == T_DIR && omode != O_RDONLY){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE){
    f->type  = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off  = 0;
  }

  f->ip       = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;

  int
sys_symlink(void)
{
  char *target, *path;
  struct inode *ip;

  if(argstr(0, &target) < 0 || argstr(1, &path) < 0)
    return -1;

  begin_op();
  ip = create(path, T_SYMLINK, 0, 0);
  if(ip == 0){
    end_op();
    return -1;
  }
  // store target path in inode data
  if(writei(ip, target, 0, strlen(target)) != strlen(target)){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}