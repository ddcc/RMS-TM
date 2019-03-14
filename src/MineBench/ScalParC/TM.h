#ifndef _TM_H
 #define _TM_H
    #if defined(TM)
    #include <dlfcn.h>
    /*it is compiled with TM*/
    #define TRANSACTION_BEGIN __transaction_atomic {
    #define TRANSACTION_END }
    #define TM_CALLABLE __attribute__((transaction_safe))
        #define TM_PURE __attribute__((transaction_pure))
    #define TM_THREAD_EXIT() void *h = dlopen(NULL, RTLD_NOW); if (h) { void (*f)() = (void (*)())dlsym(h, "_ITM_finalizeThread"); if (f) (*f)(); dlclose(h); };
    #else
    #define TRANSACTION_BEGIN
    #define TRANSACTION_END
    #define TM_CALLABLE
        #define TM_PURE
    #define TM_THREAD_EXIT()
    #endif
#endif
