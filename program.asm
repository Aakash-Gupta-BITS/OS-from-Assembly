@10
D = A
@LCL
M = D

@256
D = A
@SP
M = D

  // push constant 5
  @5  // D = 5
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // pop local 0
  @LCL  // addr <- LCL + 0
  D = M
  @0
  D = D + A
  @R15
  M = D
  @SP  // SP--
  M = M - 1
  A = M  // RAM[addr] <- RAM[SP]
  D = M
  @R15
  A = M
  M = D

 // label START
(START)

  // push local 1
  @LCL  // addr <- LCL + 1
  D = M
  @1
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

  // push constant 5
  @5  // D = 5
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

 // gt
  @SP  // Load values
  AM = M - 1
  D = M
  A = A - 1
  D = D - M
  @ALU_COMPARE_1_ELSE  // Store in D
  D;JGE
  D = 1
  @ALU_COMPARE_1_FINISH
  0; JMP
(ALU_COMPARE_1_ELSE)
  D = 0
(ALU_COMPARE_1_FINISH)
  @SP // Store in memory
  A = M - 1
  M = D

 // if-goto END
  @SP
  AM = M - 1
  D = M
  @END
  D; JNE

  // push local 1
  @LCL  // addr <- LCL + 1
  D = M
  @1
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

  // push local 2
  @LCL  // addr <- LCL + 2
  D = M
  @2
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

 // add
  @SP
  AM = M - 1
  D = M
  A = A - 1
  M = D + M

  // pop local 2
  @LCL  // addr <- LCL + 2
  D = M
  @2
  D = D + A
  @R15
  M = D
  @SP  // SP--
  M = M - 1
  A = M  // RAM[addr] <- RAM[SP]
  D = M
  @R15
  A = M
  M = D

  // push local 1
  @LCL  // addr <- LCL + 1
  D = M
  @1
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

  // push constant 1
  @1  // D = 1
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

 // add
  @SP
  AM = M - 1
  D = M
  A = A - 1
  M = D + M

  // pop local 1
  @LCL  // addr <- LCL + 1
  D = M
  @1
  D = D + A
  @R15
  M = D
  @SP  // SP--
  M = M - 1
  A = M  // RAM[addr] <- RAM[SP]
  D = M
  @R15
  A = M
  M = D

 // goto START
  @START
  0; JMP

 // label END
(END)
A=-1
0;JMP
