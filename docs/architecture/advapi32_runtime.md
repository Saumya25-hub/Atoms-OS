# 🏛️ ADVAPI32.sll V1.0 Architecture Specification

> **Subsystem:** ADVAPI32.sll V1.0 Security, Registry & System Services Runtime  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Ring 3 Security & Service Control Layer (Above KERNEL32 & BOSLL)  

---

## 1. Executive Summary & Architectural Philosophy

**ADVAPI32.sll V1.0** is the official Ring 3 Security, Registry, Service Control, Event Logging, Cryptography, and Access Control Runtime inside **ATOMS OS**. Modeled after Windows `ADVAPI32.dll`, Service Control Manager, Registry APIs, Security Model, ReactOS, Wine, Linux PAM, systemd, and SELinux principles, ADVAPI32.sll provides the single authority for every privileged user-mode operation above BOSLL and KERNEL32.

### Core Architectural Mandates:
- **Public Security & Registry Runtime Authority**: Every Ring 3 application delegates Registry manipulation, security token validation, privilege adjustments, service management, event logging, and cryptographic hashing to ADVAPI32.sll.
- **Layered Subsystem Flow**:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │       Ring 3 Applications (Explorer, Services, Apps)        │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard ADVAPI32 API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      ADVAPI32.sll                           │
 │  ├── 1. Runtime Manager       ├── 11. Policy Manager         │
 │  ├── 2. Registry Runtime      ├── 12. LSA Runtime            │
 │  ├── 3. Security Descriptor   ├── 13. Impersonation Engine   │
 │  ├── 4. SID Manager           ├── 14. Audit Engine           │
 │  ├── 5. ACL Runtime           ├── 15. Environment Security   │
 │  ├── 6. Token Runtime         ├── 16. Reg Notifications      │
 │  ├── 7. Privilege Engine      ├── 17. Handle Security        │
 │  ├── 8. Service Control Mgr   ├── 18. Performance Runtime    │
 │  ├── 9. Event Log Runtime     ├── 19. Diagnostics Engine     │
 │  └── 10. Cryptography Runtime └── 20. 300-Test Suite         │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Subsystem Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                     KERNEL32.sll / BOSLL.sll                │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Kernel Security Syscalls
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               Kernel Security Manager & Syscall             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Monolithic Kernel
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       x86_64 Kernel                         │
 └──────────────────────────────┴──────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/advapi32/`)

```text
userspace/libs/advapi32/
├── include/
│   ├── advapi32_types.h
│   ├── advapi32_api.h
│   └── advapi32_public.h
├── core/
│   └── advapi_runtime.c
├── registry/
│   ├── advapi_registry.c
│   └── advapi_reg_notify.c
├── security/
│   └── advapi_security.c
├── sid/
│   └── advapi_sid.c
├── acl/
│   └── advapi_acl.c
├── tokens/
│   └── advapi_tokens.c
├── privilege/
│   └── advapi_privilege.c
├── services/
│   └── advapi_services.c
├── eventlog/
│   └── advapi_eventlog.c
├── crypto/
│   └── advapi_crypto.c
├── policy/
│   └── advapi_policy.c
├── lsa/
│   └── advapi_lsa.c
├── impersonation/
│   └── advapi_impersonation.c
├── audit/
│   └── advapi_audit.c
├── environment/
│   └── advapi_env.c
├── handles/
│   └── advapi_handles.c
├── performance/
│   └── advapi_performance.c
├── diagnostics/
│   └── advapi_diagnostics.c
├── tests/
│   └── advapi32_certification_tests.c
└── docs/
    └── advapi32_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Manager**: Lifecycle management, subsystem init (`AdvApiInitialize`, `AdvApiShutdown`).
2. **Registry Runtime**: Registry hierarchy manipulation (`RegCreateKeyEx`, `RegOpenKeyEx`, `RegCloseKey`, `RegDeleteKey`, `RegQueryValueEx`, `RegSetValueEx`).
3. **Security Descriptor Engine**: ACL & Security descriptors (`InitializeSecurityDescriptor`, `SetSecurityDescriptorOwner`, `SetSecurityDescriptorDacl`).
4. **SID Manager**: Security Identifier allocation & lookup (`AllocateAndInitializeSid`, `EqualSid`, `FreeSid`, `LookupAccountSid`).
5. **ACL Runtime**: Access Control List management (`InitializeAcl`, `AddAccessAllowedAce`, `CheckTokenMembership`).
6. **Token Runtime**: Process & Thread tokens (`OpenProcessToken`, `OpenThreadToken`, `DuplicateToken`).
7. **Privilege Engine**: Privilege adjustment & lookup (`AdjustTokenPrivileges`, `LookupPrivilegeValue`).
8. **Service Control Manager**: Service manager lifecycle (`OpenSCManager`, `CreateService`, `StartService`, `ControlService`, `DeleteService`, `CloseServiceHandle`).
9. **Event Log Runtime**: System & Audit Event Logging (`RegisterEventSource`, `ReportEvent`, `DeregisterEventSource`).
10. **Cryptography Runtime**: Crypto provider & random number generator (`CryptAcquireContext`, `CryptGenRandom`, `CryptCreateHash`, `CryptHashData`, `CryptEncrypt`, `CryptDecrypt`).
11. **Policy Manager**: System security policy loading and caching.
12. **LSA Runtime**: Local Security Authority communication and authentication.
13. **Impersonation Engine**: Identity switching (`ImpersonateLoggedOnUser`, `RevertToSelf`).
14. **Audit Engine**: Security event auditing for logins and objects.
15. **Environment Security**: Protected environment variables and secure configuration storage.
16. **Registry Notification Engine**: Registry key/value monitoring callbacks.
17. **Handle Security Manager**: Secure handle allocation and leak detection.
18. **Performance Runtime**: Fast registry and permission lookup caches.
19. **Diagnostics Engine**: Security, registry, and service diagnostics audit.
20. **Certification Battery**: 300-test production certification suite (`advapi32_certification_tests.c`).
