  .text
  .code 16

.section .iwram, "ax", %progbits;
.arm;
.align 4;
.global fast_set;
.type fast_set STT_FUNC;

fast_set:
  /* r0: *dest */
  /* r1: v */
  /* r2: len */
  push {r4-r11}
  add r2, r0, r2
  mov r3, r1
  mov r4, r1
  mov r5, r1
  mov r6, r1
  mov r7, r1
  mov r8, r1
  mov r9, r1
  mov r10, r1
  mov r11, r1
fast_set_lup:
  stmia r0!, {r1,r3,r4,r5,r6,r7,r8,r9,r10,r11} /* 10 */
  stmia r0!, {r1,r3,r4,r5,r6,r7,r8,r9,r10,r11} /* 20 */
  stmia r0!, {r1,r3,r4,r5,r6,r7,r8,r9,r10,r11} /* 30 */
  stmia r0!, {r1,r3,r4,r5,r6,r7,r8,r9,r10,r11} /* 40 */
  cmp r0, r2
  BNE fast_set_lup
  pop {r4-r11}
  bx lr

.section .iwram, "ax", %progbits;
.arm;
.align 4;
.global fast_copy;
.type fast_copy STT_FUNC;

fast_copy:
  /* r0: *dest */
  /* r1: *src */
  /* r2: len */
  push {r4-r10}
  add r2, r0, r2
fast_copy_lup:
  ldmia r1!, {r3,r4,r5,r6,r7,r8,r9,r10}
  stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}
  cmp r0, r2
  BNE fast_copy_lup
  pop {r4-r10}
  bx lr

.section .iwram, "ax", %progbits;
.arm;
.align 4;
.global fast_copy_texcol;
.type fast_copy_texcol STT_FUNC;

fast_copy_texcol:
  /* r0: *dest */
  /* r1: *src */
  push {r4-r9}
  ldmia r1!, {r2,r3,r4,r5,r6,r7,r8,r9}
  stmia r0!, {r2,r3,r4,r5,r6,r7,r8,r9}
  ldmia r1!, {r2,r3,r4,r5,r6,r7,r8,r9}
  stmia r0!, {r2,r3,r4,r5,r6,r7,r8,r9}

  pop {r4-r9}
  bx lr

.section .iwram, "ax", %progbits;
.arm;
.align 4;
.global fast_copy_texcols;
.type fast_copy_texcols STT_FUNC;

fast_copy_texcols:
  /* r0: *dest */
  /* r1: *src1 */
  /* r2: *src2 */
  push {r4-r10}
  ldmia r1!, {r3,r4,r5,r6,r7,r8,r9,r10}
  stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}
  ldmia r1!, {r3,r4,r5,r6,r7,r8,r9,r10}
  stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}
  ldmia r2!, {r3,r4,r5,r6,r7,r8,r9,r10}
  stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}
  ldmia r2!, {r3,r4,r5,r6,r7,r8,r9,r10}
  stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}

  pop {r4-r10}
  bx lr

