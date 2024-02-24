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

  // function sum 1
(sum)

  // push argument 0
  @ARG  // addr <- ARG + 0
  D = M
  @0
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

 // if-goto REST
  @SP
  AM = M - 1
  D = M
  @REST
  D; JNE

  // push constant 0
  @0  // D = 0
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

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

 // label REST
(REST)

  // push argument 0
  @ARG  // addr <- ARG + 0
  D = M
  @0
  A = D + A
  D = M
  @SP  // SP++
  M = M + 1
  A = M  // RAM[addr] <- RAM[SP - 1]
  A = A - 1
  M = D

  // push argument 0
  @ARG  // addr <- ARG + 0
  D = M
  @0
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

 // sub
  @SP
  AM = M - 1
  D = M
  A = A - 1
  M = M - D

  // call sum 1
  @FUNC_CALL_sum_2_RET  // push FUNC_CALL_sum_2_RET
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
  @sum // JUMP to function
  0; JMP
(FUNC_CALL_sum_2_RET)

 // add
  @SP
  AM = M - 1
  D = M
  A = A - 1
  M = D + M

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

  // function Main.main 0
(Main.main)

  // push constant 10
  @10  // D = 10
  D = A
  @SP  // RAM[SP] = D
  A = M
  M = D
  @SP  // SP++
  M = M + 1

  // call sum 1
  @FUNC_CALL_sum_3_RET  // push FUNC_CALL_sum_3_RET
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
  @sum // JUMP to function
  0; JMP
(FUNC_CALL_sum_3_RET)

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