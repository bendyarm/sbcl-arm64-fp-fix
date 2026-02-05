# Plan: Fix SBCL ARM64 Linux Floating-Point Traps

## Problem

SBCL on Linux ARM64 doesn't support floating-point exception traps because
`os_restore_fp_control()` in `src/runtime/arm64-linux-os.c` is unimplemented:

```c
void os_restore_fp_control(os_context_t *context)
{
    /* FIXME: Implement. */
}
```

This causes ACL2 to fail with:
```
This Lisp is unsuitable for ACL2, because it failed a check that
floating-point overflow causes an error.
```

## Solution

Implement `os_restore_fp_control()` for ARM64 Linux by:
1. Finding the fpsimd_context in the signal context's `__reserved` area
2. Reading the FPCR value from it
3. Restoring FPCR using the `msr fpcr, <reg>` instruction

## Linux ARM64 Signal Context Structure

```
ucontext_t
  └── uc_mcontext
        ├── fault_address
        ├── regs[31]  (general purpose registers)
        ├── sp
        ├── pc
        ├── pstate
        └── __reserved[4096]  <-- fpsimd_context is here
              └── fpsimd_context (magic = 0x46508001)
                    ├── head.magic (uint32)
                    ├── head.size (uint32)
                    ├── fpsr (uint32)   -- status register
                    └── fpcr (uint32)   -- control register
```

## Test Strategy

### Test 1: C-level test (test-fp-traps.c)
Verify that Linux ARM64 can detect FP overflow via signals.

### Test 2: SBCL test (test-sbcl-fp.lisp)
Test SBCL's floating-point trap handling after patching.

### Test 3: ACL2 build test
Verify ACL2 builds successfully with patched SBCL.

## Files

1. `arm64-linux-os.c.patch` - The SBCL patch
2. `test-fp-traps.c` - C-level FP trap test
3. `test-sbcl-fp.lisp` - SBCL FP trap test
4. `.github/workflows/test-arm64-fp.yml` - GitHub Actions workflow

## Testing Platform

GitHub Actions `ubuntu-24.04-arm` runner (native ARM64 Linux).

## References

- Linux kernel: arch/arm64/kernel/signal.c
- Linux kernel: arch/arm64/include/uapi/asm/sigcontext.h
- SBCL: src/runtime/x86-64-linux-os.c (working implementation)
- SBCL: src/runtime/arm64-linux-os.c (broken implementation)
