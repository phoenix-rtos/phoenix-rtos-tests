# ld.elf_so tests
Ported from NetBSD

## Porting notes
While porting be cautious about test cases not performing cleanup (e.g. `dlclose`) as `ATF` provides test case isolation using fork.
