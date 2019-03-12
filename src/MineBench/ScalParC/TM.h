#ifndef _TM_H
 #define _TM_H
    #if defined(TM)
    __attribute__((weak)) void _ITM_finalizeThread();

    /*it is compiled with TM*/
    #define TRANSACTION_BEGIN __transaction_atomic {
    #define TRANSACTION_END }
    #define TM_CALLABLE __attribute__((transaction_safe))
        #define TM_PURE __attribute__((transaction_pure))
    #define TM_THREAD_EXIT() if (_ITM_finalizeThread) _ITM_finalizeThread()
    #else
    #define TRANSACTION_BEGIN
    #define TRANSACTION_END
    #define TM_CALLABLE
        #define TM_PURE
    #define TM_THREAD_EXIT()
    #endif
#endif
