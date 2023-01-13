# OS-from-Assembly
There are many projects. One which simulates the binary file is completed.

## Binary Simulator
### Running
1. Compilation has to be done via the following command:

   ```{bash}
   g++ -o simulator.out CPU.cpp
   ```
   
2. To run the file generated, do the following:

   ```
   ./simulator.out <instruction_file_location> <memory_file_location> <memory_dump_location>
   ```

### File Format
Both memory and instruction files have one instruction per line, with each instruction in binary format. For example, we can have the following instruction file:
```
0000 0000 0000 0000
1111 1100 0001 0000
0000 0000 0000 0001
1111 0000 1001 0000
0000 0000 0000 0000
1110 0011 0000 1000
1110 1110 1010 0000
1110 1010 1000 0111

```
and the following data file:
```
0000 0000 0000 0001
0000 0000 0000 0010

```

Note that there should be a newline at the end of each input file. Also, when `PC` is set to $65535$, the program finishes. In the above example, copy last two lines from instructions to set `PC` to $65535$.
