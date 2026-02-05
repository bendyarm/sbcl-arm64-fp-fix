/* test-fp-traps-direct.c - Test ARM64 FP traps using direct FPCR manipulation
 *
 * This bypasses glibc's feenableexcept() to directly set FPCR trap enable bits.
 *
 * FPCR exception enable bits (AArch64):
 *   Bit 8:  IOE - Invalid Operation Enable
 *   Bit 9:  DZE - Divide by Zero Enable
 *   Bit 10: OFE - Overflow Enable
 *   Bit 11: UFE - Underflow Enable
 *   Bit 12: IXE - Inexact Enable
 *   Bit 15: IDE - Input Denormal Enable
 *
 * Compile: gcc -o test-fp-traps-direct test-fp-traps-direct.c -lm
 * Run: ./test-fp-traps-direct
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <stdint.h>

static sigjmp_buf jump_buffer;
static volatile int got_sigfpe = 0;

/* FPCR bit definitions */
#define FPCR_IOE (1 << 8)   /* Invalid Operation Enable */
#define FPCR_DZE (1 << 9)   /* Divide by Zero Enable */
#define FPCR_OFE (1 << 10)  /* Overflow Enable */
#define FPCR_UFE (1 << 11)  /* Underflow Enable */
#define FPCR_IXE (1 << 12)  /* Inexact Enable */
#define FPCR_IDE (1 << 15)  /* Input Denormal Enable */

static inline uint64_t get_fpcr(void) {
    uint64_t fpcr;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
    return fpcr;
}

static inline void set_fpcr(uint64_t fpcr) {
    __asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));
}

static void sigfpe_handler(int sig, siginfo_t *info, void *ucontext) {
    got_sigfpe = 1;
    siglongjmp(jump_buffer, 1);
}

int main() {
    struct sigaction sa;
    sa.sa_sigaction = sigfpe_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGFPE, &sa, NULL);

    uint64_t fpcr_before = get_fpcr();
    printf("FPCR before: 0x%016llx\n", (unsigned long long)fpcr_before);

    /* Enable overflow, divide-by-zero, and invalid operation traps */
    uint64_t fpcr_new = fpcr_before | FPCR_OFE | FPCR_DZE | FPCR_IOE;
    printf("Setting FPCR to: 0x%016llx\n", (unsigned long long)fpcr_new);
    set_fpcr(fpcr_new);

    uint64_t fpcr_after = get_fpcr();
    printf("FPCR after:  0x%016llx\n", (unsigned long long)fpcr_after);

    /* Check if the bits actually stuck */
    if ((fpcr_after & (FPCR_OFE | FPCR_DZE | FPCR_IOE)) != (FPCR_OFE | FPCR_DZE | FPCR_IOE)) {
        printf("WARNING: FPCR trap bits did not stick!\n");
        printf("  OFE: %s\n", (fpcr_after & FPCR_OFE) ? "enabled" : "NOT enabled");
        printf("  DZE: %s\n", (fpcr_after & FPCR_DZE) ? "enabled" : "NOT enabled");
        printf("  IOE: %s\n", (fpcr_after & FPCR_IOE) ? "enabled" : "NOT enabled");
    } else {
        printf("Trap bits successfully set.\n");
    }

    printf("\nTesting overflow (1e308 * 1e308)...\n");
    got_sigfpe = 0;

    if (sigsetjmp(jump_buffer, 1) == 0) {
        volatile double big = 1e308;
        volatile double result = big * big;  /* Should overflow */
        (void)result;
        printf("FAIL: No SIGFPE caught (result=%g)\n", result);
        return 1;
    } else {
        printf("SUCCESS: Caught SIGFPE for overflow\n");
        return 0;
    }
}
