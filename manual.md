# SUBLEQ eForth Reference Manual

> A high-performance Forth implementation for SUBLEQ machines
> Self-hosting development environment on minimal hardware

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [System Architecture](#system-architecture)
- [Programming Guide](#programming-guide)
- [Language Reference](#language-reference)
- [Examples](#examples)
- [Development and Debugging](#development-and-debugging)
- [Performance Considerations](#performance-considerations)
- [References](#references)

## Overview

This reference manual serves as a complete guide to SUBLEQ eForth, a remarkable achievement in computational minimalism: a complete, self-hosting development environment implemented on a SUBLEQ (SUBtract and branch if Less than or EQual to zero) architecture. SUBLEQ is a One Instruction Set Computer (OISC) that achieves Turing completeness with a single instruction type, while eForth is a minimal, portable Forth kernel designed to simplify implementation across diverse hardware platforms. Together, they demonstrate that sophisticated software systems can emerge from the most constrained hardware foundations.

### What This Manual Covers

This manual serves multiple audiences and purposes:

For Educators and Students: Complete explanations of how SUBLEQ eForth demonstrates fundamental computing concepts, from basic stack operations through self-hosting compilation. The system provides an ideal laboratory for understanding language implementation, virtual machines, and bootstrapping processes.

For Practitioners: Detailed programming guide covering Forth syntax, control structures, memory management, and system programming techniques specific to the SUBLEQ environment. Includes practical examples, debugging strategies, and performance optimization approaches.

For System Implementers: Comprehensive technical reference documenting the four-layer architecture, virtual machine design, memory organization, and threading mechanisms. Essential for understanding, modifying, or porting the system.

For Researchers: Analysis of the unique constraints and solutions in single-instruction computing, including formal verification opportunities, security implications, and architectural trade-offs between hardware and software complexity.

### System Scope and Capabilities

The system implements a complete eForth environment using only the SUBLEQ instruction, providing the same programming interface that standard eForth implementations achieve with approximately 30 machine primitives on conventional CPUs. This extraordinary feat is accomplished through a carefully designed virtual machine layer that translates eForth operations into sequences of SUBLEQ instructions.

Key capabilities include interactive development, self-hosting compilation, configurable feature sets ranging from 6 KiB minimal systems to 12 KiB full development environments, and complete compatibility with standard eForth programming practices.

### Architectural Innovation

The implementation achieves complete self-hosting through four distinct architectural layers operating within 8 KiB of memory: a SUBLEQ macro-assembler that provides essential development tools and requires only a host Forth system for initial operation; a 16-bit SUBLEQ virtual machine that implements approximately 40 operation codes while operating exclusively on raw SUBLEQ instructions; a meta-compiler that generates ROM images and manages system configuration parameters; and an interactive eForth layer that provides a full development environment comparable to traditional Forth systems. Each layer provides services to the layer above while maintaining functional independence, enabling complete bootstrapping without external dependencies once the system is established.

The system supports compile-time feature selection through configuration constants, enabling builds tailored to specific requirements. Available features include multitasking support, extended arithmetic operations, block-based file editing capabilities, floating-point arithmetic, and additional I/O operations. Build configurations range from minimal 6 KiB cores suitable for severely resource-constrained environments to full 12 KiB development systems with comprehensive functionality. The implementation operates with 16-bit cells as the fundamental data unit, supports a maximum 64 KiB address space, and provides memory-mapped character-based terminal I/O.

What distinguishes this implementation is the demonstration of the minimal boundary between hardware and software capabilities: while classic eForth needs approximately 30 machine primitives on a normal CPU, the SUBLEQ port delivers the same API on top of a single instruction by inserting one ultra-lean virtual machine tier. This architectural approach demonstrates how complex software systems can be constructed incrementally from minimal computational substrates, requiring initial compilation using a host Forth system for bootstrapping but becoming completely self-sufficient once established. This self-hosting capability makes SUBLEQ eForth both a practical development environment and an educational platform for understanding the fundamental principles of computation, compilation, and system design.

### Evolutionary Context and Project Heritage

SUBLEQ eForth represents the culmination of three progressive stages in computational minimalism: traditional Forth with 100+ primitives enabling direct human-machine communication, eForth with approximately 30 primitives reducing machine dependencies, and SUBLEQ eForth achieving ultimate hardware minimalism by implementing approximately 40 virtual operations on top of a single instruction. This implementation extends foundational work by Richard James Howe, transforming a proof-of-concept into a practical development environment while preserving educational transparency.

## Why Learn Forth?

Forth exists in a hierarchy of minimalism that reveals fundamental computing principles. ANS Forth provides hundreds of standardized words, eForth reduces this to ~30 primitives, and SUBLEQ eForth implements these primitives as virtual operations on a single hardware instruction. This progression demonstrates how software complexity can compensate for hardware simplicity.

Unlike languages that hide their implementation behind complex compilers, Forth exposes every mechanism directly to the programmer. You can understand and modify the entire system, from the interpreter loop to memory management. Working with SUBLEQ eForth develops programming intuition that transfers across all platforms - successfully implementing functionality on a single-instruction computer makes any conventional processor feel approachable.

SUBLEQ eForth serves as an ideal laboratory for understanding computer science fundamentals: language implementation, virtual machines, self-hosting compilers, and the relationship between hardware and software. Every component fits on a single screen, enabling complete comprehension of a working system. The trivial ISA makes formal verification and side-channel experiments feasible in ways impractical on larger cores.

## Implementation Context

This is a 16-bit Forth implementation where cells are 16 bits. Addresses refer to cells, not bytes - address `100` refers to the 100th cell (byte offset 200). Since SUBLEQ lacks CALL/RET instructions, the interpreter uses dynamic code modification for threading and software-simulated return stack management.

## Quick Start

### Interactive Web-Based System

The most accessible way to begin exploring SUBLEQ eForth is through the interactive web-based version available at [GitHub](https://howerj.github.io/subleq.htm). This complete implementation provides:

- Full SUBLEQ eForth system with all features enabled
- Zero installation requirements - executes directly in web browsers
- Ideal environment for learning and experimentation
- Complete compatibility with all examples in this manual

Simply navigate to the provided URL and begin entering Forth commands immediately. This web-based system provides an excellent platform for following along with examples throughout this manual and experimenting with concepts as they are introduced.

### Local Installation

Requirements: C compiler, Gforth, GNU Make

```shell
# macOS
brew install gforth

# Ubuntu/Debian
sudo apt-get install gforth build-essential

# Fedora/CentOS
sudo dnf install gforth gcc make
```

### Building and Running

The complete build and execution process requires only a few commands:

```shell
# Build and run the system
make run
```

#### Build Process Details

This builds the SUBLEQ emulator and launches the eForth interpreter. The system demonstrates bootstrapping: it can rebuild itself once established.

Additional Commands:
- `make check` - Run test suite
- `make bench` - Performance benchmarks
- `make bootstrap` - Verify self-hosting
- `make clean` - Remove built files

### Your First Interactive Session

Begin exploring SUBLEQ eForth with this guided introduction to fundamental concepts and operations:

```forth
\ Welcome to SUBLEQ eForth!
\ Comments start with backslash and extend to end of line

\ List all available commands
words

\ To access additional system words (including .s for stack display):
system +order
.s                 \ Now .s should work to show stack contents

\ Basic arithmetic uses postfix notation
\ Instead of "21 + 21", write "21 21 +"
21 21 + .          \ Result: 42
cr                 \ Print newline

\ Stack manipulation example
5 3 2              \ Stack: [5, 3, 2] (2 is on top)
.s                 \ Show current stack
*                  \ Multiply top two: 3 * 2 = 6. Stack: [5, 6]
+                  \ Add: 5 + 6 = 11. Stack: [11]
.                  \ Print 11

\ Define a new word
: hello            \ Start definition
    ." Hello, World!" cr
;                  \ End definition

\ Execute the new word
hello

\ Define a word that uses the stack
: double ( n -- 2n )
    dup +          \ Duplicate and add
;

\ Test the double function
7 double .         \ Result: 14

\ System introspection
here .             \ Show dictionary pointer

\ Exit the system
bye
```

This introductory session demonstrates the core concepts that make Forth unique: postfix notation for arithmetic, stack-based data manipulation, word definition and execution, and interactive system exploration.


## System Architecture

### Understanding the Forth Data Model

This implementation employs 16-bit cells as the fundamental data unit, establishing consistent conventions throughout the system. The system uses consistent data organization: cell size is 16 bits (2 bytes) as the basic unit of data storage and manipulation, address size is 16 bits (64KB total address space) sufficient for substantial programs, character size is 8 bits stored within cells using bit manipulation for standard ASCII support, and each stack position contains one 16-bit cell for uniform data handling.

Memory addresses always point to cell boundaries, ensuring proper alignment for efficient access. The addressing convention represents a fundamental concept: All Forth addresses refer to cells, not bytes. Address 10 refers to the 10th cell (located at byte offset 20), not byte 10. Only character-specific operations work with byte offsets within cells. This convention is fundamental to understanding memory operations and must be consistently applied throughout programming.

### The SUBLEQ Foundation

SUBLEQ achieves universal computation through a remarkably simple instruction format that demonstrates the sufficiency of minimal computational primitives. The instruction format follows a consistent pattern:

```assembly
subleq a, b, c   ; Mem[b] = Mem[b] - Mem[a]
                 ; if (Mem[b] ≤ 0) goto c
```

This performs three sequential steps: subtract `memory[a]` from `memory[b]`, store the result back to `memory[b]`, and if the result is ≤ 0, branch to address `c`. When the branch target is the next instruction, the third argument can be omitted for brevity (`subleq a, b`).

Despite having only subtraction and conditional branching, SUBLEQ can implement any computational operation through careful composition. Addition is performed by:

```assembly
; Add Mem[a] to Mem[b], result in Mem[b]
subleq a, Z     ; Z = 0 - Mem[a] = -Mem[a]
subleq Z, b     ; Mem[b] = Mem[b] - (-Mem[a]) = Mem[b] + Mem[a]
subleq Z, Z     ; Clear temporary: Z = Z - Z = 0
```

Copy operations use:

```assembly
; Copy Mem[a] to Mem[b]
subleq b, b     ; Clear destination: Mem[b] = 0
subleq a, Z     ; Z = 0 - Mem[a] = -Mem[a]
subleq Z, b     ; Mem[b] = 0 - (-Mem[a]) = Mem[a]
subleq Z, Z     ; Clear temporary: Z = 0
```

A simple program demonstrating conditional execution:

```assembly
; If Mem[x] > 5, goto positive_branch
; First: compute (Mem[x] - 6)
subleq six, x, positive_branch   ; if x - 6 ≤ 0, goto positive_branch
; x ≤ 5: execute this path
subleq msg1, output
subleq halt, halt, halt

positive_branch:
; x > 5: execute this path
subleq msg2, output
subleq halt, halt, halt

; Data
six:    6
msg1:   "small"
msg2:   "large"
output: -1      ; Output port
halt:   -1      ; Halt address
```

The instruction set extends beyond basic computation through I/O operations: `a = -1` reads one byte from input stream to address `b`, `b < 0` writes one byte from address `a` to output stream, and `c < 0` terminates program execution.

### Why SUBLEQ and Forth Are a Perfect Match

SUBLEQ and Forth represent complementary approaches to computational minimalism that create a uniquely powerful combination. SUBLEQ demonstrates that any computation can be performed with a single instruction type, while Forth demonstrates that sophisticated programming environments can be built from minimal language primitives. Together, they prove that neither complex hardware nor elaborate software infrastructure is necessary for complete computational capability.

Forth's stack-based execution model maps naturally onto SUBLEQ's memory-centric architecture. Forth operations manipulate data through stack operations that translate efficiently into SUBLEQ's subtract-and-branch primitives. Both systems prioritize explicitness over convenience, giving programmers direct control over computational resources without hidden mechanisms or automatic optimizations. This philosophical alignment enables the SUBLEQ virtual machine to provide Forth's essential operations without compromising either system's fundamental characteristics.

The combination reveals the minimal boundary between hardware and software. Traditional processors provide dozens of instruction types to support high-level languages, but SUBLEQ eForth demonstrates that a single instruction can support the same programming interface through careful software design. This insight has profound implications for understanding computer architecture, embedded system design, and the relationship between programming languages and their underlying platforms.

### Virtual Machine Architecture

The system employs a layered architecture where each level provides services to higher levels while maintaining clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Forth Applications                       │
│         Interactive REPL • Mathematical Programs            │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│              eForth Word Definitions (~170)                 │
│   Control Structures • I/O • Arithmetic • Memory Mgmt       │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│             SUBLEQ-Adapted Primitives (~40)                 │
│    Stack Operations • Memory Access • Logic • Control       │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│               SUBLEQ Hardware/Emulator                      │
│    Single Instruction: SUBLEQ a, b, c                       │
│    Memory: 64KB • I/O: Character terminal                   │
└─────────────────────────────────────────────────────────────┘
```

This layered approach enables sophisticated functionality while maintaining implementation simplicity. Each layer abstracts away the complexity of lower layers, allowing higher-level components to operate without concern for underlying implementation details. The isolation between layers also enables independent optimization and modification of each component, demonstrating how complex software systems can be constructed incrementally from minimal computational substrates.

### Memory Organization

The system employs a carefully designed memory layout that maximizes efficiency while accommodating the constraints of the SUBLEQ instruction set. Understanding this layout is crucial for system programming and debugging.

| Memory Region | Address Range | Purpose |
|---------------|---------------|---------|
| Zero Page | 0–1 | Optimization constants and special values |
| Boot Vector | 2 | System startup entry point |
| VM Core | Startup Region | Inner interpreter implementation |
| Primitives | ≤ primitive | Extended operation set implementation |
| Dictionary | > primitive | Standard eForth layout for user definitions |
| Task Block | End-512 cells | Thread-local storage for multitasking |

#### Detailed Memory Layout

The memory organization follows a predictable pattern that enables efficient address calculation and system operation:

```
Higher Memory ↑
┌─────────────────────────────────────────────────────────┐
│  Task Block (Thread-local data) | End-512 to End        │
│                    512 cells = 1 KiB                    │
├─────────────────────────────────────────────────────────┤
│  Dictionary (User definitions) | > primitive            │
│                   Grows upward                          │
├─────────────────────────────────────────────────────────┤
│  Primitives (Core operations) | 3 to primitive          │
│                   Fixed size                            │
├─────────────────────────────────────────────────────────┤
│  VM Core (Inner interpreter) | 3 to startup region      │
│                 Self-modifying                          │
├─────────────────────────────────────────────────────────┤
│  Boot Vector | Address 2                                │
├─────────────────────────────────────────────────────────┤
│  Permanent Zero | Address 1                             │
├─────────────────────────────────────────────────────────┤
│  Permanent Zero | Address 0                             │
└─────────────────────────────────────────────────────────┘
Lower Memory ↓
```

This layout provides predictable memory regions for different system components while enabling efficient address calculation and memory management. The permanent zeros at addresses 0 and 1 provide optimization constants that simplify certain operations, while the boot vector at address 2 provides the system entry point.

### Two-Stack Model Implementation

SUBLEQ eForth maintains Forth's fundamental two-stack architecture, which provides elegant solutions to control flow and data management challenges. Understanding this architecture is essential for effective Forth programming.

Forth was created for minuscule machines and real-time control, leading to a design where everything operates on two software stacks. This approach minimizes hardware requirements while providing complete computational capability. The entire execution model including parameter passing, expression evaluation, control flow, and subroutine linkage operates through this dual-stack architecture.

The two stacks serve distinct purposes and have different visibility and hardware requirements:

| Stack | Lifetime | Typical Contents | Hardware Needed | Key Operations |
|-------|----------|------------------|-----------------|----------------|
| Data Stack | Visible to programmer | Operands, intermediate results, loop indices, I/O flags | None - just two RAM cells: pointer and "top-of-stack" cache | `DUP` `DROP` `SWAP` `OVER` `ROT` |
| Return Stack | Hidden except for explicit `>R R>` words | Return addresses, local variables, temporary scratch | None - pointer held in RAM like any other variable | `>R` `R@` `R>` `RDROP` |

The stack separation solves a fundamental computational problem: if only one stack handled both data and return addresses, predicting stack depth upon function return would become impossible in the general case. The return stack provides clean, predictable control flow that operates independently of data stack usage patterns.

Return stack discipline requires careful attention: The return stack must maintain perfect balance across every word termination. Every `>R` operation must have a corresponding `R>` or `RDROP` operation before the word ends. Unbalanced return stack usage will cause system crashes because the inner interpreter relies on return stack integrity for proper control flow.

This discipline extends to temporary storage usage: while the return stack can store temporary values during computation, these values must be removed before the word exits. The return stack serves as a last-in-first-out temporary storage area, but its primary purpose is control flow management.

### Threading and Control Flow

The implementation employs sophisticated techniques to achieve efficient threading despite SUBLEQ's single-instruction constraint. These techniques demonstrate how high-level programming constructs can be implemented efficiently on minimal hardware.

Dynamic code modification enables proper word-to-word transitions by adjusting jump addresses at runtime. Address-range dispatch handles primitive operation invocation by testing addresses against known ranges and branching to appropriate handlers. Runtime patching of NEXT instructions facilitates high-level word threading by modifying the thread advancement code as needed.

These techniques collectively provide performance comparable to conventional Forth implementations while operating within severe architectural constraints. The sophistication of these approaches demonstrates that hardware limitations need not prevent the implementation of efficient high-level programming environments.

## Programming Guide

### Understanding Forth's Computational Philosophy

Forth represents a distinctive approach to programming that emphasizes directness, simplicity, and programmer autonomy. This philosophy manifests in several core tenets that distinguish it from mainstream programming languages:

Explicit Control Over Implicit Safety: Forth prioritizes explicit programmer control over automatic safety mechanisms. The language assumes that programmers understand their requirements and should have direct access to system capabilities without intervening abstraction layers.

Directness Over Abstraction: The language favors directness of expression over multiple abstraction layers. Operations map closely to underlying machine capabilities, enabling programmers to understand and predict system behavior precisely.

Uniform Word Treatment: Everything in Forth is a word (function), including control structures, comments, and system operations. This uniformity eliminates special syntax and enables unlimited language extension.

Efficiency Over Safety: Computational efficiency takes precedence over error prevention, enabling maximum performance while requiring careful programming practices.

### Essential Forth Characteristics

Several fundamental characteristics distinguish Forth from mainstream programming languages and must be understood for effective programming:

Forth is space-delimited and processes programs one line at a time using Reverse Polish Notation for mathematical operations. The system operates with two stacks - a data stack for computation and a return stack for control flow - both directly accessible to programmers. The interpreter alternates between compile mode (building new definitions) and interactive mode (immediate execution).

Forth is a procedural language where functional programming is possible but not the primary strength. The language is untyped and not memory safe, placing full responsibility on programmers for correctness. Forth operates in a global hyper-static environment where new redefinitions do not replace old ones; functions that used previous definitions continue using those definitions even after new ones are entered.

Each Forth implementation is unique despite various standards (FORTH-79, ANS Forth), and SUBLEQ eForth specifically is case-sensitive unlike many traditional implementations. The system features an incremental, interactive compiler that enables immediate testing and modification of code during development.

### Forth Terminology and Nomenclature

Forth employs distinctive terminology that differs from mainstream computing culture, reflecting its origins in the microcomputer era:

- double: Refers to double-width integers, not floating-point numbers. On a 16-bit system, a double would be a 32-bit integer. Early Forth implementations lacked floating-point capabilities due to the constraints of 8-bit target systems.
- word: In Forth, this refers to a function or command, not the machine's natural data width. This terminology stems from Forth's dictionary-based organization where each function is considered a "word" in the language.
- cell: Used to denote the natural machine width since "word" already has a different meaning. On this 16-bit implementation, a cell is 16 bits. This maintains consistency with Forth's addressing model.
- dictionary: The collection of all currently compiled functions in the Forth system, implemented as a linked list where each entry contains a word definition. The name reflects the linguistic metaphor of Forth's vocabulary.
- meta-compiler: A cross-compiler written in Forth used to create new Forth systems, differing from the standard compiler terminology. This enables the self-hosting capabilities central to Forth's design.
- compiler security: Optional implementation features that perform basic program validation, such as ensuring matching control structure pairs. These may be omitted in minimal implementations for size and performance reasons.

### Forth's Execution Model

Forth employs a remarkably simple but powerful execution model that processes input line by line with immediate feedback. Understanding this model is fundamental to effective Forth programming and explains many aspects of the language's behavior.

The execution sequence follows a consistent pattern:
1. Parse each input line into space-delimited tokens (words) from left to right
2. For each word encountered, execute it immediately with semantics that depend on the compiler mode and word type (whether it is immediate or not)
3. If a word is not found in the dictionary, attempt to parse it as a numeric literal in the current input radix
4. If numeric parsing succeeds, push the number onto the data stack (in command mode) or compile it (in compile mode)
5. If an error occurs, discard the remainder of the line and perform error handling
6. If the line executes successfully and the interpreter is in command mode, print "ok"

Understanding that Forth words are ordinary functions rather than syntax is fundamental. The word `."` is a function, not syntax like strings in C. In Forth, strings, comments, if statements, loops, and function definitions are all ordinary words. This uniformity enables the language's remarkable extensibility but requires a different mental model than most programming languages.

#### Basic Arithmetic and Reverse Polish Notation

Forth uses Reverse Polish Notation (RPN) for arithmetic operations, where operands precede operators. This notation differs from the infix notation used in most programming languages but provides several advantages in a stack-based system.

To add two numbers and print the result:

```forth
2 2 + .            \ Equivalent to "2+2", prints: 4
```

The parsing rules apply consistently throughout the system. Entering `2+` will likely cause an error because `2+` is treated as a single undefined word, not as `2 +`. This consistency eliminates special cases and makes the parser implementation simpler.

#### Stack-Based Execution

When the line `2 3 * 2 + .` is entered, the following sequence occurs:

1. `2` is pushed to the data stack
2. `3` is pushed to the data stack
3. `*` pops `3` and `2`, multiplies them (3×2=6), pushes result
4. `2` is pushed to the data stack
5. `+` pops `2` and `6`, adds them (6+2=8), pushes result
6. `.` pops `8` and prints it in the current output base
7. `ok` is printed and the next line is awaited

Stack operations follow Last In, First Out (LIFO) ordering. For non-commutative operations like subtraction, the second-to-top element is the first operand: `5 2 -` computes 5-2, not 2-5. This convention is consistent throughout Forth and must be remembered when performing operations where order matters.

### Your First Forth Program: Hello World

It is customary for the first program in any programming tutorial to be a "Hello World" program. This tradition traces back to Brian Kernighan and Dennis Ritchie's influential 1978 book "The C Programming Language," though Kernighan had used similar examples in earlier Bell Labs tutorials. The "Hello World" program serves several essential pedagogical purposes: it demonstrates the basic syntax and structure of the language, provides immediate visual feedback that the development environment is working correctly, requires minimal understanding of complex concepts, and establishes a common reference point across different programming languages.

In SUBLEQ eForth, this simple program demonstrates several fundamental concepts:

```forth
: hello cr ." Hello, World" ;
```

Once entered, call the function by typing `hello` and pressing return. Note that Forth is space-delimited and every space matters. The following constructs will not work or will produce different output:

```forth
: hello cr ."Hello, World" ;        \ Missing space before string
:hello cr ."Hello, World";          \ Missing spaces around definition
:hello cr ." Hello, World";         \ Multiple spacing issues
: hello cr ."  Hello, World" ;      \ Extra spaces in string
```

#### Breaking Down the Hello World Definition

The working definition `": hello cr ." Hello, World" ;"` contains several important components:

`: (colon)`: A defining word that takes the next word from the input stream (`hello`) and creates a new header in the dictionary. The word is not yet linked into the dictionary and cannot be called until the definition is complete.

`cr`: Prints a newline character. This ensures the greeting appears on its own line.

`." (dot-quote)`: Parses the input stream until the closing `"`, compiling the string into the dictionary. When the function executes, this string will be printed. Important limitations: the `"` character cannot be escaped or included within strings, and the maximum string length in SUBLEQ eForth is 255 bytes.

`; (semicolon)`: Ends the function definition. This is an immediate word that switches the interpreter back to command mode and links the word definition into the dictionary, making it callable. Until this happens, the word will not appear in the dictionary, and any previous definition with the same name remains active.

This simple example demonstrates Forth's immediate compilation model, space-sensitive parsing, string handling capabilities, and the dictionary linking process that makes new words available for use.

### Understanding Forth Numbers and Numeric Systems

Forth's numeric system includes several important concepts that differ from conventional programming languages. Understanding these differences is essential for effective numeric programming in Forth.

#### Number Input Format Recognition

Forth recognizes various numeric formats during input parsing, enabling flexible numeric input while maintaining consistent behavior:

```forth
\ Different number formats
42                 \ Decimal number
$2A                \ Hexadecimal (SUBLEQ eForth requires uppercase)
-15                \ Negative number

\ Double precision numbers
42.                \ Double cell number (occupies 2 stack slots)
123.45             \ Still a double cell integer, not floating point!

dpl @ .            \ Check decimal point location after number entry
```

The decimal point in Forth indicates double-precision integers, not floating-point numbers. This distinction is crucial for understanding numeric operations. The decimal point sets a system variable (`dpl`) that tracks the decimal point location for certain operations, but the number itself remains an integer value.

#### Base Conversion Operations

Forth supports multiple numeric bases through runtime base conversion, enabling flexible numeric representation and conversion:

```forth
\ Working with different bases
decimal            \ Set base to 10
16 .               \ Prints: 16
16
hex                \ Set base to 16
.                  \ Prints: 10 (16 in hex is 10)
decimal            \ Back to decimal
base @ .           \ Show current base
```

The current base affects both numeric input parsing and output formatting, providing flexible numeric representation capabilities. This runtime base conversion enables programs to work with different numeric representations as needed without requiring separate conversion functions.

### Defining Words and Code Structure

Word definition represents the primary mechanism for extending Forth's vocabulary and building complex programs from simple components. Understanding the definition process and proper code structure is essential for effective Forth programming.

#### Simple Word Definitions

Basic word definitions follow a consistent pattern that enables clear expression of computational intent:

```forth
\ Basic word definition
: square ( n -- n^2 )
    dup *
;

5 square .         \ Result: 25

\ Words can call other words
: quadruple ( n -- 4n )
    square square
;

3 quadruple .      \ Result: 81
```

Word definitions create new vocabulary entries that behave identically to built-in system words once defined. The colon (`:`) word begins the definition and switches to compile mode, while the semicolon (`;`) word ends the definition and returns to interpretation mode.

The stack comment notation (`( n -- n^2 )`) documents the word's behavior but is not enforced by the system. These comments serve as essential documentation for understanding word interfaces and should be included in all but the most trivial definitions.

#### Control Structure Implementation

Forth implements control structures as ordinary words rather than special syntax, enabling uniform treatment throughout the system. This uniformity allows control structures to be extended and modified like any other part of the language:

```forth
\ Conditional execution
: abs ( n -- |n| )
    dup 0< if negate then
;

\ Loops using begin/until
: countdown ( n -- )
    begin
        dup .
        1 - dup 0=
    until
    drop
;

\ Loops using for/next (eForth style)
: count-down ( n -- )
    0 swap for
        r@ .
    next
;

\ Note: for/next executes n+1 times
5 count-down         \ Prints: 5 4 3 2 1 0
```

Understanding that control structures are ordinary words helps explain Forth's uniform syntax and enables the creation of custom control structures when needed. This uniformity also means that control structures can be redefined or extended like any other words in the system.

#### Complex Control Structure Patterns

Nested and compound control structures enable sophisticated program logic while maintaining Forth's characteristic simplicity:

```forth
\ Nested conditionals
: describe-number ( n -- )
    dup 0= if
        ." Zero"
    else
        dup 0< if
            ." Negative "
            abs .
        else
            ." Positive " .
        then
    then
;

\ Using begin/while/repeat
: print-squares ( n -- )
    begin
        dup 0>
    while
        dup dup * . space
        1-
    repeat
    drop
;

5 print-squares    \ Prints: 25 16 9 4 1
```

These examples demonstrate how control structures can be combined to create sophisticated logic while maintaining readability and the consistent flow of Forth programs.

### Control Structures in Detail

Forth implements control structures as ordinary words rather than special syntax, enabling uniform treatment and custom control structure creation. All control structures must be used within word definitions; using them outside definitions typically causes errors because they manipulate compilation addresses.

#### Conditional Execution

Basic conditional structures use stack-based boolean values for control flow. Understanding Forth's boolean representation (true is -1, false is 0) is essential for effective conditional programming.

`if...then` executes code block if top stack item is non-zero (true):
```forth
: test-positive ( n -- )
    dup 0> if ." positive" then
;
```

`if...else...then` provides alternative execution paths:
```forth
: sign-description ( n -- )
    dup 0< if
        ." negative"
    else
        dup 0> if
            ." positive"
        else
            ." zero"
        then
    then
    drop
;
```

The `then` keyword, borrowed from conditional logic terminology, marks the end of the conditional block rather than introducing a consequence as in some languages. This usage follows mathematical logic where "if P then Q" means "P implies Q."

#### Loop Constructs

Forth provides several loop constructs with different characteristics and use cases. Understanding when to use each type enables effective algorithm implementation.

Infinite Loops:
`begin...again` creates infinite loops requiring manual termination:
```forth
: infinite-loop
    begin
        key emit  \ Echo input characters
    again
;
```

Conditional Loops:
`begin...until` continues until condition becomes true (non-zero):
```forth
: countdown ( n -- )
    begin
        dup . 1- dup 0=
    until
    drop
;
```

#### For-Next Loops (eForth Style)

The `for...next` construct provides definite iteration with accessible loop counters:

Basic for-next loop (executes n+1 times):
```forth
: print-numbers ( n -- )
    for
        r@ .    \ Print current loop counter
    next
;
```

Advanced for-aft-then-next construct:
```forth
: complex-loop ( n -- )
    for
        ." start "           \ Executed once
    aft
        r@ . ." middle "     \ Executed n times
    then
        ." end "             \ Executed n+1 times
    next
;
```

The loop counter is accessible via `r@` (return stack fetch). Early loop termination requires `rdrop exit` to properly clean up the return stack.

#### Do-Loop Constructs (Optional in SUBLEQ eForth)

Standard Forth includes `do...loop` constructs for range-based iteration:

Basic do-loop with inclusive bounds:
```forth
: range-print ( end start -- )
    do
        i .     \ Print loop index
    loop
;
```

Variable increment loops:
```forth
: skip-count ( end start increment -- )
    do
        i .
    +loop       \ Increment by value on stack
;
```

Conditional do-loops avoid execution when start equals end:
```forth
: safe-range ( end start -- )
    ?do
        i .
    loop
;
```

Loop indices are accessed with `i` (current), `j` (enclosing), and `k` (doubly enclosing). Early termination uses `leave` rather than `exit`.

#### Case Statements (Optional)

Multi-way branching uses case statements for cleaner code than nested conditionals:

```forth
: process-grade ( grade -- )
    case
        char A of ." Excellent" endof
        char B of ." Good" endof
        char C of ." Average" endof
        char D of ." Poor" endof
        ." Unknown grade"    \ Default case
    endcase
;
```

Case statements consume the test value and execute only the matching branch.

### Comments and Documentation

Forth treats comments as ordinary words rather than special syntax, providing flexibility but requiring understanding of their implementation characteristics. Effective documentation practices improve code maintainability and enable collaboration.

#### Standard Comment Words

Two primary comment words serve different purposes and have different implementation characteristics:

Line Comments (`\`):
- Discards everything from the comment word to the end of the current line
- Works reliably across all contexts and implementations
- Safe for multi-line usage

```forth
\ This is a line comment
42 . \ This prints 42
```

Parenthetical Comments (`(`):
- Discards everything until the matching closing parenthesis `)`
- Requires space after opening parenthesis (it's a parsing word)
- May have implementation-dependent limitations

```forth
( This is a parenthetical comment )
42 ( this number ) . ( prints it )
```

#### Comment Implementation Considerations

The `(` word's behavior depends on its implementation, which can cause portability issues across different Forth systems:

Safe Usage:
```forth
( This works reliably )
( This also works )
```

Potentially Problematic Usage:
```forth
(This might not work)     \ No space after opening
(
    This might fail       \ Multi-line comments
    across implementations
)
```

Implementation differences arise from two factors:
1. Word-based parsing: Some implementations look for `)` as a separate word, not as part of another word like `work)`
2. Line reload behavior: Some implementations don't reload input lines within comments, treating end-of-line as an implicit `)`

#### Documentation Conventions

Effective Forth documentation combines comments with stack effect notation to provide both interface and implementation information:

```forth
\ Calculate compound interest
\ Formula: principal * (1 + rate)^years
: compound-interest ( principal rate years -- amount )
    >r              \ Save years on return stack
    100 +           \ Convert percentage to multiplier
    100 /           \ Normalize rate
    r>              \ Retrieve years
    power           \ Calculate (1+rate)^years
    *               \ Multiply by principal
;
```

Stack effect comments serve as interface documentation, while explanatory comments provide algorithmic insight and implementation details.

### Conditional Compilation

Forth provides conditional compilation capabilities through immediate words that enable portable code development across different Forth implementations. This capability becomes particularly important when developing code that must work across multiple Forth systems or when implementing optional features.

#### Conditional Compilation Words

`[if]`, `[else]`, and `[then]` function as compile-time conditionals that determine which code sections are included in the final program:

```forth
0 [if]
    ." This won't compile"
[else]
    ." This will compile"
[then]
```

These words evaluate conditions during compilation rather than execution, enabling implementation-specific code paths and optional feature inclusion.

#### Feature Detection

The `defined` word tests for word availability, enabling conditional compilation based on vocabulary:

```forth
defined ?dup 0= [if]
    : ?dup dup if dup then ;
[then]
```

This pattern provides fallback implementations for words that may not exist in all Forth systems, enabling portable code development.

Conditional compilation enables portable code that adapts to different Forth implementations while maintaining functionality across varying feature sets. This capability is particularly valuable when developing libraries or applications that must work across multiple Forth systems.

### Memory Operations and Data Structures

Forth provides direct access to memory through a collection of memory manipulation words that enable construction of arbitrary data structures. Understanding these operations is essential for systems programming and advanced applications.

#### Basic Memory Access

The fundamental memory operations provide direct interface to system memory:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `@` | `( addr -- value )` | Fetch cell |
| `!` | `( value addr -- )` | Store cell |
| `c@` | `( addr -- char )` | Fetch character |
| `c!` | `( char addr -- )` | Store character |
| `+!` | `( n addr -- )` | Add to memory |

Memory access words operate on the natural machine word size (16 bits in SUBLEQ eForth). The `@` operator fetches values of the system's natural size, while character access words (`c@` and `c!`) handle byte-sized quantities within the cell-based addressing model.

#### Variables and Constants

Variables and constants provide named storage locations and fixed values using defining words:

```forth
\ Creating variables (uninitialized)
variable x
variable y

\ Creating constants
42 constant answer
256 constant max-size

\ Using variables
0 x !        \ Store 0 in variable x
x @ 1 + x !  \ Increment x
x @ .        \ Print current value

\ Constants return their value
answer .     \ Prints: 42
```

Variables return their address when executed, enabling both reading and writing operations. Constants return their value directly, providing efficient access to fixed data without requiring memory fetch operations. It's important to note that variables are not guaranteed to be initialized to any particular value.

#### Dictionary Space Management

The dictionary grows when new words are defined or memory is allocated manually. Several words help manage dictionary space:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `here` | `( -- addr )` | Dictionary pointer |
| `allot` | `( n -- )` | Allocate n bytes in dictionary |
| `,` | `( n -- )` | Compile cell to dictionary |
| `c,` | `( char -- )` | Compile character to dictionary |
| `align` | `( -- )` | Align dictionary pointer |

You can allocate and initialize memory in the dictionary:

```forth
variable string-ptr  ( used to store a pointer )
here string-ptr !    ( store current position )
here .               ( display current position )
5 c,                 ( write string length )
char H c,            ( write 'H' )
char e c,            ( write 'e' )
char l c,            ( write 'l' )
char l c,            ( write 'l' )
char o c,            ( write 'o' )
align                ( align dictionary pointer )
string-ptr @ count type cr  ( display "Hello" )
```

The `cells` word converts cell count to byte count for proper address arithmetic:

```forth
: array ( n "name" -- )
    create cells allot
;

: [] ( index array -- addr )
    swap cells +
;

\ Example usage
10 array my-data         \ Create array with 10 cells
42 0 my-data [] !        \ Store 42 at index 0
17 5 my-data [] !        \ Store 17 at index 5
0 my-data [] @ .         \ Fetch and print value at index 0: 42
5 my-data [] @ .         \ Fetch and print value at index 5: 17

\ Multiple arrays
5 array temperatures
3 array scores
25 2 temperatures [] !   \ Store temperature reading
100 1 scores [] !        \ Store test score
```

Understanding this distinction is important for efficient programming: constants compile their values directly into code, while variables require address calculation and memory access. This difference affects both performance and memory usage patterns.

### Advanced Programming Techniques

#### Execution Tokens and Indirect Execution

Execution tokens enable runtime selection of operations and implementation of hooks and callbacks, providing powerful abstraction mechanisms:

```forth
\ Getting execution tokens
variable operation
' + operation !          \ Store execution token for +
2 3 operation @ execute . \ Prints: 5

\ Creating hooks for customizable behavior
variable <emit-hook>
' emit <emit-hook> !

: my-emit ( char -- )
    <emit-hook> @ execute
;

\ Custom emit that converts to uppercase
: upper-emit ( char -- )
    dup 97 123 within if 32 - then emit
;

' upper-emit <emit-hook> !
char h my-emit           \ Prints: H
```

Execution tokens provide powerful abstraction mechanisms that enable flexible program architecture and runtime behavior modification. This capability enables implementation of callbacks, hooks, and other advanced programming patterns.

#### Recursive Definitions

Forth supports recursion through the `recurse` word, which provides safe self-reference within word definitions. You cannot call a function within its own definition because the definition has not yet been linked into the dictionary. Attempting direct self-reference will either cause an error if the word is undefined, or call a previous definition with the same name if one exists:

```forth
: x x ;                     \ Fails if `x` is not defined

: y cr ." executing first y" ;
: y cr ." new y" y ;         \ Calls the old definition of y
y                           \ May produce redefinition warning
```

Instead, use `recurse` for proper recursive calls:

```forth
\ Factorial using recursion
: factorial ( n -- n! )
    dup 1 <= if
        drop 1
    else
        dup 1 - recurse *
    then
;

5 factorial .           \ Prints: 120

\ Practical recursion example
: countdown ( n -- )
    ?dup if 1- dup . recurse then
;

10 countdown            \ Prints: 9 8 7 6 5 4 3 2 1 0
```

Recursive calls consume return stack space, which is finite and platform-dependent. Deep recursion may cause stack overflow. Some Forth implementations optimize tail recursion by replacing the final `recurse` with a jump to the beginning of the word, eliminating additional return stack usage. However, this optimization is not implemented in SUBLEQ eForth.

The `recurse` word ensures proper compilation of recursive calls and prevents issues that could arise from direct self-reference during compilation. This approach enables implementation of recursive algorithms while maintaining system stability.

## Language Reference

### Core Vocabulary Organization

The SUBLEQ eForth vocabulary is organized into logical groups that correspond to different aspects of system operation and programming support. Understanding this organization helps locate appropriate words for specific tasks and provides insight into system capabilities.

#### Stack Manipulation Operations

Stack manipulation forms the foundation of Forth programming, providing the basic tools for data organization and flow control. Understanding these operations is essential because Forth exposes the computational stack directly to programmers, unlike most languages that hide stack management.

The basic stack manipulation words have no side effects beyond rearranging data:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `dup` | `( n -- n n )` | Duplicate top item |
| `drop` | `( n -- )` | Remove top item |
| `swap` | `( a b -- b a )` | Exchange top two items |
| `over` | `( a b -- a b a )` | Copy second item to top |
| `rot` | `( a b c -- b c a )` | Rotate top three items |
| `nip` | `( a b -- b )` | Remove second item, leaving topmost |
| `-rot` | `( a b c -- c a b )` | Rotate top three items in opposite direction |
| `>r` | `( n -- ) (R: -- n )` | Move to return stack |
| `r@` | `( -- n ) (R: n -- n )` | Copy from return stack |
| `r>` | `( -- n ) (R: n -- )` | Move from return stack |

These operations can be practiced interactively. Note that comments in Forth are placed between `(` and `)`, and a space must come after `(` since it is a word:

```forth
.s         ( This should display no numbers )
1 drop .s  ( This should do the same )
1 dup .s   ( Displays `1 1` )
1 2 drop . ( Displays `1` )
1 2 .s     ( Note the order of `1 2` )
1 2 . .    ( Note the order: `2` prints first, then `1` )
1 2 nip .  ( Displays `2` )
1 2 over   ( Displays nothing yet... )
.s         ( Displays `1 2 1` )
drop drop drop ( Clear the stack )
```

To understand `rot` and `-rot` direction, experiment with:

```forth
1 2 3 rot .s    \ What order results?
1 2 3 -rot .s   \ What about this?
```

The return stack operations (`>r`, `r@`, `r>`) must be used with extreme caution and only within word definitions. Using them in command mode will produce the error `-14?` because they interfere with the system's control flow. Every `>r` must have a corresponding `r>` or `rdrop` before the word exits to maintain return stack balance.

#### Arithmetic Operations

Arithmetic operations implement the mathematical foundation required for computational tasks, with careful attention to operand order for non-commutative operations:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `+` | `( a b -- sum )` | Addition |
| `-` | `( a b -- diff )` | Subtraction (a - b) |
| `*` | `( a b -- product )` | Multiplication |
| `/` | `( a b -- quotient )` | Division (a / b) |
| `/mod` | `( a b -- remainder quotient )` | Division with remainder |
| `mod` | `( a b -- remainder )` | Modulo operation |
| `negate` | `( n -- -n )` | Two's complement |
| `abs` | `( n -- |n| )` | Absolute value |

Forth-specific arithmetic conveniences include:
- `2*`: Multiply by two (often implemented as left shift)
- `2/`: Divide by two (implementation varies for negative numbers)
- `1+`: Increment by one
- `1-`: Decrement by one

All arithmetic operations follow Forth's postfix convention, where operands precede the operator and results replace operands on the stack. For non-commutative operations like subtraction and division, the second-to-top element serves as the first operand: `5 2 -` computes 5-2, not 2-5.

You can experiment with these operations:

```forth
8 2 - .              \ Prints: 6
-1 -3 - .            \ Prints: 2
9 3 / .              \ Prints: 3
9 4 / .              \ Prints: 2 (integer division)
```

#### Logical and Bitwise Operations

Logical operations provide both boolean logic and bitwise manipulation capabilities. Forth's truth representation (true is -1, false is 0) enables bitwise operators to function as logical operators:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `and` | `( a b -- result )` | Bitwise AND |
| `or` | `( a b -- result )` | Bitwise OR |
| `xor` | `( a b -- result )` | Bitwise XOR |
| `invert` | `( a -- ~a )` | Bitwise NOT |
| `lshift` | `( n count -- result )` | Left shift |
| `rshift` | `( n count -- result )` | Right shift (zero-fill) |

Examples of bitwise operations:

```forth
-1 $A5A5 and .       \ Bitwise AND with hex number
$A5A5 $5A5A and .    \ AND two hex values
$A5A5 $5A5A or .     \ OR two hex values
$AA55 $5A5A xor .    \ XOR two hex values
1 4 lshift .         \ Left shift: 1 becomes 16
8192 2 rshift .      \ Right shift: 8192 becomes 2048
```

The universal nature of bitwise operations enables implementation of complex selection logic:

```forth
: mux dup >r and swap r> invert and or ;
```

This multiplex function selects between two values based on a condition, demonstrating how bitwise operations can implement logical selection.

#### Comparison Operations

Comparison operations generate boolean flags using Forth's truth representation:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `=` | `( a b -- flag )` | Equal |
| `<>` | `( a b -- flag )` | Not equal |
| `<` | `( a b -- flag )` | Signed less than |
| `>` | `( a b -- flag )` | Signed greater than |
| `<=` | `( a b -- flag )` | Signed less than or equal |
| `>=` | `( a b -- flag )` | Signed greater than or equal |
| `0=` | `( n -- flag )` | Equal to zero |
| `0<>` | `( n -- flag )` | Not equal to zero |
| `0<` | `( n -- flag )` | Less than zero |
| `0>` | `( n -- flag )` | Greater than zero |
| `u<` | `( u u -- flag )` | Unsigned less than |
| `u>` | `( u u -- flag )` | Unsigned greater than |
| `u<=` | `( u u -- flag )` | Unsigned less than or equal |
| `u>=` | `( u u -- flag )` | Unsigned greater than or equal |

The distinction between signed and unsigned comparisons is crucial because Forth lacks type information. On a 16-bit system, -1 and 65535 represent the same bit pattern; the programmer must select the appropriate comparison operator based on intended interpretation.

Experimenting with comparisons reveals the differences:

```forth
1 2 < .        \ Prints: -1 (true)
1 2 > .        \ Prints: 0 (false)
1 2 u< .       \ Prints: -1 (true, same as signed)
1 2 u> .       \ Prints: 0 (false, same as signed)
-1 1 < .       \ Prints: -1 (true, -1 is less than 1)
1 -1 < .       \ Prints: 0 (false, 1 is not less than -1)
-1 1 u< .      \ Prints: 0 (false! -1 as unsigned is 65535)
1 -1 u< .      \ Prints: -1 (true, 1 is less than 65535)
0 0= .         \ Prints: -1 (true)
1 0= .         \ Prints: 0 (false)
-1 0= .        \ Prints: 0 (false)
```

Notice the crucial differences between signed and unsigned operators when dealing with negative numbers treated as unsigned values.

This design enables elegant logical operations because bitwise operators can function as logical operators:

```forth
-1 -1 and .        \ Prints: -1 (true AND true = true)
-1 0 and .         \ Prints: 0 (true AND false = false)
-1 0 or .          \ Prints: -1 (true OR false = true)
```

This design enables the `mux` (multiplex) function to work effectively for conditional selection.

#### Memory Access Operations

Memory access operations provide direct interface to system memory for data storage and retrieval, enabling construction of arbitrary data structures:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `@` | `( addr -- value )` | Fetch cell |
| `!` | `( value addr -- )` | Store cell |
| `c@` | `( addr -- char )` | Fetch character |
| `c!` | `( char addr -- )` | Store character |
| `+!` | `( n addr -- )` | Add to memory |

Memory access words operate on the natural machine word size (at least 16 bits). The `@` operator fetches values of the system's natural size (16 bits on a 16-bit platform, 32 bits on a 32-bit platform). Address alignment requirements depend on the implementation; unaligned access may be prohibited or cause performance penalties.

Character access words (`c@` and `c!`) handle byte-sized quantities, providing access to individual bytes within the cell-based addressing model. Many 32-bit and 64-bit Forth implementations provide additional words for specific data sizes (16-bit, 32-bit, 64-bit) with prefixes like `w` (word) or `q` (quad).

#### Variables and Constants

Variables and constants provide named storage locations and fixed values using defining words:

Variables create uninitialized storage locations that return their address when executed:
```forth
variable x
variable y
```

Constants store fixed values that return their value when executed:
```forth
255 constant max-value
```

Address arithmetic enables construction of data structures:
```forth
: array ( n "name" -- )
    create cells allot
;

: [] ( index array -- addr )
    swap cells +
;
```

The `cells` word converts cell count to byte count for proper address arithmetic, accounting for the system's cell size.

#### Input/Output Operations

I/O operations provide interface to the external environment for user interaction and data exchange:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `.` | `( n -- )` | Print signed number |
| `u.` | `( u -- )` | Print unsigned number |
| `emit` | `( char -- )` | Output character |
| `key` | `( -- char )` | Input character |
| `cr` | `( -- )` | Output newline |
| `space` | `( -- )` | Output space |
| `."` | `( "text" -- )` | Print string literal |

These operations enable development of interactive programs and provide the basic tools required for user interface development.

### Extended Stack Operations

Extended stack operations provide additional tools for complex data manipulation beyond the basic stack words:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `pick` | `( n -- x )` | Copy nth stack item (0=top) |
| `?dup` | `( n -- n n \| 0 -- 0 )` | Duplicate if non-zero |
| `tuck` | `( a b -- b a b )` | Insert copy under second item |
| `2dup` | `( a b -- a b a b )` | Duplicate top two items |
| `2drop` | `( a b -- )` | Drop top two items |

These operations enable sophisticated stack manipulation patterns that would require multiple basic operations to achieve. However, excessive use of these words often indicates poorly structured code that would benefit from redesign.

#### Advanced Arithmetic Operations

Extended arithmetic operations provide enhanced computational capabilities for precision arithmetic and specialized calculations:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `um*` | `( u1 u2 -- ud )` | Unsigned multiply to double |
| `um/mod` | `( ud u -- rem quot )` | Unsigned double divide |
| `m/mod` | `( d n -- rem quot )` | Mixed precision divide |

These operations provide precise control over arithmetic precision and overflow handling, essential for numerical computation and systems programming.

#### Double-Precision Numbers

Double-precision numbers in Forth (called "doubles") occupy two cells on the data stack and provide extended range for integer calculations. They are not related to floating-point numbers but represent double-width integers.

Most double-precision words are not available in the base SUBLEQ eForth system. Only a minimal set is provided:

Double-precision numbers are entered by including a decimal point:

```forth
\ Entering double-precision numbers
2 dpl @ .s 2drop        \ Single-cell number, dpl = -1
2.1 dpl @ .s drop 2drop \ Double-cell number, dpl shows decimal position
20.1 dpl @ .s drop 2drop
```

The `dpl` variable indicates the decimal point location: -1 for single-cell numbers, 0 or greater for double-cell numbers showing the decimal point position.

Available conversion and arithmetic operations:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `s>d` | `( n -- d )` | Convert signed cell to double |
| `d+` | `( d1 d2 -- d3 )` | Double-cell addition |
| `dnegate` | `( d -- -d )` | Double-cell negation |
| `um+` | `( u u -- ud )` | Unsigned add producing double |

For unsigned conversions, simply push 0 after the number since the high portion of a double-cell integer is stored on the top stack location.

Stack manipulation words like `2drop`, `2dup` are used with double-precision numbers to manage the two-cell values efficiently. Most implementations define only a minimal subset of double-precision words due to space constraints.

#### Number Base Operations

Number base operations enable flexible numeric representation and conversion, supporting multiple numeric bases for different application requirements:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `decimal` | `( -- )` | Set base to 10 |
| `hex` | `( -- )` | Set base to 16 |
| `base` | `( -- addr )` | Variable holding current base |
| `>number` | `( ud addr1 u1 -- ud addr2 u2 )` | Convert string to number |
| `number?` | `( addr u -- d flag )` | Attempt number conversion |

These operations enable programs to work with different numeric representations and provide tools for numeric input parsing and output formatting.

#### Dictionary and System Operations

Dictionary operations provide access to system internals and enable system introspection and manipulation:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `words` | `( -- )` | List available words |
| `here` | `( -- addr )` | Dictionary pointer |
| `allot` | `( n -- )` | Allocate n bytes in dictionary |
| `,` | `( n -- )` | Compile cell to dictionary |
| `c,` | `( char -- )` | Compile character to dictionary |
| `'` | `( "name" -- xt )` | Get execution token |
| `execute` | `( xt -- )` | Execute word by token |
| `cell+` | `( addr -- addr' )` | Add one cell size to address |
| `cells` | `( n -- n' )` | Convert cell count to byte count |
| `align` | `( -- )` | Align dictionary pointer |
| `aligned` | `( addr -- aligned-addr )` | Round up to alignment |
| `bye` | `( -- )` | Exit system |

These operations enable system programming, memory management, and development of system tools and utilities.

#### Defining Words

Defining words provide the foundation for extending the system vocabulary and creating new language constructs:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `:` | `( "name" -- )` | Start word definition |
| `;` | `( -- )` | End word definition |
| `constant` | `( n "name" -- )` | Create named constant |
| `variable` | `( "name" -- )` | Create variable |
| `create` | `( "name" -- )` | Create word with data space |
| `does>` | `( -- )` | Define runtime behavior |
| `immediate` | `( -- )` | Mark last word as immediate |
| `postpone` | `( "name" -- )` | Compile word regardless of immediacy |
| `[` | `( -- )` | Switch to interpretation mode |
| `]` | `( -- )` | Switch to compilation mode |
| `literal` | `( n -- )` | Compile number into definition |

These words enable creation of new vocabulary and provide the tools required for implementing domain-specific languages and specialized programming constructs.

#### String and Parsing Operations

String operations provide text processing capabilities and input parsing support for advanced text manipulation:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `char` | `( "name" -- char )` | Get ASCII value of next character |
| `[char]` | `( "name" -- char )` | Compile-time char |
| `parse` | `( delim -- addr u )` | Parse until delimiter |
| `word` | `( delim -- addr )` | Parse word to counted string |
| `find` | `( addr -- addr 0 \| xt 1 \| xt -1 )` | Find word in dictionary |
| `>number` | `( ud addr u -- ud addr u )` | Convert string prefix to number |
| `count` | `( addr -- addr+1 u )` | Get count from counted string |
| `type` | `( addr u -- )` | Display string |

These operations enable text processing applications and provide the foundation for implementing custom parsing and input processing systems.

#### Compiler State and Control

Compiler control operations provide access to the compilation system and enable meta-programming capabilities:

| Word | Stack Effect | Description |
|------|--------------|-------------|
| `state` | `( -- addr )` | Variable: 0=interpret, <>0=compile |
| `compile,` | `( xt -- )` | Compile execution token |
| `recurse` | `( -- )` | Compile call to current definition |
| `exit` | `( -- )` | Return from current definition |

These operations enable sophisticated compilation techniques and provide the tools required for implementing custom compiling words and domain-specific language features.

## Examples

The following examples demonstrate SUBLEQ eForth's capabilities and illustrate how sophisticated algorithms can be implemented efficiently within the constraints of a single-instruction architecture. These examples serve both as practical programming templates and as demonstrations of the system's computational completeness.

### Mathematical Algorithms

#### Integer Square Root Implementation

The following implementation demonstrates an efficient algorithm for computing integer square roots without requiring floating-point arithmetic:

```forth
: sqrt ( n -- root )
    dup 2 < if exit then
    >r 1 1
    begin
        over r@ <
    while
        dup 2 * 1 + rot + swap 1 +
    repeat
    swap drop r> drop
;

\ Test the algorithm
16 sqrt .          \ Result: 4
625 sqrt .         \ Result: 25
```

This algorithm employs the mathematical property that consecutive perfect squares differ by consecutive odd numbers. Rather than repeatedly calculating complete squares, it constructs them incrementally, providing efficient computation for systems with limited arithmetic capabilities.

The algorithm maintains two values: the current estimate and the current odd number. Each iteration adds the next odd number to build the next perfect square while tracking whether the target has been exceeded. This approach minimizes computational complexity while providing exact results for perfect squares and proper truncation for non-perfect squares.

#### Mandelbrot Set Visualization

This sophisticated example demonstrates how complex mathematical visualizations can be implemented on minimal hardware using fixed-point arithmetic. The Mandelbrot set, defined by the iterative formula z = z² + c, normally requires floating-point arithmetic, but this implementation uses scaled integer mathematics to achieve the same results.

```forth
\ This program renders the Mandelbrot set to a text console. It uses
\ 4-bit fixed-point arithmetic to simulate fractional numbers on an
\ integer-only system. The core algorithm iterates the formula z = z^2 + c
\ for each point on a 2D grid and selects a character to display based
\ on how quickly the point's magnitude escapes a fixed threshold.

\ --- Constants and Configuration ---

16 constant scale           \ The core of the fixed-point math system.
                            \ Represents the value of 1.0. Must be a power of 2.

64 constant thresh          \ Escape threshold. A point has "escaped" if its
                            \ magnitude |z| > 2. This constant is pre-calculated
                            \ for the scaled environment: (2*scale)^2 / scale.

48 constant maxiter         \ Maximum iterations. Prevents infinite loops for
                            \ points that are inside the Mandelbrot set.

\ The boundaries of the complex plane to be rendered. These values are
\ pre-multiplied by 'scale' to avoid repeated calculations later.
-32 constant xmin           \ Real axis (x) minimum, scaled (-2.0 * 16)
 16 constant xmax           \ Real axis (x) maximum, scaled (1.0 * 16)
  1 constant xstep          \ Step size for x-axis (scaled)
-20 constant ymin           \ Imaginary axis (y) minimum, scaled (-1.25 * 16)
 20 constant ymax           \ Imaginary axis (y) maximum, scaled (1.25 * 16)
  1 constant ystep          \ Step size for y-axis (scaled)

\ A simple palette for mapping iteration counts to display characters.
create palette
    char . c, char - c, char - c, char = c, char o c,
    char + c, char * c, char # c, char % c, bl c,

\ --- Variables for Main Loop Control ---

variable x                  \ Stores the current x-coordinate (real part)
variable y                  \ Stores the current y-coordinate (imaginary part)
variable ci                 \ Caches the current y-coord for the inner loop

\ --- Core Words ---

: output ( iter -- )        \ Selects and emits a character from the palette.
    5 / palette + c@ emit ; \ Maps a range of counts to each character.

: muls ( a b -- ab/scale )  \ Performs scaled multiplication for fixed-point numbers.
    * scale / ;             \ Multiplies two scaled numbers and rescales the result.

\ --- Complex Number Arithmetic ---

: c2 ( ar ai -- ar' ai' )    \ Calculates a complex square: z' = z^2
    \ Implements z^2 = (ar^2 - ai^2) + (2*ar*ai)i using scaled math.
    2dup muls 2* >r         \ Calculate new imag part (2*ar*ai/s) and stash it.
    dup muls >r             \ Calculate ai*ai/s and stash it.
    dup muls r> -           \ new_ar = ar*ar/s - ai*ai/s.
    r> ;                    \ Retrieve the stashed new imaginary part.

: abs2 ( ar ai -- |z|^2/scale ) \ Calculates the scaled squared magnitude of z.
    \ Implements |z|^2/s = (ar*ar/s) + (ai*ai/s).
    dup muls swap dup muls + ;

\ --- Mandelbrot Iteration Logic ---

: iter ( cr ci n zr zi -- cr ci n+1 zr' zi' ) \ Performs one z = z^2 + c step.
    c2 >r >r 1+             \ Calculate z^2, stash results, increment n.
    r> 3 pick +             \ zr_new = zr_temp + cr.
    r> 3 pick + ;           \ zi_new = zi_temp + ci.

: point ( cr ci -- n )      \ Calculates escape iterations for a single point.
    0 0 0                   \ Initialize stack with n=0, zr=0, zi=0.
    begin
        iter                \ Perform one z = z^2 + c step.
        2dup abs2 thresh >  \ Check if |z|^2 has escaped the threshold.
        >r 2 pick maxiter > r> or \ Check if max iterations was reached.
    until                   \ Loop until one of the above conditions is true.

    \ The loop is done. The stack is ( cr ci n zr zi ).
    \ This complex sequence correctly isolates 'n' for the result.
    \ Step-by-step:
    \   ( cr ci n zr zi ) drop drop -> ( cr ci n )
    \   ( cr ci n ) rot rot         -> ( n cr ci )
    \   ( n cr ci ) drop drop       -> ( n )
    drop drop rot rot drop drop ;

\ --- Top-Level Rendering Loop ---

: mandel ( -- )             \ Renders the full Mandelbrot set.
    cr
    ymin y !
    begin                   \ Outer loop for the y-coordinate.
        y @ ymax > 0=
    while
        y @ ci !            \ Cache y-value as 'ci' for the inner loop.
        xmin x !            \ Reset x to the start of the row.
        begin               \ Inner loop for the x-coordinate.
            x @ xmax > 0=
        while
            x @ ci @ point output
            x @ xstep + x ! \ Move to the next x position.
        repeat
        cr                  \ Newline after drawing a full row.
        y @ ystep + y !     \ Move to the next y position.
    repeat ;

mandel
```

This program produces the following ASCII visualization of the Mandelbrot set:

```
.................................................
.................................................
............................ --..................
...........................------................
...........................--- - -...............
.........................-----=---...............
.........................---  =----..............
.......................------   ----.............
......................-------   ------...........
..................-------=--    ------...........
.................------- -=      - -----.........
...............-------- = =       --  --.........
.............----------              ----........
..........- ---------o=               ---........
.........-------------                - -........
.......-------  ----                   --........
......-------=  =----                  --........
......------       -=                  --........
.--..----                              --........
.----------                            --........
                                     ----........
.--.--------                         ----........
......-------=                        o--........
......-------                          --........
........------=    =                   --........
..........---- -   --                  --........
...........-----=-----                  -........
............---------                 ----.......
..............----------              ---........
...............--------                --........
................-.-----=           =--  .........
...................--------       ----=-.........
......................------     ------..........
........................----    =----.-..........
.........................----- ------............
.........................------ ---..............
..........................--- -- -...............
............................----.................
.............................---.................
.................................................
.................................................
```

#### Key Implementation Techniques

Fixed-Point Arithmetic: The program uses 16 as the scaling factor, representing 1.0. All fractional values are pre-multiplied by this scale factor. The `muls` word performs scaled multiplication by multiplying two numbers and then dividing by the scale to maintain proper precision.

Complex Number Operations: The `c2` word implements complex squaring using the formula (a + bi)² = (a² - b²) + (2ab)i. The implementation carefully manages the stack to compute both real and imaginary components efficiently.

Escape Detection: The `abs2` word computes the squared magnitude |z|² without taking the square root, since we only need to compare against a threshold. This avoids the expense of square root calculation.

Character Mapping: The program uses a simple palette that maps iteration counts to different ASCII characters, creating visual density gradients that reveal the fractal structure.

Nested Loop Structure: The main rendering loop demonstrates proper nested iteration with careful variable management and stack discipline.

This example showcases SUBLEQ eForth's capability to implement sophisticated mathematical algorithms despite hardware constraints. The fixed-point arithmetic techniques demonstrated here apply to many other computational problems requiring fractional precision on integer-only systems.

### System Programming

#### Memory Dump Utility

A memory dump utility provides insight into system state and enables debugging of memory-related issues:

```forth
: dump ( addr count -- )
    over + swap
    begin
        2dup >
    while
        dup @ u. space
        cell+
    repeat
    2drop cr
;

\ Dump first 10 cells of memory
0 10 dump
```

This utility demonstrates address arithmetic, loop control, and formatted output capabilities while providing a practical tool for system exploration and debugging. Such utilities become essential when working within the minimal error-checking environment of SUBLEQ eForth, where understanding system state is crucial for effective development.

### Defining Utility Functions: Min and Max

This example demonstrates how the concepts covered in this manual combine to create useful functions. The `min` and `max` functions can be implemented in several ways, each illustrating different programming techniques and optimization strategies.

#### Basic Implementation

```forth
: min 2dup > if drop else nip then ;
: max 2dup < if drop else nip then ;
```

This version uses conditional execution with different stack cleanup operations depending on the comparison result.

#### Optimized Implementation

```forth
: min 2dup > if swap then drop ;
: max 2dup < if swap then drop ;
```

This version is slightly more efficient because it always performs the same final operation (`drop`) regardless of the condition, reducing code size and potentially improving performance.

#### Using the Multiplex Function

If your system has the `mux` function available (as SUBLEQ eForth does):

```forth
: min 2dup > mux ;
: max 2dup < mux ;
```

This version leverages the previously defined `mux` function, demonstrating how building up a library of primitive operations enables more concise expression of higher-level functions.

#### Understanding the Stack Effects

All these implementations have the same stack effect:

```forth
min ( n1 n2 -- n )   \ Returns the smaller of two signed numbers
max ( n1 n2 -- n )   \ Returns the larger of two signed numbers
```

To create unsigned versions, simply change the comparison operator:

```forth
: umin 2dup u> if swap then drop ;
: umax 2dup u< if swap then drop ;
```

These examples demonstrate several important Forth concepts:
- Stack manipulation with `2dup` to work with copies
- Conditional execution with `if...then` and `if...else...then`
- The importance of choosing the right comparison operator
- How different implementations can achieve the same result with varying efficiency
- Building complex operations from simpler primitives

The progression from basic to optimized implementations also illustrates the Forth philosophy of programmer responsibility for optimization, since Forth implementations typically provide minimal automatic optimization compared to other language compilers.

## Advanced Programming Topics

### Compiler Modes: Interpretation vs Compilation

SUBLEQ eForth operates in two distinct modes that fundamentally alter how input is processed and executed. Understanding these modes is essential for effective Forth programming and helps explain many aspects of the language's behavior.

#### Interpretation Mode (Command Mode)

In interpretation mode, which serves as the system's default state, the system provides immediate execution of commands:

- Words are executed immediately upon recognition
- Numbers are pushed onto the data stack immediately
- The `ok` prompt appears after successful execution of each command line
- Errors are reported immediately when encountered

```forth
\ In interpretation mode:
2 3 + .            \ Executes immediately, prints: 5
words              \ Executes immediately, shows word list
```

This mode enables interactive exploration, testing, and immediate problem-solving, making it ideal for system exploration and algorithm development.

#### Compilation Mode

Compilation mode is entered automatically by the `:` word and exited by the `;` word:

- Words are compiled into the current definition rather than executed (unless marked immediate)
- Numbers are compiled as literals rather than pushed onto the stack
- No `ok` prompt appears until the definition is completed
- The system builds executable code rather than executing commands

```forth
\ Compilation mode example:
: test-word        \ Enters compilation mode
    2 3 +          \ Compiled into definition
    .              \ Compiled into definition
;                  \ Exits compilation mode, links word

test-word          \ Now executes the compiled definition
```

Understanding this distinction is crucial for effective Forth programming, as it explains why certain operations behave differently in different contexts.

#### Immediate Words

Immediate words represent a special category that executes even during compilation mode, enabling the implementation of control structures and compile-time operations:

```forth
\ Examples of immediate words:
: test             \ : is NOT immediate
    if             \ if IS immediate - executes during compilation
        ." true"   \ ." IS immediate - executes during compilation
    then           \ then IS immediate - executes during compilation
;                  \ ; IS immediate - exits compilation mode
```

Immediate words enable the creation of control structures that appear to be part of the language syntax but are actually ordinary words with special compilation behavior.

#### Manual Mode Switching

Advanced programming techniques sometimes require explicit control over compilation mode:

```forth
\ Switch to interpretation mode during compilation:
: calculate-at-compile-time
    [ 2 3 + ]      \ Calculate 5 at compile time
    literal        \ Compile the result (5) as a literal
    .              \ Compile call to .
;

calculate-at-compile-time  \ Same effect as: : calculate 5 . ;
```

This capability enables sophisticated meta-programming techniques and compile-time optimization.

### Create and Does>: Defining Word Factories

The `create` and `does>` words enable creation of defining words - words that create other words with specific behavior patterns. This capability represents one of Forth's most powerful meta-programming features.

#### Basic Create/Does> Pattern

```forth
\ Define a word that creates constants
: constant ( n "name" -- )
    create ,       \ Create word, compile value
    does> @        \ When created word runs: fetch value
;

42 constant answer
answer .           \ Prints: 42

\ Define a word that creates variables
: variable ( "name" -- )
    create 0 ,     \ Create word, compile initial value 0
    does>          \ When created word runs: return address
;

variable counter
5 counter !       \ Store 5
counter @ .       \ Prints: 5
```

This pattern enables creation of families of related words with consistent behavior, providing powerful abstraction capabilities.

### Understanding SUBLEQ eForth Specific Behavior

SUBLEQ eForth differs from other Forth implementations in several important ways that affect programming and debugging practices. The system is case-sensitive, requiring consistent attention to capitalization when entering commands and defining words. All system words use lowercase conventions.

Some words commonly mentioned in Forth literature are not included in the minimal SUBLEQ eForth base system:

- `roll` - Can be defined as: `: roll ?dup if swap >r 1- recurse r> swap then ;`
- `.s` - May need to be defined or accessed via `system +order`
- Many double-precision words (`d+`, `d*`, etc.)
- Some loop constructs (`do...loop` requires enabling)

Use the `words` command to determine what vocabulary is actually available in your system configuration. The system provides minimal protective mechanisms, requiring careful programming practices. This minimal error checking provides maximum performance but requires disciplined programming practices and careful testing.

### Forth Naming Conventions and Style

Forth employs standardized comment notation and naming patterns that convey meaning through word structure and provide immediate insight into functionality.

#### Stack Effect Documentation Standards

Forth uses standardized comment notation to document word behavior clearly:

```forth
\ Basic format: ( before -- after )
: square ( n -- n^2 )        \ Takes one number, returns its square
    dup *
;

: sum-of-squares ( a b -- sum )  \ Takes two numbers, returns sum of squares
    dup * swap dup * +
;
```

Stack effect comments describe the transformation that occurs when a word executes. These comments are not enforced by the interpreter but serve as essential documentation for understanding word behavior.

#### Parameter Type Conventions

Stack comments use standardized symbols to indicate parameter types:

| Symbol | Type | Description |
|--------|------|-------------|
| `n` | Signed cell | Standard signed integer |
| `u` | Unsigned cell | Unsigned integer |
| `c` | Character | Single byte value |
| `f` | Flag | Boolean value (0 or -1) |
| `d` | Double cell | Double-precision integer |
| `ud` | Unsigned double | Unsigned double-precision |
| `a` | Address | Memory address (aligned) |
| `b` | Character address | Byte-aligned address |
| `xt` | Execution token | Word's execution address |
| `"name"` | Parsed input | Word parsed from input stream |

#### Standard Naming Patterns

Forth employs consistent naming conventions that immediately convey functionality:

Type Prefixes and Suffixes:

| Pattern | Meaning | Example |
|---------|---------|---------|
| `u` prefix | Unsigned operations | `u<`, `u.`, `umax` |
| `d` prefix | Double-cell operations | `d+`, `d.`, `dabs` |
| `c` prefix | Character/byte operations | `c@`, `c!`, `c,` |
| `2` prefix | Operates on pairs | `2dup`, `2drop`, `2swap` |
| `.` prefix | Display operations | `.`, `.s`, `.r` |
| `@` suffix | Fetch operations | `@`, `c@`, `r@` |
| `!` suffix | Store operations | `!`, `c!`, `+!` |

Directional Convention Patterns:

| Pattern | Meaning | Example |
|---------|---------|---------|
| `>` indicates "to" | Conversion direction | `>r`, `s>d`, `>number` |
| `<` indicates "from" | Source direction | `r>`, `0<`, `<#` |
| `?` prefix | Conditional operation | `?dup`, `?do` |
| `#` prefix | Count or number | `#s`, `#>` |
| `+` prefix | Adds to a value | Adding offset to structure |

#### Return Stack Operations

Words that manipulate both stacks require special notation:

```forth
>r ( n --, R: -- n )     \ Move item to return stack
r> ( -- n, R: n -- )     \ Move item from return stack
r@ ( -- n, R: n -- n )   \ Copy from return stack
```

The notation shows effects on both the data stack (before the comma) and return stack (after R:).

#### Conditional Stack Effects

Some words have conditional behavior documented with alternative stack effects:

```forth
?dup ( n -- n n | 0 -- 0 )   \ Duplicate if non-zero, leave if zero
```

The vertical bar `|` separates different possible behaviors based on input conditions.

These conventions provide immediate understanding of word functionality and promote consistent vocabulary development across Forth systems.

## Development and Debugging

### Debugging Techniques in SUBLEQ eForth

Debugging in SUBLEQ eForth requires understanding the system's minimal approach to error reporting and developing techniques that work within its constraints. Effective debugging combines systematic approaches with understanding of the system's limitations.

#### Understanding Error Messages

SUBLEQ eForth provides terse but informative error messages that follow standard Forth conventions:

| Error Code | Meaning | Common Causes |
|------------|---------|---------------|
| `-13?` | Undefined word | Using `roll` (not in base system), typos, missing vocabulary |
| `-4?` | Stack underflow | Operations on empty stack, unbalanced stack usage |
| `-3?` | Stack overflow | Excessive stack usage, infinite recursion |
| `-14?` | Compile-only word | Using compile-only words in interpretation mode |

```forth
undefined-word        \ Results in: undefined-word -13? ok
\ -13 indicates "undefined word" error

.s                    \ If .s is not available: .s -13? ok
\ Solution: system +order to access additional words

\ Stack underflow example:
+                     \ On empty stack: + -4? ok
\ -4 indicates stack underflow
```

Learning these error codes enables rapid problem identification and resolution. Each error code corresponds to a specific type of failure, enabling programmers to quickly identify the nature of problems.

#### Memory and Dictionary Inspection

Understanding system state requires tools for memory and dictionary examination:

```forth
\ Check dictionary usage
here .                \ Current dictionary pointer
unused .              \ Remaining dictionary space

\ Examine recent definitions
words                 \ List all available words

\ Memory dump utility (from examples section)
: dump ( addr count -- )
    over + swap
    begin
        2dup >
    while
        dup @ u. space
        cell+
    repeat
    2drop cr
;
```

These tools provide insight into system state and enable verification of memory operations.

## Performance Considerations

### SUBLEQ-Specific Performance Characteristics

The single-instruction nature of SUBLEQ creates unique performance considerations that differ significantly from conventional processor architectures. Understanding these characteristics enables more effective optimization and realistic performance expectations.

#### Instruction Execution Overhead

Every operation in SUBLEQ requires multiple instruction cycles to accomplish what conventional processors achieve in single instructions. Understanding this overhead enables more informed optimization decisions:

- Simple arithmetic operations require 10-20 SUBLEQ instructions
- Stack manipulation operations involve multiple memory accesses
- Conditional branches require complex instruction sequences
- Memory operations include address calculation overhead

This overhead makes algorithmic efficiency more critical than in conventional systems, where hardware complexity often masks inefficient algorithms. The programmer must consider not just the logical complexity of algorithms but also their implementation complexity in the SUBLEQ environment.

#### Memory Access Patterns

SUBLEQ's memory-centric architecture makes memory access patterns particularly important for performance:

Efficient patterns:
- Sequential memory access that enables predictable addressing
- Batch operations that amortize address calculation costs
- Local data structures that minimize address range requirements
- Return stack usage for temporary storage rather than complex data stack manipulation

Inefficient patterns:
- Random memory access requiring frequent address recalculation
- Excessive stack manipulation requiring many rotation operations
- Frequent conditional branches that disrupt instruction flow
- Deep nesting that requires extensive return stack management

#### Optimization Strategies

Several specific strategies can significantly improve performance in the SUBLEQ environment:

Minimize stack operations requiring complex manipulation:
```forth
\ Efficient: Direct calculation
: efficient-calc ( a b c -- result )
    rot * +
;

\ Inefficient: Excessive manipulation
: inefficient-calc ( a b c -- result )
    rot rot rot swap over rot + *
;
```

Use return stack for temporary storage rather than extensive data stack shuffling:
```forth
: temp-storage ( a b c d -- result )
    >r >r
    +
    r> * r> +
;
```

Factor frequently used operations into separate words to enable code reuse and optimization:
```forth
\ Factor common operations
: square ( n -- n^2 ) dup * ;
: cube ( n -- n^3 ) dup square * ;

\ Rather than repeating dup * inline
```

Batch memory operations rather than individual accesses:
```forth
: efficient-copy ( src dest count -- )
    for
        over @ over ! cell+ swap cell+ swap
    next 2drop
;
```

### Code Organization for Performance

#### Word Size and Complexity

Optimal word size balances reusability against call overhead. In SUBLEQ eForth, function calls involve significant overhead, making the trade-off between modularity and performance more pronounced:

Prefer medium-sized words that accomplish complete operations without excessive decomposition:
```forth
\ Good: Complete operation in reasonable size
: process-data ( data -- result )
    validate-range
    apply-transformation
    normalize-output
;

\ Avoid: Excessive decomposition
: tiny-step-1 ( n -- n ) 1 + ;
: tiny-step-2 ( n -- n ) 2 * ;
: tiny-step-3 ( n -- n ) 3 - ;
: inefficient-process ( n -- result )
    tiny-step-1 tiny-step-2 tiny-step-3
;
```

#### Control Structure Efficiency

Choose control structures that minimize branching overhead:

Prefer `begin...until` for simple loops:
```forth
\ Efficient for simple counting
: countdown ( n -- )
    begin
        dup . 1- dup 0=
    until
    drop
;
```

Use `for...next` for definite iteration:
```forth
\ Efficient for known iteration counts
: repeat-operation ( n -- )
    for
        perform-operation
    next
;
```

Minimize nested conditionals that require complex branching:
```forth
\ Efficient: Early exit pattern
: validate-input ( n -- flag )
    dup 0< if drop 0 exit then
    dup 100 > if drop 0 exit then
    drop -1
;

\ Less efficient: Nested structure
: nested-validate ( n -- flag )
    dup 0< if
        drop 0
    else
        dup 100 > if
            drop 0
        else
            drop -1
        then
    then
;
```

### Memory Management Strategies

#### Dictionary Space Management

Efficient dictionary usage becomes critical in memory-constrained environments:

Monitor dictionary usage regularly:
```forth
: check-space ( -- )
    here ." Dictionary at: " . cr
;
```

Factor definitions to minimize memory fragmentation:
```forth
\ Prefer factored definitions that reuse components
: common-operation ( n -- n' ) dup * 1 + ;
: operation-a ( n -- result ) common-operation 2 * ;
: operation-b ( n -- result ) common-operation 3 + ;

\ Rather than duplicating code in each definition
```

Use constants for frequently referenced values:
```forth
100 constant max-value
0 constant min-value

\ Rather than embedding literals throughout code
```

#### Variable and Data Structure Efficiency

Design data structures for efficient access patterns:

Prefer arrays for sequential access:
```forth
create lookup-table 10 , 20 , 30 , 40 , 50 ,

: table-lookup ( index -- value )
    cells lookup-table + @
;
```

Use variables for frequently accessed state:
```forth
variable current-mode
variable operation-count

\ Rather than storing state on the stack
```

Group related data for efficient access:
```forth
\ Structure for related values
create configuration
    100 ,    \ max-items
    50 ,     \ threshold
    -1 ,     \ enabled flag

: max-items configuration @ ;
: threshold configuration cell+ @ ;
: enabled? configuration 2 cells + @ ;
```

## References

### Essential Reading for SUBLEQ and Forth

#### Foundational Papers and Resources

[URISC: The Ultimate RISC](https://web.ece.ucsb.edu/~parhami/pubs_folder/parh88-ijeee-ultimate-risc.pdf) by Behrooz Parhami provides the theoretical foundation for understanding SUBLEQ and one-instruction computer architectures. This seminal paper demonstrates how universal computation can be achieved with a single instruction type and establishes the theoretical groundwork for implementations like SUBLEQ eForth.

[eForth Overview](http://forth.org/eforth.html) by Bill Muench and Chen-Hanson Ting documents the design philosophy and implementation approach of eForth, the portable Forth kernel that serves as the foundation for SUBLEQ eForth. This resource explains the minimalist approach to Forth implementation and the design decisions that enable portability across diverse architectures.

#### Forth Language Resources

[Starting Forth](https://www.forth.com/starting-forth/) by Leo Brodie remains the classic introduction to Forth programming. Brodie's approachable writing style and practical examples make complex concepts accessible to programmers coming from other language backgrounds. This book provides essential background for understanding Forth's unique programming model and philosophy.

[Thinking Forth](http://thinking-forth.sourceforge.net/) by Leo Brodie addresses advanced Forth programming methodology and system design principles. This book goes beyond basic syntax to explore how to structure large Forth applications, manage complexity, and develop effective programming practices within Forth's minimalist framework.

[ANS Forth Standard](https://forth-standard.org/) provides the official language specification for modern Forth implementations. While SUBLEQ eForth is based on the earlier eForth design, understanding the standard helps place the implementation in context and explains the rationale behind various design decisions.

#### Related Projects and Implementations

[GForth](https://gforth.org/) represents a modern, full-featured Forth implementation that demonstrates contemporary approaches to Forth system design. Studying GForth alongside SUBLEQ eForth illustrates the spectrum of possible implementation approaches and trade-offs between functionality and simplicity.

[JonesForth](https://github.com/AlexandreAbreu/jonesforth) provides a tutorial implementation that explains Forth implementation techniques in detail. The extensive commentary and step-by-step approach make it an excellent complement to studying SUBLEQ eForth's more advanced techniques.

[WAForth](https://github.com/remko/waforth) demonstrates Forth's remarkable portability through a complete interpreter and dynamic compiler written entirely in raw WebAssembly. The entire system fits into a 10k WebAssembly module and uses subroutine threading rather than traditional direct threading due to WebAssembly's structured constraints. WAForth exemplifies how Forth adapts to diverse architectures, from minimal embedded systems like SUBLEQ to modern web platforms.

[Moving Forth](https://www.bradrodriguez.com/papers/) is a series on writing Forth kernels.

[Implementing a Forth](https://ratfactor.com/forth/implementing) makes a list of several Forth implementations.

#### Community Resources and Support

The Forth programming community provides several venues for ongoing learning and assistance:

Reddit's [r/Forth](https://www.reddit.com/r/Forth/) community offers informal discussion, problem-solving assistance, and sharing of projects and techniques. The community includes both beginners and experienced practitioners who can provide guidance on practical Forth usage.

Stack Overflow's [Forth questions](https://stackoverflow.com/questions/tagged/forth) section provides a searchable repository of specific technical questions and solutions. The question-and-answer format makes it particularly useful for resolving specific implementation issues.

The [Forth Interest Group](http://www.forth.org/) (FIG) serves as the primary professional organization for the Forth community, maintaining standards, organizing conferences, and preserving the language's history and development.
