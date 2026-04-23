─────────────────────────────────────────────────────────────

#define SYS_symlink 22


extern uint64 sys_symlink(void);

[SYS_symlink] sys_symlink,


entry("symlink");
───────────────────────────────────────────────────────────

int symlink(const char*, const char*);


#define T_SYMLINK 4