#include "defines.h"
#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline uint32 AtomicCompareExchangeUInt32(uint32 volatile *Value, uint32 New, uint32 Expected)
{
    uint32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);

    return(Result);
}
inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = _InterlockedExchange64((__int64 volatile *)Value, New);

    return(Result);
}
inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    // NOTE(casey): Returns the original value _prior_ to adding
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);

    return(Result);
}    
inline u32 GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}

#elif COMPILER_LLVM
// TODO(casey): Does LLVM have real read-specific barriers yet?
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
inline u32 AtomicCompareExchangeUInt32(u32 volatile* value, u32 new_val, u32 expected) {
    u32 result = __sync_val_compare_and_swap(value, expected, new_val);
    return result;
}
inline u64 AtomicExchangeU64(u64 volatile* value, u64 new_val) {
    u64 result = __sync_lock_test_and_set(value, new_val);
    return result;
}
inline u64 AtomicAddU64(u64 volatile* value, u64 addend) {
    // NOTE: Returns the original value _prior_ to adding
    u64 result = __sync_fetch_and_add(value, addend);
    return result;
}    

inline u32 GetThreadID(void) {
    u32 thread_id;
    asm("mov %%fs:0x10,%0" : "=r"(thread_id));
    return thread_id;
}
#else
// Other compilers/platforms
#endif
    

struct TicketMutex {
    u64 volatile ticket;
    u64 volatile serving;
};

inline void BeginTicketMutex(TicketMutex* mutex) {
    u64 ticket = AtomicAddU64(&mutex->ticket, 1);
    while (ticket != mutex->serving);
}

inline void EndTicketMutex(TicketMutex* mutex) {
    AtomicAddU64(&mutex->serving, 1);
}