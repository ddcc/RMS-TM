#ifndef	_TM_H
 #define _TM_H
 #if defined(TM)
   /*it is compiled with TM*/
   #define TRANSACTION_BEGIN __tm_atomic {
   #define TRANSACTION_END }
   #define TM_CALLABLE __attribute__((tm_callable))
   #define TM_PURE __attribute__((tm_pure))
 #else
    #define TRANSACTION_BEGIN
    #define TRANSACTION_END
    #define TM_CALLABLE
 #endif
#endif