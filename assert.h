#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
#define assert(x) (void)0
#else
#define assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__), 0)))
#endif

void __attribute__((noreturn)) __assert_fail(const char* expr, const char* file, unsigned int line, const char* function);

#endif /* ASSERT_H */
