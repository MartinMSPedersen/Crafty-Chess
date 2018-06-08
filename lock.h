#if defined(SMP)

#if defined(_WIN32) || defined(_WIN64)

#  define pthread_attr_t  HANDLE
#  define pthread_t       HANDLE
#  define thread_t        HANDLE
#  define tfork(t,f,p)    WinStartThread((void *)(f),(void *)(p))

extern pthread_t WinStartThread(void * func, void * args);

#if (defined (_M_ALPHA) && !defined(NT_INTEREX))
   
#  ifdef __cplusplus
      extern "C" __int64 __asm (char *, ...);
      extern "C" void __MB (void);
#     pragma intrinsic (__asm)
#     pragma intrinsic (__MB)
#  endif

  typedef volatile int lock_t[1];

#  define LockInit(v)      ((v)[0] = 0)
#  define LockFree(v)      ((v)[0] = 0)
#  define Unlock(v)        (__MB(), (v)[0] = 0)
#  define Pause()

   __inline void Lock (volatile int *hPtr) {
   __asm ("lp: ldl_l v0,(a0);"
          "    xor v0,1,v0;"
          "    beq v0,lp;"
          "    stl_c v0,(a0);"
          "    beq v0,lp;"
          "    mb;",
                  hPtr);
   }

#elif (defined (_M_IX86) && !defined(NT_INTEREX))

   typedef volatile int lock_t[1];

#  define LockInit(v)      ((v)[0] = 0)
#  define LockFree(v)      ((v)[0] = 0)
#  define Unlock(v)        ((v)[0] = 0)
#  define Pause()


#  if (_MSC_VER > 1200)

#     ifdef __cplusplus
        extern "C" long _InterlockedExchange (long*, long);
#     else
        extern long _InterlockedExchange (long*, long);
#     endif
#     pragma intrinsic (_InterlockedExchange)

   __forceinline void Lock (volatile int *hPtr)
    {
    int iValue;
    volatile int *hPtrTmp;

    hPtrTmp = hPtr;     // Workaround for vc7 beta1 bug
    iValue = 1;
    for (;;)
        {
        iValue = _InterlockedExchange ((long*) hPtrTmp, iValue);
        if (0 == iValue)
            return;
        while (*hPtrTmp)
            ;   // Do nothing
        }
    }

#  else

__inline void Lock (volatile int *hPtr)
    {
    __asm
      {
        mov     ecx, hPtr
   la:  mov     eax, 1
        xchg    eax, [ecx]
        test    eax, eax
        jz      end
   lb:  mov     eax, [ecx]
        test    eax, eax
        jz      la
        jmp     lb
   end:
      }
    }

#  endif

#elif ((defined (_M_IA64) || defined (_M_AMD64)) && !defined(NT_INTEREX))

#  include <windows.h>

#  pragma intrinsic (_InterlockedExchange)

   typedef volatile LONG lock_t[1];

#  define LockInit(v)      ((v)[0] = 0)
#  define LockFree(v)      ((v)[0] = 0)
#  define Unlock(v)        ((v)[0] = 0)
#  define Pause()


__forceinline void Lock (volatile LONG *hPtr)
    {
    int iValue;

    for (;;)
        {
        iValue = _InterlockedExchange ((LPLONG) hPtr, 1);
        if (0 == iValue)
            return;
        while (*hPtr)
            ;   // Do nothing
        }
    }


#else /* NT non-Alpha/Intel, without assembler Lock() */

#  define lock_t           volatile int
#  define LockInit(v)      ((v) = 0)
#  define LockFree(v)      ((v) = 0)
#  define Lock(v)          do {                                         \
                             while(InterlockedExchange((LPLONG)&(v),1) != 0);  \
                           } while (0)
#  define Unlock(v)        ((v) = 0)
#  define Pause()

#endif /* architecture check */

#else  /* not NT, assume SMP using POSIX threads (LINUX, etc)  */

#if defined(MUTEX)

#  define Lock(v)          pthread_mutex_lock(&v)
#  define LockInit(v)      pthread_mutex_init(&v,0)
#  define LockFree(v)      pthread_mutex_destroy(&v)
#  define Unlock(v)        pthread_mutex_unlock(&v)
#  define lock_t           pthread_mutex_t

#elif defined(ALPHA)

#  include <machine/builtins.h>

#  define lock_t           volatile long
#  define LockInit(v)      ((v) = 0)
#  define LockFree(v)      ((v) = 0)
#  define Lock(v)          __LOCK_LONG(&(v))
#  define Unlock(v)        __UNLOCK_LONG(&(v))
#  define Pause()

#else /* POSIX, but not using MUTEXes */

#  define Pause()                                            \
     ({                                                      \
     asm volatile ("pause");                                 \
     })

static void __inline__ LockX86(volatile int * lock) {
	int dummy;
        asm __volatile__ (
            "1:          movl    $1, %0"           "\n\t"
            "            xchgl   (%1), %0"         "\n\t"
            "            testl   %0, %0"           "\n\t"
            "            jz      3f"               "\n\t"
            "2:          pause"                    "\n\t"
            "            movl    (%1), %0"         "\n\t"
            "            testl   %0, %0"           "\n\t"
            "            jnz     2b"               "\n\t"
            "            jmp     1b"               "\n\t"
            "3:"                                   "\n\t"
	    : "=&q" (dummy)
	    : "q" (lock)
	    : "cc");
}

static void __inline__ UnlockX86(volatile int * lock) {
	int dummy;
        asm __volatile__ ("movl    $0, (%1)"
	    : "=&q" (dummy)
	    : "q" (lock)
	    );
}
#  define LockInit(p)           (p=0)
#  define LockFree(p)           (p=0)
#  define Unlock(p)             (UnlockX86(&p))
#  define Lock(p)               (LockX86(&p))
#  define lock_t                volatile int

#endif /* MUTEX */

#if defined(CLONE)
#  define tfork(t,f,p)   {                                            \
                         char *m=malloc(0x10000f);                    \
                         m=(void*) (((size_t) m+15)&~15);             \
                         if (m <= 0) printf("malloc() failed\n");     \
                         pids[smp_threads]=clone(f,                   \
                         m+0x100000,                                  \
                         CLONE_VM+CLONE_FILES,                        \
                         (void*) p); }
                           
#else
#  define tfork(t,f,p)          pthread_create(&t,&pthread_attr,f,(void*) p)
#endif

#endif /* NT or POSIX */

#else
#  define LockInit(p)
#  define LockFree(p)
#  define Lock(p)
#  define Unlock(p)
#  define Pause()
#endif /*  SMP code */

