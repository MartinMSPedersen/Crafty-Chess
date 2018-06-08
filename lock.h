#if defined(SMP)

#if (defined(NT_i386) || defined(NT_AXP))

#  define pthread_attr_t  HANDLE
#  define pthread_t       HANDLE
#  define thread_t        HANDLE
#  define tfork(t,f,p)    do {                                             \
                            (pthread_t)_beginthreadex(0,0,(void *)(f),(void *)(p),0,0);  \
                          } while (0)

#if (defined (_M_ALPHA) && !defined(NT_INTEREX))
   
#  ifdef __cplusplus
      extern "C" __int64 __asm (char *, ...);
      extern "C" void __MB (void);
#     pragma intrinsic (__asm)
#     pragma intrinsic (__MB)
#  endif

  typedef volatile int lock_t[1];

#  define LockInit(v)      ((v)[0] = 0)
#  define UnLock(v)        (__MB(), (v)[0] = 0)

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
#  define UnLock(v)        ((v)[0] = 0)


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


#else /* NT non-Alpha/Intel, without assembler Lock() */

#  define lock_t           volatile int
#  define LockInit(v)      ((v) = 0)
#  define Lock(v)          do {                                         \
                             while(InterlockedExchange((LPLONG)&(v),1) != 0);  
\
                           } while (0)
#  define UnLock(v)        ((v) = 0)

#endif /* architecture check */

#else  /* not NT, assume SMP using POSIX threads (LINUX, etc)  */

#if defined(MUTEX)

#  define Lock(v)          pthread_mutex_lock(&v)
#  define LockInit(v)      pthread_mutex_init(&v,0)
#  define UnLock(v)        pthread_mutex_unlock(&v)
#  define lock_t           pthread_mutex_t

#elif defined(ALPHA)

#  include <machine/builtins.h>

#  define lock_t           volatile long
#  define LockInit(v)      ((v) = 0)
#  define Lock(v)          __LOCK_LONG(&(v))
#  define UnLock(v)        __UNLOCK_LONG(&(v))

#else /* POSIX, but not using MUTEXes */

#  define exchange(adr,reg)                                  \
     ({ volatile int _ret;                                   \
     asm volatile ("xchgw %0,%1"                             \
     : "=q" (_ret), "=m" (*(adr))    /* Output %0,%1 */      \
     : "m"  (*(adr)), "0"  (reg));   /* Input (%2),%0 */     \
     _ret;                                                   \
     })
#  define LockInit(p)           (p=0)
#  define UnLock(p)             (exchange(&p,0))
#  define Lock(p)               while(exchange(&p,1)) while(p)
#  define lock_t                volatile int

#endif /* MUTEX */

#if defined(CLONE)
#  define tfork(t,f,p)   {                                            \
                         char *m=malloc(0x100010);                    \
                         if (m <= 0) printf("malloc() failed\n");     \
                         (void) clone(f,                            \
                         m+0x100000,                                  \
                         CLONE_VM+CLONE_FILES+17,                     \
                         (void*) p); }
                           
#else
#  define tfork(t,f,p)          pthread_create(&t,&pthread_attr,f,(void*) p)
#endif

#endif /* NT or POSIX */

#else
#  define LockInit(p)
#  define Lock(p)
#  define UnLock(p)
#endif /*  SMP code */

