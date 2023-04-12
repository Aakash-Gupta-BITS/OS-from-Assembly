  // BOOTSTRAP AREA
  // SP = 256
  @256
  D = A
  @SP
  M = D

  // Call Sys.init
  @Sys.init
  0; JMP

(Sys.init)
  // LCL = ARG = SP
  @SP
  D = A
  @LCL
  M = D

  // USER CODE

  // call Main.main 0

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1
  @FUNC_CALL_Main.main_1_RET  // push FUNC_CALL_Main.main_1_RET
  D = A
  @SP
  AM = M + 1
  A = A - 1
  M = D
  @LCL  // push LCL
  D = M
  @SP
  AM = M + 1
  A = A - 1
  M = D
  @ARG  // push ARG
  D = M
  @SP
  AM = M + 1
  A = A - 1
  M = D
  @THIS  // push THIS
  D = M
  @SP
  AM = M + 1
  A = A - 1
  M = D
  @THAT  // push THAT
  D = M
  @SP
  AM = M + 1
  A = A - 1
  M = D
  @SP  // LCL = SP
  D = M
  @LCL
  M = D
  @6  // ARG = SP - 5 - nArgs
  D = D - A
  @ARG
  M = D
  @Main.main // JUMP to function
  0; JMP
(FUNC_CALL_Main.main_1_RET)
  A = -1
  0; JMP

  // function Main.main 0
(Main.main)

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // push constant 0
  @0  // D = 0
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

  // push constant 1
  @1  // D = 1
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

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

  // push constant 9
  @9  // D = 9
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

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

  // push constant 2
  @2  // D = 2
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // pop local 3
  @LCL  // addr <- LCL + 3
  D = M
  @3
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

  // push local 3
  @LCL  // addr <- LCL + 3
  D = M
  @3
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

 // eq
  @SP  // Load values
  AM = M - 1
  D = M
  A = A - 1
  D = D - M
  @ALU_COMPARE_1_ELSE  // Store in D
  D;JNE
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

  // push local 0
  @LCL  // addr <- LCL + 0
  D = M
  @0
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
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

  // push local 0
  @LCL  // addr <- LCL + 0
  D = M
  @0
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

 // sub
  @SP
  AM = M - 1
  D = M
  A = A - 1
  M = M - D

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

  // push constant 1
  @1  // D = 1
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // push local 3
  @LCL  // addr <- LCL + 3
  D = M
  @3
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

  // pop local 3
  @LCL  // addr <- LCL + 3
  D = M
  @3
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

  // pop static 0
  @program.0  // addr <- program.0 + 0
  D = A
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

  // return
  @SP  // MEM[ARG] <- MEM[SP - 1]
  A = M - 1
  D = M
  @ARG
  A = M
  M = D
  D = A + 1  // SP = ARG + 1
  @SP
  M = D
  @LCL  // R15 <- return address
  D = M
  @5
  A = D - A
  D = M
  @R15
  M = D
  @LCL  // pop THAT using LCL
  AM = M - 1
  D = M
  @THAT
  M = D
  @LCL  // pop THIS using LCL
  AM = M - 1
  D = M
  @THIS
  M = D
  @LCL  // pop ARG using LCL
  AM = M - 1
  D = M
  @ARG
  M = D
  @LCL  // pop LCL using LCL
  AM = M - 1
  D = M
  @LCL
  M = D
  @R15  // jump to return address
  A = M
  0;JMP
