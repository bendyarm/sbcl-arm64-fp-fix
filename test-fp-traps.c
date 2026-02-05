/* test-fp-traps.c - Test floating-point trap handling on Linux ARM64
 *
 * Compile: gcc -o test-fp-traps test-fp-traps.c -lm
 * Run: ./test-fp-traps
 *
 * Expected output on working system:
 *   Enabling FP traps...
 *   Testing overflow...
 *   SUCCESS: Caught SIGFPE for overflow
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <fenv.h>
#include <setjmp.h>
#include <math.h>

static sigjmp_buf jump_buffer;
static volatile int got_sigfpe = 0;

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

    printf("Enabling FP traps...\n");
    feenableexcept(FE_OVERFLOW | FE_DIVBYZERO | FE_INVALID);

    printf("Testing overflow...\n");
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
