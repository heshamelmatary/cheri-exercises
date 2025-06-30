# Answers - Exercise an inter-global-object buffer overflow

2. Expected output:
   ```
   # run_qemu ./buffer-overflow-global-baseline
   c = c
   c = b
   # ./buffer-overflow-global-cheri
   c = c
   MON|ERROR: received message 0x00000006  badge: 0x0000000000000001  tcb cap: 0x8000000000000008
   MON|ERROR: faulting PD: buffer-overflow-global-cheri
   ...
   MON|ERROR: CHERI Security Violation: ip=0x0000000000200064  fault_addr=0x0000000000203380  fsr=0x0000000000000814  (data fault)
   MON|ERROR: description of fault: Bounds violation
   MON|ERROR: CHERI fault type: CHERI data fault due to load, store or AMO
   ```

3. Example session:
   ```
    ...
    (gdb) break *0x0000000000200064 if $a0 == 0x203380
    Breakpoint 3 at 0x200064: file buffer-overflow-global.c, line 16.
    (gdb) c
    Continuing.

    Breakpoint 3, fill_buf (buf=<optimized out>, len=<optimized out>) at buffer-overflow-global.c:16
    16                      buf[i] = 'b';
    (gdb) disassemble
    Dump of assembler code for function fill_buf:
       0x0000000000200058 <+0>:     addi    a1,a1,1
       0x000000000020005a <+2>:     seqz    a2,a1
       0x000000000020005e <+6>:     add     a1,a1,a2
       0x0000000000200060 <+8>:     li      a2,98
    => 0x0000000000200064 <+12>:    sb      a2,0(ca0)
       0x0000000000200068 <+16>:    addi    a1,a1,-1
       0x000000000020006a <+18>:    add     ca0,ca0,1
       0x000000000020006e <+22>:    bnez    a1,0x200064 <fill_buf+12>
       0x0000000000200070 <+24>:    ret
    End of assembler dump.
    (gdb) p $ca0
    $5 = () 0x203380 <c> [V:1111:C:rw.C.l..:1:.:0x203300-0x203380]
    (gdb) si
    MON|ERROR: received message 0x00000006  badge: 0x0000000000000001  tcb cap: 0x8000000000000008
    MON|ERROR: faulting PD: buffer-overflow-global-cheri
    Registers:
    ddc : 0x0
    pcc : 0x200064 [rxCM1111,0x200000-0x204000] (capmode)
    cra : 0x2000ea [rxCM1111,0x200000-0x204000] (capmode) (sentry)
    csp : 0x3fffffef30 [rwCM1111,0x3fffffe000-0x3ffffff000]
    cgp : 0x0
    cs0 : 0x2026f7 [rCM1111,0x2026f7-0x2026ff]
    cs1 : 0x203380 [rwCM1111,0x203380-0x203381]
    cs2 : 0x0
    cs3 : 0x203000 [rwCM1111,0x203000-0x203010]
    cs4 : 0x0
    cs5 : 0x0
    cs6 : 0x0
    cs7 : 0x0
    cs8 : 0x0
    cs9 : 0x0
    cs10 : 0x0
    cs11 : 0x0
    ca0 : 0x203380 [rwCM1111,0x203300-0x203380]
    ca1 : 0x1
    ca2 : 0x62
    ca3 : 0xffffffffffffffff
    ca4 : 0x0
    ca5 : 0x0
    ca6 : 0x0
    ca7 : 0xfffffffffffffff4
    ct0 : 0x0
    ct1 : 0x200084 [rxCM1111,0x200000-0x202dc0] (capmode)
    ct2 : 0x0
    ct3 : 0x0
    ct4 : 0x0
    ct5 : 0x0
    ct6 : 0x0
    ctp : 0x0
    MON|ERROR: CHERI Security Violation: ip=0x0000000000200064  fault_addr=0x0000000000203380  fsr=0x0000000000000814  (data fault)
    MON|ERROR: description of fault: Bounds violation
    MON|ERROR: CHERI fault type: CHERI data fault due to load, store or AMO
    <<seL4(CPU 0) [receiveIPC/142 T0xffffffc0fffe6400 "rootserver" @8a000460]: Reply object already has unexecuted reply!>>
   ```
   The array has been incremented beyond the end of the allocation as out
   of bounds store has been attempted (`Bounds violation`).

5. Expected output:
   ```
   # run_qemu ./buffer-overflow-global-cheri
   c = c
   c = c
   ```
   To see why this occurs, examine the bounds of the buffer in `fill_buf`.
   ```
   gdb ./buffer-overflow-global-cheri
   ...
   Reading symbols from ./buffer-overflow-global-cheri...
   (gdb) target remote :1234
   Remote debugging using :1234
   0x0000000000001000 in ?? ()
   (gdb) break fill_buf
   Breakpoint 1 at 0x200058: file buffer-overflow-global.c, line 15.
   (gdb) c
   ...
   c = c

   Breakpoint 1, fill_buf (buf=0x204000 <buffer> [V:1111:C:rw.C.l..:1:.:0x204000-0x304800] "", len=1048577) at buffer-overflow-global.c:15
   15              for (size_t i = 0; i <= len; i++)
   (gdb)
   ```
   This indicates that buffer has been allocated (1024 * 1026) bytes. This
   is due to the padding required to ensure that the bounds of `buffer`
   don't overlap with other allocations. As a result, there as an area beyond
   the end of the C-language object that is nonetheless in bounds.

6. Solution:
   ```diff
   --- buffer-overflow-global.c
   +++ buffer-overflow-global.c
   @@ -6,7 +6,7 @@ char c;
    void
    fill_buf(char *buf, size_t len)
    {
   -       for (size_t i = 0; i <= len; i++)
   +       for (size_t i = 0; i < len; i++)
                   buf[i] = 'b';
    }
   ```

7. Expected output:
   ```
   # run_qemu ./buffer-overflow-global-cheri
   c = c
   c = c
   ```
