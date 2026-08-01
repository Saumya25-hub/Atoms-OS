# 🏛️ Terminal.BOSX V1.0 — Native Console Host & Command Runtime

## Architecture Overview

Terminal.BOSX is the official native command-line environment of ATOMS OS.
It is a pure **Console Host + Command Runtime** — it owns ZERO kernel functionality.

### Dependency Chain
```
Terminal.BOSX
      │
      ├── USER32.sll      (Console window, input)
      ├── COMCTL32.sll    (Console controls)
      ├── SHELL32.sll     (ShellExecute, file ops)
      ├── KERNEL32.sll    (Process, file, env)
      ├── ADVAPI32.sll    (Env persistence)
      ├── BOSLL.sll       (Runtime, memory stats)
      └── WS2_32.sll      (Network commands)
            │
            ▼
         ATOMS Kernel
```

## Built-in Commands (40+)

| Category | Commands |
|---|---|
| Files | dir, ls, cd, pwd, mkdir, rmdir, copy, move, del, rename, type |
| Process | start, tasklist, taskkill, run |
| System | cls, echo, ver, hostname, whoami, time, date, exit |
| Environment | set, unset, env, path |
| Network | ping, ipconfig, netstat |
| Diagnostics | mem, cpu, handles, uptime, diagnostics |
| Shell | help, history, alias, clearhistory, reload |
