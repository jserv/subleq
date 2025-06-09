# SUBLEQ

```
        ___              _    _         ___    _   _  ___    _      ___    _____
       (  _`\           ( )_ ( )       (  _`\ ( ) ( )(  _`\ ( )    (  _`\ (  _  )
   __  | (_(_)_    _ __ | ,_)| |__     | (_(_)| | | || (_) )| |    | (_(_)| ( ) |
 /'__`\|  _)/'_`\ ( '__)| |  |  _ `\ + `\__ \ | | | ||  _ <'| |  _ |  _)_ | | | |
(  ___/| | ( (_) )| |   | |_ | | | |   ( )_) || (_) || (_) )| |_( )| (_( )| (('\|
`\____)(_) `\___/'(_)   `\__)(_) (_)   `\____)(_____)(____/'(____/'(____/'(___\_)
```

## Introduction
SUBLEQ (SUBtract and branch if Less than or EQual to zero) represents a fascinating class of processors known as
[One Instruction Set Computers](https://en.wikipedia.org/wiki/One-instruction_set_computer) (OISC). Despite their
extreme simplicity, these processors are Turing-complete, proving that sophisticated computation does not require
complex hardware. This project explores that minimalism by running a complete, self-hosting
[Forth](https://www.forth.com/forth/) system on a 16-bit SUBLEQ architecture.

The foundational work for this system was pioneered by [Richard James Howe](https://github.com/howerj/subleq),
who first ported the compact and elegant [eForth](https://www.forth.org/eforth.html) to a SUBLEQ machine. His
project established the viability of building a high-level development environment on a minimal instruction set.

This project continues that legacy by focusing on high-performance execution. It introduces a new, optimized
C-based virtual machine. This optimizer fuses common instruction patterns from the eForth image into
super-instructions, dramatically accelerating the environment beyond the original implementation and making it
practical for interactive use. The result is a complete development environment—including an assembler and
cross-compiler—that is not only functional but reasonably fast.

This endeavor proves a powerful point about software portability: a language system as versatile as Forth can
be adapted to run efficiently on even the most constrained architectures. The project serves as both an
experimental platform and an embodiment of the programming puzzle that makes minimalist computing and the
Forth language so challenging and rewarding.

## Build and Run
This setup requires the installation of a C compiler, [Gforth](https://gforth.org/),
and GNU Make.
* macOS: `brew install gforth`
* Ubuntu Linux / Debian: `sudo apt-get install gforth build-essential`

To run eForth on SUBLEQ, simply type:
```shell
$ make run
```

Below is an example session demonstrating basic usage:
```
words
21 21 + . cr
: hello ." Hello, World!" cr ;
hello
bye
```

This allows you to operate eForth within the system. For a list of available
commands, enter `words` and press Enter. In Forth, the term "word" refers to
a function. It is called a "word" because Forth functions are typically named
using space-delimited characters, often forming a single, descriptive term.
Words are organized into vocabularies, which collectively make up the dictionary
in Forth. Numbers are input in Reverse Polish Notation (RPN); for instance,
inputting:
```
2 2 + . cr
```

will display `4`.

To define a new function, use the following format:
```
: hello cr ." Hello, World!" ;
```

Remember that spaces are critical in eForth syntax. After defining a function,
simply type `hello` to execute it.

The system is self-hosting, meaning it can generate new eForth images using
the current eForth image and source code. While Gforth is used to compile the
image from `subleq.fth`, the Forth system's self-hosting capability also allows
for building new images after modifying any Forth source files. To initiate
self-hosting and validation, run `make bootstrap`.

## SUBLEQ
SUBLEQ machines belong to a class called OISC, which uses a single instruction
to perform any computable task, albeit inefficiently. SUBLEQ originated from the
"URISC" concept introduced in the 1988 paper
[URISC: The Ultimate Reduced Instruction Set Computer](https://web.ece.ucsb.edu/~parhami/pubs_folder/parh88-ijeee-ultimate-risc.pdf).
The intent was to provide a simple platform for computer engineering students to
design their own instruction sets and microcode. SUBLEQ falls under arithmetic-based
OISCs, in contrast to other types like bit-manipulation or Transport Triggered
Architectures (MOVE-based). Despite its simplicity, SUBLEQ could be made more
efficient with the addition of just one extra instruction, such as NAND or a
Right Shift.

A single SUBLEQ instruction is structured as follows:
```
SUBLEQ a, b, c
```

Since SUBLEQ is the only available instruction, it is often abbreviated simply
as:
```
a b c
```

The three operands `a`, `b`, and `c` are stored in three consecutive memory
locations. Each operand represents an address in memory. The SUBLEQ instruction
performs the following operations in pseudo-code:
```python
    r = m[b] - m[a]
    if r <= 0:
        pc = c
    m[b] = r
```

There are three notable exceptions to the standard operation:
1. Halting Execution: If the address `c` is negative or refers to an invalid
   memory location outside the addressable range, the program halts.
2. Input Operation: If the address `a` is `-1`, a byte is read from the input
   and stored at address `b`.
3. Output Operation: If the address `b` is negative, a byte from address `a` is
   sent to the output.

The SUBLEQ specification does not dictate how numbers are represented. Key
considerations include:
- Bit Length: Numbers can be 8-bit, 16-bit, 32-bit, 64-bit, or even arbitrary
  precision.
- Negative Numbers: Typically implemented using two's complement, but other
  methods like sign-magnitude can also be used.

## eForth
The image is a variant of Forth known as "eForth." This implementation of eForth
differs from standard ANS Forth implementations, notably lacking constructs like
the "do...loop" and its variants. The concept behind eForth was to develop a
system that required only a small set of primitives, around 30, written in
assembly to create a highly portable and reasonably efficient Forth. Bill
Muench originally developed eForth, with later enhancements made by Dr.
Chen-Hanson Ting.

`subleq.fth` functions as both a cross-compiler and an eForth interpreter,
specifically designed for SUBLEQ. Written entirely in Forth, `subleq.fth` has
been verified for compatibility with Gforth and can also be executed using
a pre-generated eForth image running on a SUBLEQ machine.

The cross-compilation process functions as outlined below:
1. Assembler: A specialized assembler for the SUBLEQ architecture enables
   low-level machine code generation tailored to the instruction set.
2. Virtual machine: Leveraging the SUBLEQ assembler, a virtual machine is
   constructed. This VM is capable of supporting higher-level programming
   constructs, facilitating the seamless execution of Forth code within the
   SUBLEQ environment.
3. Forth word definitions: These definitions are instrumental in building
   a full-fledged Forth interpreter, allowing for the creation, compilation,
   and execution of Forth programs.
4. Forth image: The finalized Forth image, encapsulating the interpreter and
   its environment, is output to the standard output stream. This image
   initializes the VM with the necessary configurations and word definitions to
   operate effectively.

"Meta-compilation" in Forth refers to a process similar to cross-compilation,
though the term carries a distinct meaning in the Forth community compared to
its use in broader computer science. This difference stems from Forth's
evolution within the microcomputer scene, which was separate from the academic
environment of the 1980s and earlier. The term "meta-compilation" may have been
somewhat mistranslated. While most modern programs employ unit testing
frameworks, here, a meta-compilation system serves as an extensive testing
mechanism. If the system compiles image "A," which can then compile another
image "B," and "B" matches "A" byte-for-byte, this gives reasonable confidence
that the image is correct, or at least correct enough for self-compilation.

This process is performed with the following commands:
```shell
$ gforth subleq.fth > stage0.dec
$ cc -o subleq subleq.c
$ ./subleq stage0.dec < subleq.fth > stage1.dec
$ diff -w stage0.dec stage1.dec
```

The `stage0.dec` image was initially generated using Gforth to create the first
functional eForth for the SUBLEQ machine. Once the eForth image is ready, it
serves as the meta-compiler, capable of compiling itself and generating
`stage1.dec`. The image generated by Gforth should be identical to the one
produced by SUBLEQ eForth when using the same `subleq.fth` file. If the two
images are identical, the bootstrapping process is considered complete. Although
the Gforth interpreter is no longer required, it is retained because it is
significantly faster than using SUBLEQ eForth to compile a new image.

It is noteworthy that approximately half of the memory allocated is dedicated to
the virtual machine, which facilitates the writing and execution of Forth code.
The `BLOCK` word-set within this implementation does not interact directly with
mass storage. Instead, it maps blocks to memory, enabling efficient memory
management and access.

## License
SUBLEQ is released under the BSD 2 clause license. Use of this source code is governed by
a BSD-style license that can be found in the LICENSE file.

## Reference
* [SUBLEQ EFORTH: Forth Metacompilation for a SUBLEQ Machine](https://www.amazon.com/dp/B0B5VZWXPL)
* [Subleq Toolchain Bootstrapping](https://github.com/jvorob/subleq-bootstrap)
* [Higher Subleq (HSQ): a typeless simplified C-like language which compiles into Subleq](https://esolangs.org/wiki/Higher_Subleq)
* [eForth in C/C++](https://github.com/chochain/eforth)
