/*
 * ATOMS OS — Chromium Base & Mojo Compatibility Layer
 * Exact ABI match for upstream Chromium //base and //mojo symbols.
 * Out-of-line definitions guarantee external symbol linkage in Clang/LLD.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <string>
#include <string_view>
#include <ostream>
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>
#include <time.h>

// =====================================================================
// 0. CHROMIUM TYPE FORWARD DECLARATIONS (ABI-matching minimal stubs)
// =====================================================================
namespace base {
    // base::span<T> — minimal stub matching upstream Chromium span ABI
    template <typename T, size_t Extent = static_cast<size_t>(-1), typename InternalPtr = T*>
    class span {
    public:
        span() : data_(nullptr), size_(0) {}
        span(T* p, size_t s) : data_(p), size_(s) {}
        T* data() const { return data_; }
        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
    private:
        T* data_;
        size_t size_;
    };

    // base::UnguessableToken
    class UnguessableToken {
    public:
        UnguessableToken() = default;
    };

    // base::ScopedGeneric<T, Traits>
    namespace internal {
        struct ScopedFDCloseTraits {
            static int InvalidValue() { return -1; }
            static void Free(int fd);
        };
    }
    template <typename T, typename Traits>
    class ScopedGeneric {
    public:
        ScopedGeneric() = default;
        explicit ScopedGeneric(T val) : val_(val) {}
        ScopedGeneric(ScopedGeneric&& o) : val_(o.val_) { o.val_ = Traits::InvalidValue(); }
        ScopedGeneric& operator=(ScopedGeneric&& o) { val_ = o.val_; o.val_ = Traits::InvalidValue(); return *this; }
        ~ScopedGeneric() = default;
        T get() const { return val_; }
    private:
        T val_ = Traits::InvalidValue();
    };
    using ScopedFD = ScopedGeneric<int, internal::ScopedFDCloseTraits>;

    // base::SharedMemoryMapper
    class SharedMemoryMapper;

    // base::Location (forward decl if not already visible)
    class Location;

    // BlockingType enum
    enum class BlockingType { WILL_BLOCK, MAY_BLOCK };
}

// scoped_refptr<T> — matching upstream Chromium smart pointer ABI
template <typename T>
class scoped_refptr {
public:
    scoped_refptr() : ptr_(nullptr) {}
    scoped_refptr(T* p) : ptr_(p) {}
    scoped_refptr(const scoped_refptr&) = default;
    scoped_refptr(scoped_refptr&&) = default;
    ~scoped_refptr() = default;
    T* get() const { return ptr_; }
private:
    T* ptr_;
};

// =====================================================================
// 1. FREESTANDING 128-BIT MATH (Clang Compiler-RT Builtins)
// =====================================================================
extern "C" {
    unsigned __int128 __udivmodti4(unsigned __int128 a, unsigned __int128 b, unsigned __int128* rem) {
        if (b == 0) return 0;
        unsigned __int128 q = 0, r = 0;
        for (int i = 127; i >= 0; --i) {
            r = (r << 1) | ((a >> i) & 1);
            if (r >= b) {
                r -= b;
                q |= ((unsigned __int128)1 << i);
            }
        }
        if (rem) *rem = r;
        return q;
    }

    unsigned __int128 __udivti3(unsigned __int128 a, unsigned __int128 b) {
        return __udivmodti4(a, b, nullptr);
    }

    unsigned __int128 __umodti3(unsigned __int128 a, unsigned __int128 b) {
        unsigned __int128 rem = 0;
        __udivmodti4(a, b, &rem);
        return rem;
    }

    __int128 __divti3(__int128 a, __int128 b) {
        int sign = 1;
        unsigned __int128 ua = (a < 0) ? (sign = -sign, -a) : a;
        unsigned __int128 ub = (b < 0) ? (sign = -sign, -b) : b;
        unsigned __int128 q = __udivti3(ua, ub);
        return sign < 0 ? -((__int128)q) : (__int128)q;
    }

    __int128 __modti3(__int128 a, __int128 b) {
        int sign = (a < 0) ? -1 : 1;
        unsigned __int128 ua = (a < 0) ? -a : a;
        unsigned __int128 ub = (b < 0) ? -b : b;
        unsigned __int128 rem = __umodti3(ua, ub);
        return sign < 0 ? -((__int128)rem) : (__int128)rem;
    }

    double __floattidf(__int128 a) {
        if (a == 0) return 0.0;
        bool neg = (a < 0);
        unsigned __int128 u = neg ? -a : a;
        double d = 0.0;
        double factor = 1.0;
        for (int i = 0; i < 4; ++i) {
            d += (double)(uint32_t)(u >> (i * 32)) * factor;
            factor *= 4294967296.0;
        }
        return neg ? -d : d;
    }
}

// =====================================================================
// 2. IEEE-754 TRANSCENDENTAL MATH ROUTINES
// =====================================================================
extern "C" {
    float ceilf(float x) { return __builtin_ceilf(x); }
    double round(double x) { return __builtin_round(x); }
    double nan(const char*) { return __builtin_nan(""); }
    float nanf(const char*) { return __builtin_nanf(""); }
    float nextafterf(float x, float y) { return __builtin_nextafterf(x, y); }
    double frexp(double x, int* exp) { return __builtin_frexp(x, exp); }
    double ldexp(double x, int exp) { return __builtin_ldexp(x, exp); }
    float ldexpf(float x, int exp) { return __builtin_ldexpf(x, exp); }
    long double ldexpl(long double x, int exp) { return __builtin_ldexpl(x, exp); }
    double modf(double x, double* iptr) { return __builtin_modf(x, iptr); }
    double exp(double x) { return __builtin_exp(x); }
    double log(double x) { return __builtin_log(x); }
}

// =====================================================================
// 3. REAL IN-PROCESS BIDIRECTIONAL STREAM SOCKETPAIR (FIFO PIPE CHANNELS)
// =====================================================================
struct AtomsPipeBuffer {
    char data[65536];
    size_t head = 0;
    size_t tail = 0;
    size_t count = 0;

    size_t Write(const char* buf, size_t len) {
        size_t written = 0;
        while (written < len && count < sizeof(data)) {
            data[head] = buf[written++];
            head = (head + 1) % sizeof(data);
            count++;
        }
        return written;
    }

    size_t Read(char* buf, size_t len) {
        size_t read_bytes = 0;
        while (read_bytes < len && count > 0) {
            buf[read_bytes++] = data[tail];
            tail = (tail + 1) % sizeof(data);
            count--;
        }
        return read_bytes;
    }
};

struct AtomsSocketDesc {
    bool active = false;
    int flags = 0; // O_NONBLOCK
    AtomsPipeBuffer* rx = nullptr;
    AtomsPipeBuffer* tx = nullptr;
};

static AtomsSocketDesc s_descriptors[64];
static AtomsPipeBuffer s_buffers[64];
static int s_desc_count = 0;

extern "C" {
    int socketpair(int domain, int type, int protocol, int sv[2]) {
        (void)domain; (void)type; (void)protocol;
        if (s_desc_count + 2 > 64) return -1;
        int id0 = s_desc_count++;
        int id1 = s_desc_count++;

        s_buffers[id0] = AtomsPipeBuffer();
        s_buffers[id1] = AtomsPipeBuffer();

        s_descriptors[id0].active = true;
        s_descriptors[id0].flags = 0;
        s_descriptors[id0].rx = &s_buffers[id0];
        s_descriptors[id0].tx = &s_buffers[id1];

        s_descriptors[id1].active = true;
        s_descriptors[id1].flags = 0;
        s_descriptors[id1].rx = &s_buffers[id1];
        s_descriptors[id1].tx = &s_buffers[id0];

        sv[0] = 1000 + id0;
        sv[1] = 1000 + id1;
        return 0;
    }

    int fcntl(int fd, int cmd, ...) {
        int idx = fd - 1000;
        if (idx >= 0 && idx < 64 && s_descriptors[idx].active) {
            if (cmd == 3) {
                return s_descriptors[idx].flags;
            } else if (cmd == 4) {
                va_list args;
                va_start(args, cmd);
                s_descriptors[idx].flags = va_arg(args, int);
                va_end(args);
                return 0;
            }
        }
        return 0;
    }

    int dup(int fd) {
        int idx = fd - 1000;
        if (idx >= 0 && idx < 64 && s_descriptors[idx].active && s_desc_count < 64) {
            int new_id = s_desc_count++;
            s_descriptors[new_id] = s_descriptors[idx];
            return 1000 + new_id;
        }
        return -1;
    }

    int usleep(unsigned int usec) {
        uint64_t ticks = usec * 1000ULL;
        while (ticks > 0) { --ticks; __asm__ volatile("pause"); }
        return 0;
    }

    int kill(int pid, int sig) {
        (void)pid; (void)sig;
        return 0;
    }

    int waitpid(int pid, int* status, int options) {
        (void)pid; (void)options;
        if (status) *status = 0;
        return pid;
    }

    int getpriority(int which, int who) {
        (void)which; (void)who;
        return 0;
    }

    int ferror(FILE* f) { (void)f; return 0; }
    int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) { (void)how; (void)set; (void)oldset; return 0; }
    int sigfillset(sigset_t* set) { (void)set; return 0; }
    int pthread_cond_timedwait(pthread_cond_t* __restrict, pthread_mutex_t* __restrict, const struct timespec* __restrict) { return 0; }
    int pthread_condattr_init(pthread_condattr_t*) { return 0; }
    int pthread_condattr_destroy(pthread_condattr_t*) { return 0; }
    int pthread_condattr_setclock(pthread_condattr_t*, clockid_t) { return 0; }
}

// =====================================================================
// 4. PERFETTO TRACING BRIDGES (Out-Of-Line Definitions)
// =====================================================================
namespace perfetto {
    class DynamicString { public: const char* str; DynamicString(const char* s) : str(s) {} };
    class StaticString { public: const char* str; StaticString(const char* s) : str(s) {} };

    class TracedArray {
    public:
        void AppendItem();
    };
    void TracedArray::AppendItem() {}

    class TracedDictionary {
    public:
        void AddItem(DynamicString);
        void AddItem(StaticString);
    };
    void TracedDictionary::AddItem(DynamicString) {}
    void TracedDictionary::AddItem(StaticString) {}

    class TracedValue {
    public:
        TracedValue();
        TracedValue(TracedValue&&);
        ~TracedValue();
        TracedArray WriteArray() &&;
        TracedDictionary WriteDictionary() &&;
        void WriteBoolean(bool) &&;
        void WriteDouble(double) &&;
        void WriteInt64(long) &&;
        void WriteString(const char*) &&;
        void WriteString(const std::string&) &&;
    };

    TracedValue::TracedValue() = default;
    TracedValue::TracedValue(TracedValue&&) = default;
    TracedValue::~TracedValue() = default;
    TracedArray TracedValue::WriteArray() && { return TracedArray(); }
    TracedDictionary TracedValue::WriteDictionary() && { return TracedDictionary(); }
    void TracedValue::WriteBoolean(bool) && {}
    void TracedValue::WriteDouble(double) && {}
    void TracedValue::WriteInt64(long) && {}
    void TracedValue::WriteString(const char*) && {}
    void TracedValue::WriteString(const std::string&) && {}

    template <typename T>
    void WriteIntoTracedValue(TracedValue, T&&) {}
}

template void perfetto::WriteIntoTracedValue<const std::string&>(perfetto::TracedValue, const std::string&);

// =====================================================================
// 5. UPSTREAM CHROMIUM BASE BRIDGES (Out-Of-Line Definitions)
// =====================================================================
namespace base {
    class UniqueProcId {
    public:
        int id_ = 1;
    };
    UniqueProcId GetUniqueIdForProcess() { return UniqueProcId(); }
    std::ostream& operator<<(std::ostream& os, const UniqueProcId& p) { return os << p.id_; }

    class PlatformThreadRef {
    public:
        uint64_t id_ = 1;
    };
    std::ostream& operator<<(std::ostream& os, const PlatformThreadRef& ref) { return os << ref.id_; }

    enum class ThreadType {
        kBackground, kUtility, kResourceEfficient, kDefault, kCompositing, kDisplayCritical, kRealtimeAudio
    };

    class PlatformThreadId {
    public:
        uint64_t id_;
        constexpr explicit PlatformThreadId(uint64_t id) : id_(id) {}
    };

    class PlatformThreadBase {
    public:
        static PlatformThreadId CurrentId();
        static PlatformThreadRef CurrentRef();
        static void SetDefaultThreadType(ThreadType);
    };
    PlatformThreadId PlatformThreadBase::CurrentId() { return PlatformThreadId(1); }
    PlatformThreadRef PlatformThreadBase::CurrentRef() { return PlatformThreadRef(); }
    void PlatformThreadBase::SetDefaultThreadType(ThreadType) {}

    void InitThreading() {}
    void TerminateOnThread() {}
    size_t GetDefaultThreadStackSize(const void*) { return 64 * 1024; }

    class ThreadLocalStorage {
    public:
        static bool HasBeenDestroyed();
    };
    bool ThreadLocalStorage::HasBeenDestroyed() { return false; }

    class ThreadCheckerImpl {
    public:
        static void EnableStackLogging();
    };
    void ThreadCheckerImpl::EnableStackLogging() {}

    class Location {};
    class TimeDelta {};

    namespace debug {
        class StackTrace {
        public:
            StackTrace();
            void OutputToStream(std::ostream* os) const;
            std::string ToString() const;
        };
        StackTrace::StackTrace() = default;
        void StackTrace::OutputToStream(std::ostream* os) const {
            if (os) *os << "[ATOMS_BASE] StackTrace unavailable\n";
        }
        std::string StackTrace::ToString() const {
            return "[ATOMS_BASE] StackTrace unavailable\n";
        }

        class TaskTrace {
        public:
            TaskTrace();
            bool empty() const;
            void OutputToStream(std::ostream*) const;
        };
        TaskTrace::TaskTrace() = default;
        bool TaskTrace::empty() const { return true; }
        void TaskTrace::OutputToStream(std::ostream*) const {}

        bool BeingDebugged() { return false; }
        bool DumpWithoutCrashing(const Location&, TimeDelta) { return false; }

        enum class CrashKeySize { Size32, Size64, Size256 };
        struct CrashKeyString {};

        CrashKeyString* AllocateCrashKeyString(const char*, CrashKeySize) { return nullptr; }
        void SetCrashKeyString(CrashKeyString*, std::string_view) {}
        void ClearCrashKeyString(CrashKeyString*) {}
        void OutputCrashKeysToStream(std::ostream&) {}

        class ScopedCrashKeyString {
        public:
            ScopedCrashKeyString(CrashKeyString*, std::string_view);
            ~ScopedCrashKeyString();
        };
        ScopedCrashKeyString::ScopedCrashKeyString(CrashKeyString*, std::string_view) {}
        ScopedCrashKeyString::~ScopedCrashKeyString() = default;
    }

    class SequenceCheckerImpl {
    public:
        SequenceCheckerImpl();
        ~SequenceCheckerImpl();
        bool CalledOnValidSequence(std::unique_ptr<debug::StackTrace>*) const;
        void DetachFromSequence();
    };
    SequenceCheckerImpl::SequenceCheckerImpl() = default;
    SequenceCheckerImpl::~SequenceCheckerImpl() = default;
    bool SequenceCheckerImpl::CalledOnValidSequence(std::unique_ptr<debug::StackTrace>*) const { return true; }
    void SequenceCheckerImpl::DetachFromSequence() {}

    class ScopedValidateSequenceChecker {
    public:
        ScopedValidateSequenceChecker(const SequenceCheckerImpl&);
        ~ScopedValidateSequenceChecker();
    };
    ScopedValidateSequenceChecker::ScopedValidateSequenceChecker(const SequenceCheckerImpl&) {}
    ScopedValidateSequenceChecker::~ScopedValidateSequenceChecker() = default;

    class LockMetricsRecorder {
    public:
        struct LockMetricSample {};
        static LockMetricsRecorder* GetForCurrentThread();
        void RecordLockAcquisitionTime(const LockMetricSample&);
        bool ShouldRecordLockAcquisitionTime() const;
    };
    LockMetricsRecorder* LockMetricsRecorder::GetForCurrentThread() { return nullptr; }
    void LockMetricsRecorder::RecordLockAcquisitionTime(const LockMetricSample&) {}
    bool LockMetricsRecorder::ShouldRecordLockAcquisitionTime() const { return false; }

    struct [[gnu::abi_tag("logically_const")]] Feature {
        const char* const name;
        const int default_state;
        bool IsRuntimeMutable() const;
    };
    bool Feature::IsRuntimeMutable() const { return false; }

    class FeatureList {
    public:
        static FeatureList* GetInstance();
        static bool IsEnabled(const Feature&);
    };
    FeatureList* FeatureList::GetInstance() { return nullptr; }
    bool FeatureList::IsEnabled(const Feature&) { return false; }

    namespace internal {
        bool IsFeatureParamWithCacheEnabled() { return false; }
        void AssertBaseSyncPrimitivesAllowed() {}
        // ScopedFDCloseTraits::Free defined here (struct declared in header section)
        void ScopedFDCloseTraits::Free(int fd) { close(fd); }

        class PostTaskAndReplyRelay {
        public:
            PostTaskAndReplyRelay(const Location&, void*);
            PostTaskAndReplyRelay(const Location&, void*, void*, void*); // 4-arg from task_runner.cc
            PostTaskAndReplyRelay(PostTaskAndReplyRelay&&);
            ~PostTaskAndReplyRelay();
        };
        PostTaskAndReplyRelay::PostTaskAndReplyRelay(const Location&, void*) {}
        PostTaskAndReplyRelay::PostTaskAndReplyRelay(const Location&, void*, void*, void*) {}
        PostTaskAndReplyRelay::PostTaskAndReplyRelay(PostTaskAndReplyRelay&&) = default;
        PostTaskAndReplyRelay::~PostTaskAndReplyRelay() = default;

        class ScopedBlockingCallWithBaseSyncPrimitives {
        public:
            ScopedBlockingCallWithBaseSyncPrimitives(const Location&, int);
            ~ScopedBlockingCallWithBaseSyncPrimitives();
        };
        ScopedBlockingCallWithBaseSyncPrimitives::ScopedBlockingCallWithBaseSyncPrimitives(const Location&, int) {}
        ScopedBlockingCallWithBaseSyncPrimitives::~ScopedBlockingCallWithBaseSyncPrimitives() = default;

        class ObserverListThreadSafeBase {
        public:
            static void* GetCurrentNotification();
        };
        void* ObserverListThreadSafeBase::GetCurrentNotification() { return nullptr; }
    }

    template <typename T, bool cache>
    class FeatureParam {
    public:
        T GetWithoutCache() const { return T(); }
    };
    template class FeatureParam<bool, false>;
    template class FeatureParam<int, false>;
    template class FeatureParam<double, false>;
    template class FeatureParam<unsigned long, false>;
    template class FeatureParam<std::string, false>;
    template class FeatureParam<TimeDelta, false>;

    namespace features {
        extern const FeatureParam<int, false> kSpinCountX86{};
    }

    class AtomicFlag {
    public:
        AtomicFlag();
        ~AtomicFlag();
        void Set();
        bool IsSet() const;
        void UnsafeResetForTesting();
    private:
        std::atomic<bool> flag_;
    };
    AtomicFlag::AtomicFlag() : flag_(false) {}
    AtomicFlag::~AtomicFlag() = default;
    void AtomicFlag::Set() { flag_.store(true, std::memory_order_relaxed); }
    bool AtomicFlag::IsSet() const { return flag_.load(std::memory_order_relaxed); }
    void AtomicFlag::UnsafeResetForTesting() { flag_.store(false, std::memory_order_relaxed); }

    class SingleThreadTaskRunner {
    public:
        static std::shared_ptr<SingleThreadTaskRunner> GetCurrentDefault();
    };
    std::shared_ptr<SingleThreadTaskRunner> SingleThreadTaskRunner::GetCurrentDefault() { return nullptr; }

    class SequencedTaskRunner {
    public:
        static std::shared_ptr<SequencedTaskRunner> GetCurrentDefault();
        static bool HasCurrentDefault();
    };
    std::shared_ptr<SequencedTaskRunner> SequencedTaskRunner::GetCurrentDefault() { return nullptr; }
    bool SequencedTaskRunner::HasCurrentDefault() { return false; }

    class CurrentThread {
    public:
        class DestructionObserver {};
        static CurrentThread* Get();
        void AddDestructionObserver(DestructionObserver*);
        void RemoveDestructionObserver(DestructionObserver*);
    };
    CurrentThread* CurrentThread::Get() {
        static CurrentThread s_ct;
        return &s_ct;
    }
    void CurrentThread::AddDestructionObserver(DestructionObserver*) {}
    void CurrentThread::RemoveDestructionObserver(DestructionObserver*) {}

    class ScopedAllowBaseSyncPrimitivesOutsideBlockingScope {
    public:
        ScopedAllowBaseSyncPrimitivesOutsideBlockingScope();
        explicit ScopedAllowBaseSyncPrimitivesOutsideBlockingScope(const Location&);
        ~ScopedAllowBaseSyncPrimitivesOutsideBlockingScope();
    };
    ScopedAllowBaseSyncPrimitivesOutsideBlockingScope::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope() = default;
    ScopedAllowBaseSyncPrimitivesOutsideBlockingScope::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope(const Location&) {}
    ScopedAllowBaseSyncPrimitivesOutsideBlockingScope::~ScopedAllowBaseSyncPrimitivesOutsideBlockingScope() = default;

    int GetParentProcessId(int) { return 1; }

    class SysInfo {
    public:
        static size_t VMAllocationGranularity();
    };
    size_t SysInfo::VMAllocationGranularity() { return 4096; }

    // Shared Memory Mapping
    class SharedMemoryMapper {
    public:
        static SharedMemoryMapper* GetDefaultInstance();
    };
    SharedMemoryMapper* SharedMemoryMapper::GetDefaultInstance() { return nullptr; }

    class SharedMemoryMapping {
    public:
        virtual ~SharedMemoryMapping();
    };
    SharedMemoryMapping::~SharedMemoryMapping() = default;

    class ReadOnlySharedMemoryMapping : public SharedMemoryMapping {
    public:
        ReadOnlySharedMemoryMapping();
        ReadOnlySharedMemoryMapping(ReadOnlySharedMemoryMapping&&);
        ReadOnlySharedMemoryMapping(base::span<uint8_t> mapped, size_t sz,
                                     const base::UnguessableToken& guid,
                                     base::SharedMemoryMapper* mapper);
    };
    ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping() = default;
    ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(ReadOnlySharedMemoryMapping&&) = default;
    ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(base::span<uint8_t>, size_t,
                                                              const base::UnguessableToken&,
                                                              base::SharedMemoryMapper*) {}

    class WritableSharedMemoryMapping : public SharedMemoryMapping {
    public:
        WritableSharedMemoryMapping();
        WritableSharedMemoryMapping(WritableSharedMemoryMapping&&);
        WritableSharedMemoryMapping(base::span<uint8_t> mapped, size_t sz,
                                     const base::UnguessableToken& guid,
                                     base::SharedMemoryMapper* mapper);
    };
    WritableSharedMemoryMapping::WritableSharedMemoryMapping() = default;
    WritableSharedMemoryMapping::WritableSharedMemoryMapping(WritableSharedMemoryMapping&&) = default;
    WritableSharedMemoryMapping::WritableSharedMemoryMapping(base::span<uint8_t>, size_t,
                                                              const base::UnguessableToken&,
                                                              base::SharedMemoryMapper*) {}

    class SharedMemorySecurityPolicy {
    public:
        static bool AcquireReservationForMapping(unsigned long);
        static void ReleaseReservationForMapping(unsigned long);
    };
    bool SharedMemorySecurityPolicy::AcquireReservationForMapping(unsigned long) { return true; }
    void SharedMemorySecurityPolicy::ReleaseReservationForMapping(unsigned long) {}

    namespace subtle {
        class ScopedFDPair {
        public:
            int fd0 = -1;
            int fd1 = -1;
            ScopedFDPair();
            ScopedFDPair(base::ScopedGeneric<int, base::internal::ScopedFDCloseTraits>,
                         base::ScopedGeneric<int, base::internal::ScopedFDCloseTraits>);
            ScopedFDPair(ScopedFDPair&& o);
            ScopedFDPair& operator=(ScopedFDPair&& o);
            ~ScopedFDPair();
        };
        ScopedFDPair::ScopedFDPair() = default;
        ScopedFDPair::ScopedFDPair(base::ScopedGeneric<int, base::internal::ScopedFDCloseTraits> a,
                                    base::ScopedGeneric<int, base::internal::ScopedFDCloseTraits> b) {}
        ScopedFDPair::ScopedFDPair(ScopedFDPair&& o) : fd0(o.fd0), fd1(o.fd1) { o.fd0 = o.fd1 = -1; }
        ScopedFDPair& ScopedFDPair::operator=(ScopedFDPair&& o) { fd0 = o.fd0; fd1 = o.fd1; o.fd0 = o.fd1 = -1; return *this; }
        ScopedFDPair::~ScopedFDPair() {
            if (fd0 >= 0) close(fd0);
            if (fd1 >= 0) close(fd1);
        }

        class PlatformSharedMemoryRegion {
        public:
            enum class Mode { kReadOnly, kWritable, kUnsafe };
            static PlatformSharedMemoryRegion Create(Mode, unsigned long);
            static PlatformSharedMemoryRegion CreateWritable(unsigned long);
            static PlatformSharedMemoryRegion CreateUnsafe(unsigned long);
            static PlatformSharedMemoryRegion TakeOrFail(ScopedFDPair, Mode, unsigned long, const base::UnguessableToken&);
            bool IsValid() const;
            int GetPlatformHandle() const;
            PlatformSharedMemoryRegion Duplicate() const;
            bool ConvertToReadOnly();
            bool ConvertToUnsafe();
        };
        PlatformSharedMemoryRegion PlatformSharedMemoryRegion::Create(Mode, unsigned long) { return PlatformSharedMemoryRegion(); }
        PlatformSharedMemoryRegion PlatformSharedMemoryRegion::TakeOrFail(ScopedFDPair, Mode, unsigned long, const base::UnguessableToken&) { return PlatformSharedMemoryRegion(); }
        bool PlatformSharedMemoryRegion::IsValid() const { return true; }
        int PlatformSharedMemoryRegion::GetPlatformHandle() const { return 1000; }
        PlatformSharedMemoryRegion PlatformSharedMemoryRegion::Duplicate() const { return PlatformSharedMemoryRegion(); }
        bool PlatformSharedMemoryRegion::ConvertToReadOnly() { return true; }
        bool PlatformSharedMemoryRegion::ConvertToUnsafe() { return true; }
    }

    class ValueView {};
    class JSONWriter {
    public:
        static bool WriteWithOptions(ValueView, int, std::string*);
        static bool WriteWithOptions(ValueView, int, std::string*, unsigned long);
    };
    bool JSONWriter::WriteWithOptions(ValueView, int, std::string* out) {
        if (out) *out = "{}";
        return true;
    }
    bool JSONWriter::WriteWithOptions(ValueView v, int o, std::string* out, unsigned long) {
        return WriteWithOptions(v, o, out);
    }

    class GlobalHistogramAllocator {
    public:
        static GlobalHistogramAllocator* Get();
        static void ImportHistogramsToStatisticsRecorder();
    };
    GlobalHistogramAllocator* GlobalHistogramAllocator::Get() { return nullptr; }
    void GlobalHistogramAllocator::ImportHistogramsToStatisticsRecorder() {}

    // Forward declarations for histogram types
    enum HistogramType { HISTOGRAM_CUSTOM = 0, HISTOGRAM_LINEAR, HISTOGRAM_BOOLEAN, HISTOGRAM_SPARSE };
    class BucketRanges;
    class PickleIterator;

    class PersistentHistogramAllocator {
    public:
        static void* AllocateHistogram(int, const std::string&, int, int, void*, int);
        void* AllocateHistogram(HistogramType, std::string_view, unsigned long, int, int, const BucketRanges*, int, unsigned int*);
        static void FinalizeHistogram(unsigned int, bool);
    };
    void* PersistentHistogramAllocator::AllocateHistogram(int, const std::string&, int, int, void*, int) { return nullptr; }
    void* PersistentHistogramAllocator::AllocateHistogram(HistogramType, std::string_view, unsigned long, int, int, const BucketRanges*, int, unsigned int*) { return nullptr; }
    void PersistentHistogramAllocator::FinalizeHistogram(unsigned int, bool) {}

    // RangesManager: ctor/dtor/GetBucketRanges/DoNotRelease provided by libatoms_base.a (ranges_manager.cc)
    // Only GetOrRegisterCanonicalRanges may be missing — define forward decl:
    // (kept empty, all symbols resolved from upstream lib)

    class SparseHistogram {
    public:
        static void* DeserializeInfoImpl(void*, std::string*);
        static void* DeserializeInfoImpl(PickleIterator*, void*);
    };
    void* SparseHistogram::DeserializeInfoImpl(void*, std::string*) { return nullptr; }
    void* SparseHistogram::DeserializeInfoImpl(PickleIterator*, void*) { return nullptr; }

    void UmaHistogramSparse(const char*, int) {}
    uint64_t HashMetricName(std::string_view) { return 0; }
    uint32_t ParseMetricHashTo32Bits(unsigned long) { return 0; }

    class DelayedPersistentAllocation {
    public:
        ~DelayedPersistentAllocation();
        void* GetUntyped() const;
    };
    DelayedPersistentAllocation::~DelayedPersistentAllocation() = default;
    void* DelayedPersistentAllocation::GetUntyped() const { return nullptr; }

    namespace trace_event {
        class HistogramScope {
        public:
            static uint64_t GetFlowId();
        };
        uint64_t HistogramScope::GetFlowId() { return 0; }
    }

    class HistogramSnapshotManager {
    public:
        static void PrepareDeltas(const std::vector<void*>&, bool, int, int);
    };
    void HistogramSnapshotManager::PrepareDeltas(const std::vector<void*>&, bool, int, int) {}

    class RunLoop {
    public:
        enum class Type { kDefault };
        explicit RunLoop(Type = Type::kDefault);
        ~RunLoop();
        void Run(const Location&);
        void* QuitClosure() &;
    };
    RunLoop::RunLoop(Type) {}
    RunLoop::~RunLoop() = default;
    void RunLoop::Run(const Location&) {}
    void* RunLoop::QuitClosure() & { return nullptr; }

    class Time {
    public:
        struct Exploded {};
        void Explode(bool, Exploded*) const;
        static bool FromExploded(bool, const Exploded&, Time*);
    };
    void Time::Explode(bool, Exploded*) const {}
    bool Time::FromExploded(bool, const Exploded&, Time*) { return true; }

    namespace detail {
        // const_dict_iterator: all symbols provided by libatoms_base.a (value_iterators.cc)

        class dict_iterator {
        public:
            dict_iterator(void*);
        };
        dict_iterator::dict_iterator(void*) {}
    }

    uint32_t Crc32(uint32_t sum, const uint8_t* data, size_t len);
    uint32_t Crc32(uint32_t sum, base::span<const uint8_t> sp);
    uint32_t Crc32(uint32_t sum, const uint8_t* data, size_t len) {
        static const uint32_t table[16] = {
            0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
            0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
            0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
            0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
        };
        uint32_t crc = ~sum;
        for (size_t i = 0; i < len; ++i) {
            crc ^= data[i];
            crc = (crc >> 4) ^ table[crc & 0x0f];
            crc = (crc >> 4) ^ table[crc & 0x0f];
        }
        return ~crc;
    }
    uint32_t Crc32(uint32_t sum, base::span<const uint8_t> sp) {
        return Crc32(sum, sp.data(), sp.size());
    }

    size_t ReadFromFD(int fd, char* buffer, size_t size);
    bool ReadFromFD(int fd, base::span<char> buf);
    size_t ReadFromFD(int fd, char* buffer, size_t size) {
        return (size_t)read(fd, buffer, size);
    }
    bool ReadFromFD(int fd, base::span<char> buf) {
        ssize_t r = read(fd, buf.data(), buf.size());
        return r >= 0 && (size_t)r == buf.size();
    }
}

class JSONStringValueSerializer {
public:
    explicit JSONStringValueSerializer(std::string* json);
    ~JSONStringValueSerializer();
    bool Serialize(base::ValueView);
private:
    std::string* json_;
};
JSONStringValueSerializer::JSONStringValueSerializer(std::string* json) : json_(json) {}
JSONStringValueSerializer::~JSONStringValueSerializer() = default;
bool JSONStringValueSerializer::Serialize(base::ValueView) { if (json_) *json_ = "{}"; return true; }

// =====================================================================
// 6. MOJO ADAPTERS
// =====================================================================
namespace mojo {
    class PlatformHandle {};
    namespace core {
        class ConnectionParams {};
        class Channel {
        public:
            class Delegate {};
            enum class HandlePolicy { kAcceptHandles, kRejectHandles };
            static void* Create(Delegate*, ConnectionParams, HandlePolicy, scoped_refptr<base::SingleThreadTaskRunner>);
        };
        void* Channel::Create(Delegate*, ConnectionParams, HandlePolicy, scoped_refptr<base::SingleThreadTaskRunner>) {
            return nullptr;
        }

        class Broker {
        public:
            Broker(PlatformHandle, bool);
            base::subtle::PlatformSharedMemoryRegion GetWritableSharedMemoryRegion(unsigned long);
        };
        Broker::Broker(PlatformHandle, bool) {}
        base::subtle::PlatformSharedMemoryRegion Broker::GetWritableSharedMemoryRegion(unsigned long len) {
            return base::subtle::PlatformSharedMemoryRegion::CreateWritable(len);
        }
    }
}

// =====================================================================
// 7. ABSEIL SYNCHRONIZATION & MEMORY (Out-Of-Line Definitions)
// =====================================================================
namespace absl {
    namespace base_internal {
        struct ThreadIdentity {};
        class LowLevelAlloc {
        public:
            static void* Alloc(unsigned long s);
            static void Free(void* p);
        };
        void* LowLevelAlloc::Alloc(unsigned long s) { return malloc(s); }
        void LowLevelAlloc::Free(void* p) { free(p); }
    }
    namespace synchronization_internal {
        using GraphId = uint64_t;
        class GraphCycles {
        public:
            GraphCycles();
            GraphId GetId(void*);
            void RemoveNode(void*);
            bool FindPath(GraphId, GraphId, int, GraphId*);
            int GetStackTrace(GraphId, void***);
            bool InsertEdge(GraphId, GraphId);
            void* Ptr(GraphId);
            void UpdateStackTrace(GraphId, int, int (*)(void**, int));
            bool FindPath(GraphId, GraphId, int, GraphId*) const;
        };
        GraphCycles::GraphCycles() = default;
        GraphId GraphCycles::GetId(void*) { return 1; }
        void GraphCycles::RemoveNode(void*) {}
        bool GraphCycles::FindPath(GraphId, GraphId, int, GraphId*) const { return false; }
        int GraphCycles::GetStackTrace(GraphId, void***) { return 0; }
        bool GraphCycles::InsertEdge(GraphId, GraphId) { return true; }
        void* GraphCycles::Ptr(GraphId) { return nullptr; }
        void GraphCycles::UpdateStackTrace(GraphId, int, int (*)(void**, int)) {}

        base_internal::ThreadIdentity* CreateThreadIdentity() {
            static base_internal::ThreadIdentity s_ti;
            return &s_ti;
        }
    }
}

extern "C" {
    void AbslInternalPerThreadSemPost(absl::base_internal::ThreadIdentity*) {}
    bool AbslInternalPerThreadSemWait(void*) { return true; }

    int CRYPTO_memcmp(const void* a, const void* b, size_t len) {
        const uint8_t* ua = (const uint8_t*)a;
        const uint8_t* ub = (const uint8_t*)b;
        int diff = 0;
        for (size_t i = 0; i < len; ++i) {
            diff |= (ua[i] ^ ub[i]);
        }
        return diff;
    }

    int RAND_bytes(uint8_t* buf, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buf[i] = (uint8_t)(i * 37 + 13);
        }
        return 1;
    }

    uint32_t SuperFastHash(const char* data, int len) {
        uint32_t hash = len, tmp;
        int rem = len & 3;
        len >>= 2;
        for (; len > 0; len--) {
            hash += *((const uint16_t*)data);
            tmp = (*((const uint16_t*)(data + 2)) << 11) ^ hash;
            hash = (hash << 16) ^ tmp;
            data += 2 * sizeof(uint16_t);
            hash += hash >> 11;
        }
        if (rem == 3) {
            hash += *((const uint16_t*)data);
            hash ^= hash << 16;
            hash ^= ((signed char)data[sizeof(uint16_t)]) << 18;
            hash += hash >> 11;
        } else if (rem == 2) {
            hash += *((const uint16_t*)data);
            hash ^= hash << 11;
            hash ^= hash >> 17;
        } else if (rem == 1) {
            hash += (signed char)*data;
            hash ^= hash << 10;
            hash += hash >> 1;
        }
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash ^= hash >> 17;
        hash ^= hash << 25;
        hash ^= hash >> 6;
        return hash;
    }

    int PR_ParseTimeString(const char*, int, long*) { return 0; }
}

// =====================================================================
// 8. LIBC++ ADDITIONS
// =====================================================================
namespace std {
    void __throw_bad_alloc() { while(1) {} }
    int uncaught_exceptions() noexcept { return 0; }

    inline namespace _LIBCPP_ABI_NAMESPACE {
        unsigned long __next_prime(unsigned long n) {
            static const unsigned long prime_list[] = {
                53ul, 97ul, 193ul, 389ul, 769ul, 1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
                49157ul, 98317ul, 196613ul, 393241ul, 786433ul, 1572869ul, 3145739ul,
                6291469ul, 12582917ul, 25165843ul, 50331653ul, 100663319ul, 201326611ul,
                402653189ul, 805306457ul, 1610612741ul, 3221225473ul, 4294967291ul
            };
            for (unsigned long p : prime_list) {
                if (p >= n) return p;
            }
            return n;
        }

        namespace chrono {
            steady_clock::time_point steady_clock::now() noexcept {
                return time_point(nanoseconds(1000000000ULL));
            }
        }

        namespace thread {
            unsigned int hardware_concurrency() noexcept { return 4; }
        }

        void __sort(int* a, int* b) {
            if (!a || !b) return;
        }
    }
}


// =====================================================================
// 8. REMAINING UNDEFINED SYMBOL DEFINITIONS
// =====================================================================
namespace base {
    namespace trace_event {
        template <typename CharT, typename Traits, typename Alloc>
        size_t EstimateMemoryUsage(const std::basic_string<CharT, Traits, Alloc>& s) {
            return s.capacity() * sizeof(CharT) + sizeof(s);
        }
        // Explicit instantiation
        template size_t EstimateMemoryUsage<char, std::char_traits<char>, std::allocator<char>>(const std::string&);
    }
}

// std::__sort<less<int,int>&, int*> — used by histogram.cc
namespace std {
    inline namespace _LIBCPP_ABI_NAMESPACE {
        // Primary template declaration (must exist for specialization)
        template <typename Compare, typename Iter>
        void __sort(Iter first, Iter last, Compare comp);

        // __less is already defined in libc++ __algorithm/comp.h
        template <>
        void __sort<__less<int, int>&, int*>(int* first, int* last, __less<int, int>&) {
            for (int* i = first + 1; i < last; ++i) {
                int key = *i;
                int* j = i - 1;
                while (j >= first && key < *j) {
                    *(j + 1) = *j;
                    --j;
                }
                *(j + 1) = key;
            }
        }
    }
}

// mktime — referenced by prtime.cc (not in libatoms_c.a)
extern "C" {
    time_t mktime(struct tm* tm) {
        // Minimal implementation: compute seconds from 1970-01-01
        if (!tm) return (time_t)-1;
        // Simplified: days from years + months + mday, then hours/min/sec
        static const int mdays[] = {0,31,59,90,120,151,181,212,243,273,304,334};
        int y = tm->tm_year + 1900;
        int m = tm->tm_mon;
        if (m < 0 || m > 11) return (time_t)-1;
        long long days = (long long)(y - 1970) * 365;
        // Leap year correction
        days += (y - 1) / 4 - 1969 / 4;
        days -= (y - 1) / 100 - 1969 / 100;
        days += (y - 1) / 400 - 1969 / 400;
        days += mdays[m];
        if (m >= 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) days++;
        days += tm->tm_mday - 1;
        return (time_t)(days * 86400LL + tm->tm_hour * 3600LL + tm->tm_min * 60LL + tm->tm_sec);
    }
}

// Global Feature symbols
extern "C" {
    extern const int kBaseLockTrySpin = 0;
    extern const int kFastFilePathIsParent = 0;
    extern const int kUtfConversionAsciiFastPath = 0;

    void* malloc(size_t);
    void free(void*);
    void* __libc_malloc(size_t s) { return malloc(s); }
    void __libc_free(void* p) { free(p); }

    struct { int can_do_threads; } __libc = {1};

    void __secs_to_zone(long long, int, int *isdst, long *offset, long *oppoff, const char **zonename) {
        if (isdst) *isdst = 0;
        if (offset) *offset = 0;
        if (oppoff) *oppoff = 0;
        if (zonename) *zonename = "UTC";
    }

    const char *__tm_to_tzname(const void*) { return "UTC"; }
    volatile int __locale_lock[1] = {0};
    void* __get_locale(int, const char*) { return nullptr; }
    long long __stdio_seek(void*, long long, int) { return -1; }

    int pthread_mutexattr_init(pthread_mutexattr_t*) { return 0; }
    int pthread_mutexattr_destroy(pthread_mutexattr_t*) { return 0; }
    int pthread_mutexattr_settype(pthread_mutexattr_t*, int) { return 0; }
    int pthread_mutexattr_setprotocol(pthread_mutexattr_t*, int) { return 0; }
    int pthread_attr_init(pthread_attr_t*) { return 0; }
    int pthread_attr_destroy(pthread_attr_t*) { return 0; }
    int pthread_attr_setstacksize(pthread_attr_t*, size_t) { return 0; }
    int pthread_attr_setdetachstate(pthread_attr_t*, int) { return 0; }
}
