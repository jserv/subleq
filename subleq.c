/*
 * subleq.c - A 16-bit SUBLEQ CPU running eForth.
 *
 * This file implements a virtual machine for a 16-bit SUBLEQ (Subtract and
 * Branch if Less than or Equal to zero) machine. It includes an optimizer
 * to convert common SUBLEQ instruction sequences into single, faster
 * extended operations for improved performance with programs like eForth.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Platform detection for POSIX systems (Unix, macOS, etc.) */
#if defined(unix) || defined(__unix__) || defined(__unix) || \
    (defined(__APPLE__) && defined(__MACH__))
#define PLAT_POSIX
#endif

/* Include POSIX-specific headers for terminal control */
#ifdef PLAT_POSIX
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

/* Tail-call optimization attribute */
#if defined(__has_attribute) && __has_attribute(musttail)
#define MUST_TAIL __attribute__((musttail))
#else
#define MUST_TAIL
#endif

/* Compiler-specific attributes for optimization */
#if defined(__clang__) || defined(__GNUC__)
#define HOT_PATH __attribute__((hot))
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define UNREACHABLE __builtin_unreachable()
#else
#define HOT_PATH
#define UNLIKELY(x) (x)
#define UNREACHABLE \
    do {            \
    } while (0)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define HAS_BUILTIN_CONSTANT_P 1
#define IS_COMPILE_TIME_CONSTANT(x) __builtin_constant_p(x)
#else
#define HAS_BUILTIN_CONSTANT_P 0
#define IS_COMPILE_TIME_CONSTANT(x) 0
#endif

/* Memory size for 16-bit addressing (2^16 = 65536 words) */
#define SZ (1U << 16)

/* Mask address to 16-bit range (0-65535) */
#define MASK_ADDR(a) ((a) & (SZ - 1))

/* Create a mask for N-bit values */
#define MASK_BITS(nbits) \
    ((nbits) < 16 ? (uint16_t) ((1UL << (nbits)) - 1) : (uint16_t) 0xFFFFUL)

/* Maximum depth for optimizer pattern scanning */
#define OPTIMIZER_SCAN_DEPTH (3 * 64)

/* Profiler constants */
#define MAX_HOT_SPOTS 64
#define PROFILER_SAMPLE_RATE 1000 /* Sample every N instructions */

/* Pattern analysis constants - these represent the structure of specific
 * SUBLEQ instruction sequences that the optimizer recognizes */
#define SUBLEQ_INSN_SIZE 3 /* Each SUBLEQ instruction uses 3 memory words */

/* Calculate pattern-specific jump target offsets based on instruction structure
 */
#define ILOAD_PATTERN_JUMP_OFFSET 15 /* Original: i + 15 */
#define IJMP_PATTERN_JUMP_OFFSET 14  /* Original: i + (3 * 4) + 2 = i + 14 */
#define LDINC_INCREMENT_OFFSET 24    /* Original: 24 (ILOAD pattern size) */

#ifdef PLAT_POSIX
/* Read a character from input. For interactive terminals, this function uses
 * poll() to block indefinitely until input is available.
 */
static int vm_getch(FILE *in)
{
    int fd = fileno(in);
    if (!isatty(fd))
        return fgetc(in);

    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    int n;

    /* Use poll with a timeout of -1 to wait indefinitely for input. */
    while ((n = poll(&pfd, 1, -1)) < 0) {
        if (errno != EAGAIN && errno != EINTR)
            return -1; /* A real error occurred */
        /* If interrupted by a signal, just poll again. */
    }

    /* If we are here, poll() returned > 0, so data is ready. */
    unsigned char ch;
    if (read(fd, &ch, 1) > 0)
        return ch;

    return -1; /* EOF or read error */
}

/* Write a character to output stream, flushing for TTY */
static int vm_putch(int ch, FILE *out)
{
    if (fputc(ch, out) < 0)
        return -1;
    if (isatty(fileno(out)))
        fflush(out);
    return ch;
}
#else
/* Fallback for non-POSIX systems */
static int vm_getch(FILE *in)
{
    return fgetc(in);
}

static int vm_putch(int ch, FILE *out)
{
    if (fputc(ch, out) < 0 || fflush(out) < 0)
        return -1;
    return ch;
}
#endif

/* Extended instruction set with increment values */
#define INSN_LIST \
    _(SUBLEQ, 3)  \
    _(JMP, 0)     \
    _(ADD, 9)     \
    _(SUB, 3)     \
    _(MOV, 12)    \
    _(ZERO, 3)    \
    _(PUT, 3)     \
    _(GET, 3)     \
    _(HALT, 0)    \
    _(IADD, 21)   \
    _(ISUB, 15)   \
    _(IJMP, 0)    \
    _(ILOAD, 24)  \
    _(ISTORE, 36) \
    _(INC, 3)     \
    _(DEC, 3)     \
    _(INV, 21)    \
    _(NEG, 6)     \
    _(LSHIFT, 9)  \
    _(DOUBLE, 9)  \
    _(LDINC, 27)

/* clang-format off */
enum {
#define _(inst, inc) inst,
    INSN_LIST
#undef _
    IMAX
};
/* clang-format on */

enum {
#define _(inst, inc) INSN_INCR_##inst = inc,
    INSN_LIST
#undef _
};

static const char *insn_names[] = {
#define _(inst, inc) #inst,
    INSN_LIST
#undef _
};

/* Optimized instruction structure */
typedef struct {
    uint8_t opcode; /* Instruction opcode (from INSN_LIST) */
    uint16_t src;   /* Source operand address/value */
    uint16_t dst;   /* Destination operand address */
    uint16_t aux;   /* Auxiliary operand (e.g., SUBLEQ jump target) */
} insn_t;

/* Hot spot tracking for profiler */
typedef struct {
    uint64_t pc;         /* Program counter address */
    uint64_t exec_count; /* Execution count */
    uint8_t opcode;      /* Most frequent opcode */
} hot_spot_t;

/* Lightweight profiler state */
typedef struct {
    bool enabled;                        /* Profiler enabled flag */
    uint64_t total_instructions;         /* Total instruction count */
    uint64_t memory_accesses;            /* Total memory access count */
    uint64_t *pc_heat_map;               /* PC execution heat map */
    hot_spot_t hot_spots[MAX_HOT_SPOTS]; /* Top hot spots */
    size_t hot_spot_count;               /* Number of valid hot spots */
    clock_t start_time;                  /* Profiling start time */
    clock_t end_time;                    /* Profiling end time */
} profiler_t;

/* Optimizer state */
typedef struct {
    int matches[IMAX];        /* Count of matched instructions */
    unsigned set[10];         /* Tracks set variables ('0'-'9') */
    uint16_t vars[10];        /* Captured variable values */
    unsigned version;         /* Version counter for variable reset */
    int64_t exec_count[IMAX]; /* Execution count per instruction */
    uint8_t zero_reg[SZ];     /* Tracks memory locations holding 0 */
    uint8_t one_reg[SZ];      /* Tracks memory locations holding 1 */
    uint8_t neg1_reg[SZ];     /* Tracks memory locations holding 0xFFFF */
    clock_t start, end;       /* Timers for performance measurement */
} optimizer_t;

/* Main VM context */
typedef struct {
    uint16_t *mem;         /* Main memory (16-bit words) */
    insn_t *insn_mem;      /* Optimized instruction memory */
    uint64_t nbits;        /* Word size in bits (e.g., 16) */
    uint16_t mask;         /* Bitmask for N-bit values */
    uint64_t mem_size;     /* Total memory size in words */
    uint64_t pc;           /* Program counter */
    uint64_t load_size;    /* Loaded memory size */
    uint64_t max_addr;     /* Highest address written */
    optimizer_t opt;       /* Optimizer state */
    profiler_t prof;       /* Profiler state */
    FILE *in, *out;        /* Input/output streams */
    int error;             /* Error flag (0 = no error, -1 = error) */
    bool stats_enabled;    /* Enable performance statistics */
    bool optimize_enabled; /* Enable instruction optimization */
    bool profiler_enabled; /* Enable lightweight profiler */
} vm_t;

/* Forward declaration for the dispatcher with the unified signature */
static void dispatch(vm_t *vm, uint64_t pc, const insn_t *insn);

/* Pattern analysis helper functions */

/* Validate that a captured jump target matches the expected pattern structure.
 * This ensures that the SUBLEQ sequences we're optimizing have the correct
 * control flow relationships between instructions.
 */
static inline bool validate_jump_target(uint16_t target,
                                        uint64_t base_pc,
                                        int offset)
{
    return target == (base_pc + offset);
}

/* Profiler helper functions */
static void profiler_init(vm_t *vm)
{
    profiler_t *prof = &vm->prof;

    if (!vm->profiler_enabled) {
        prof->enabled = false;
        return;
    }

    prof->enabled = true;
    prof->total_instructions = 0;
    prof->memory_accesses = 0;
    prof->hot_spot_count = 0;
    prof->start_time = clock();

    /* Allocate PC heat map if profiling enabled */
    prof->pc_heat_map = calloc(vm->mem_size, sizeof(uint64_t));
    if (!prof->pc_heat_map) {
        fprintf(stderr, "Warning: Failed to allocate profiler memory\n");
        prof->enabled = false;
    }
}

static void profiler_cleanup(vm_t *vm)
{
    profiler_t *prof = &vm->prof;
    free(prof->pc_heat_map);
    prof->pc_heat_map = NULL;
    prof->enabled = false;
}

static inline void profiler_record_pc(vm_t *vm, uint64_t pc)
{
    profiler_t *prof = &vm->prof;

    if (!prof->enabled)
        return;

    prof->total_instructions++;

    /* Record PC heat map every instruction */
    if (prof->pc_heat_map)
        prof->pc_heat_map[pc]++;
}

static inline void profiler_record_memory_access(vm_t *vm)
{
    profiler_t *prof = &vm->prof;
    if (prof->enabled)
        prof->memory_accesses++;
}

/* Define instruction handlers.
 * It accepts a pointer to the current instruction (@insn) to avoid redundant
 * memory lookups for its operands. The tail call to dispatch passes NULL for
 * the unused @insn parameter to maintain signature compatibility, which is
 * required for the 'musttail' attribute.
 */
#define HANDLE(inst, body)                                    \
    HOT_PATH static void handle_##inst(vm_t *vm, uint64_t pc, \
                                       const insn_t *insn)    \
    {                                                         \
        (void) pc;                                            \
        (void) insn;                                          \
                                                              \
        /* Profiler hook - record PC execution */             \
        profiler_record_pc(vm, pc);                           \
                                                              \
        uint64_t next_pc = pc + INSN_INCR_##inst;             \
        do                                                    \
            body while (0);                                   \
        if (UNLIKELY(vm->error))                              \
            return;                                           \
        MUST_TAIL return dispatch(vm, next_pc, NULL);         \
    }

/* SUBLEQ: Subtract and branch if less than or equal to zero */
HANDLE(SUBLEQ, {
    /* Operands are pre-fetched from memory by the optimizer */
    uint16_t a = insn->src;
    uint16_t b = insn->dst;
    uint16_t c = insn->aux;

    if (UNLIKELY(a == vm->mask)) { /* Input */
        int ch = vm_getch(vm->in);
        if (UNLIKELY(ch == EOF || ch == -1)) {
            vm->error = -1;
            return;
        }
        vm->mem[MASK_ADDR(b)] = (uint16_t) ch;
        profiler_record_memory_access(vm);
    } else if (UNLIKELY(b == vm->mask)) { /* Output */
        profiler_record_memory_access(vm);
        if (UNLIKELY(vm_putch(vm->mem[MASK_ADDR(a)], vm->out) < 0)) {
            vm->error = -1;
            return;
        }
    } else { /* Standard SUBLEQ */
        uint16_t la = MASK_ADDR(a);
        uint16_t lb = MASK_ADDR(b);
        profiler_record_memory_access(vm); /* Read from la */
        profiler_record_memory_access(vm); /* Read from lb */
        uint16_t result = vm->mem[lb] - vm->mem[la];
        vm->mem[lb] = result;
        profiler_record_memory_access(vm); /* Write to lb */
        if (UNLIKELY(lb > vm->max_addr))
            vm->max_addr = lb;
        if (result == 0 || (result & (1U << (vm->nbits - 1))))
            next_pc = c;
    }
})

/* JMP: Unconditional jump */
HANDLE(JMP, {
    uint16_t dst = insn->dst;
    /* Often the address cleared to 0 by the JMP sequence */
    uint16_t src = insn->src;
    vm->mem[MASK_ADDR(src)] = 0;
    profiler_record_memory_access(vm);
    next_pc = dst;
})

/* MOV: Move data */
HANDLE(MOV, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read from src */
    vm->mem[MASK_ADDR(dst)] = vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write to dst */
})

/* ADD: Addition */
HANDLE(ADD, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read from src */
    profiler_record_memory_access(vm); /* Read from dst */
    /* Unsigned arithmetic provides wrap-around behavior automatically */
    vm->mem[MASK_ADDR(dst)] += vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write to dst */
})

/* SUB: Subtraction */
HANDLE(SUB, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read from src */
    profiler_record_memory_access(vm); /* Read from dst */
    vm->mem[MASK_ADDR(dst)] -= vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write to dst */
})

/* ZERO: Clear memory location */
HANDLE(ZERO, {
    uint16_t dst = insn->dst;
    vm->mem[MASK_ADDR(dst)] = 0;
    profiler_record_memory_access(vm);
})

/* PUT: Output character */
HANDLE(PUT, {
    uint16_t src = insn->src;
    profiler_record_memory_access(vm);
    if (UNLIKELY(vm_putch(vm->mem[MASK_ADDR(src)], vm->out) < 0)) {
        vm->error = -1;
        return;
    }
})

/* GET: Input character */
HANDLE(GET, {
    uint16_t dst = insn->dst;
    int ch = vm_getch(vm->in);
    if (UNLIKELY(ch == EOF || ch == -1)) {
        vm->error = -1;
        return;
    }
    vm->mem[MASK_ADDR(dst)] = (uint16_t) ch;
    profiler_record_memory_access(vm);
})

/* HALT: Terminate program */
HANDLE(HALT, {
    /* Set PC beyond valid range to stop execution */
    vm->pc = vm->mem_size / 2;
    return;
})

/* IADD: Indirect addition */
HANDLE(IADD, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read pointer */
    profiler_record_memory_access(vm); /* Read src */
    uint16_t addr = MASK_ADDR(vm->mem[MASK_ADDR(dst)]);
    profiler_record_memory_access(vm); /* Read indirect */
    vm->mem[addr] += vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write indirect */
})

/* ISUB: Indirect subtraction */
HANDLE(ISUB, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read pointer */
    profiler_record_memory_access(vm); /* Read src */
    uint16_t addr = MASK_ADDR(vm->mem[MASK_ADDR(dst)]);
    profiler_record_memory_access(vm); /* Read indirect */
    vm->mem[addr] -= vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write indirect */
})

/* IJMP: Indirect jump */
HANDLE(IJMP, {
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm);
    next_pc = vm->mem[MASK_ADDR(dst)];
})

/* ILOAD: Indirect load */
HANDLE(ILOAD, {
    uint16_t src = insn->src;
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read pointer */
    uint16_t addr = vm->mem[MASK_ADDR(src)];
    /* Special handling for input from I/O address (vm->mask) */
    if (UNLIKELY(addr == vm->mask)) {
        int ch = vm_getch(vm->in);
        if (UNLIKELY(ch == EOF || ch == -1)) {
            vm->error = -1;
            return;
        }
        vm->mem[MASK_ADDR(dst)] = (uint16_t) (-ch); /* Negated input value */
    } else {
        profiler_record_memory_access(vm); /* Read indirect */
        vm->mem[MASK_ADDR(dst)] = vm->mem[MASK_ADDR(addr)];
    }
    profiler_record_memory_access(vm); /* Write to dst */
})

/* LDINC: D = m[m[S]], then m[S]++ */
HANDLE(LDINC, {
    uint16_t src_ptr = insn->src;      /* S */
    uint16_t dst = insn->dst;          /* D */
    profiler_record_memory_access(vm); /* Read pointer */
    uint16_t addr = vm->mem[MASK_ADDR(src_ptr)];

    /* Special handling for input from I/O address (vm->mask) */
    if (UNLIKELY(addr == vm->mask)) {
        int ch = vm_getch(vm->in);
        if (UNLIKELY(ch == EOF || ch == -1)) {
            vm->error = -1;
            return;
        }
        vm->mem[MASK_ADDR(dst)] = (uint16_t) (-ch); /* Negated input value */
    } else {
        profiler_record_memory_access(vm); /* Read indirect */
        vm->mem[MASK_ADDR(dst)] = vm->mem[MASK_ADDR(addr)];
    }

    /* Post-increment the source pointer */
    vm->mem[MASK_ADDR(src_ptr)]++;
    profiler_record_memory_access(vm); /* Write to dst */
    profiler_record_memory_access(vm); /* Write pointer increment */
})

/* ISTORE: Indirect store */
HANDLE(ISTORE, {
    uint16_t src = insn->src;
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read src */
    profiler_record_memory_access(vm); /* Read pointer */
    vm->mem[MASK_ADDR(vm->mem[MASK_ADDR(dst)])] = vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write indirect */
})

/* INC: Increment by 1 */
HANDLE(INC, {
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read */
    vm->mem[MASK_ADDR(dst)]++;
    profiler_record_memory_access(vm); /* Write */
})

/* DEC: Decrement by 1 */
HANDLE(DEC, {
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read */
    vm->mem[MASK_ADDR(dst)]--;
    profiler_record_memory_access(vm); /* Write */
})

/* INV: Bitwise NOT */
HANDLE(INV, {
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read */
    vm->mem[MASK_ADDR(dst)] = ~vm->mem[MASK_ADDR(dst)];
    profiler_record_memory_access(vm); /* Write */
})

/* LSHIFT: Left shift by constant */
HANDLE(LSHIFT, {
    uint16_t src = insn->src;          /* Shift count */
    uint16_t dst = insn->dst;          /* Value to be shifted */
    profiler_record_memory_access(vm); /* Read */
    vm->mem[MASK_ADDR(dst)] <<= src;
    profiler_record_memory_access(vm); /* Write */
})

/* DOUBLE: Multiply by 2 (left shift by 1) */
HANDLE(DOUBLE, {
    uint16_t dst = insn->dst;
    profiler_record_memory_access(vm); /* Read */
    vm->mem[MASK_ADDR(dst)] <<= 1;
    profiler_record_memory_access(vm); /* Write */
})

/* NEG: Two's complement negation (dst = 0 - src) */
HANDLE(NEG, {
    uint16_t dst = insn->dst;
    uint16_t src = insn->src;
    profiler_record_memory_access(vm); /* Read src */
    vm->mem[MASK_ADDR(dst)] = 0 - vm->mem[MASK_ADDR(src)];
    profiler_record_memory_access(vm); /* Write dst */
})

/* Pattern matching function for SUBLEQ instruction optimization.
 * Matches instruction sequences against patterns using a compact
 * domain-specific language.
 *
 * Pattern symbols:
 * '0'-'9':
 *   Variable capture/match. These symbols capture the value of the current
 *   memory word (mem[pc + offset]) into a numbered variable. If the variable
 *   is already bound, it asserts that the current value matches the bound
 *   value. Example: '0' captures mem[i], subsequent '0's must match mem[i].
 *
 * 'Z':
 *   Match Zero. Requires the current memory word to be exactly 0.
 *   Example: "Z Z >" matches three consecutive zeros, where the third zero is
 *   also the next PC address.
 *
 * 'N':
 *   Match Negative One (All Bits Set). Requires the current memory word to be
 *   equal to 'vm->mask' (e.g., 0xFFFF for a 16-bit VM). This is often used
 *   for memory-mapped I/O addresses or special constant values.
 *   Example: "N ! >" implies 'SUBLEQ 0xFFFF, addr, next_pc'.
 *
 * '>':
 *   Match Next Program Counter Address. Requires the current memory word to be
 *   '(pc + offset + 1)'. This is the address that SUBLEQ would jump to if the
 *   preceding SUBLEQ instruction's result is zero or negative. Crucial for
 *   matching linear code sequences that do not branch.
 *   Example: "0 1 >" matches a SUBLEQ where the target address is the next
 *   sequential instruction.
 *
 * '%':
 *   Match Specific Constant (from 'va_arg'). Consumes an argument from the
 *   'va_list' (expected to be a 'uint16_t'). Requires the current memory word
 *   to match this provided constant.
 *   Example: 'match_pattern(vm, ..., "%", 100)' would match if the current
 *   memory word is 100.
 *
 * '!':
 *   Capture Value to Pointer (from 'va_arg'). Consumes an argument from the
 *   'va_list' (expected to be a 'uint16_t *'). Stores the value of the
 *   current memory word into the provided pointer. Does not perform a match;
 *   solely for extraction. Example: 'match_pattern(vm, ..., "!", &my_var)'
 *   captures the current memory word into 'my_var'.
 *
 * '?':
 *   Wildcard. Matches any value in the current memory word. Does not capture
 *   or compare the value. Used to skip irrelevant words in a pattern.
 *   Example: "0 ? >" matches '0' followed by any value, then the next PC
 * address.
 *
 * 'P':
 *   Match Positive (non-zero, MSB clear for signed interpretation).
 *
 * 'M':
 *   Match Memory address within valid range.
 *
 * 'R':
 *   Match Register/Variable reference (captured variable).
 *
 * @vm: Virtual machine context
 * @pc: The base program counter for the current instruction sequence
 * being matched (usually 'i' from the optimizer loop)
 * @mem: A pointer to the full main memory array of the VM (vm->mem)
 * @max_len: The maximum number of words available for scanning from 'pc'
 * @pattern: The string containing the DSL pattern to match
 * @...: Variable arguments based on pattern symbols like '%' and '!'
 *
 * Return true if pattern matches, false otherwise
 */
static bool match_pattern(vm_t *vm,
                          uint64_t pc,
                          const uint16_t *mem,
                          int max_len,
                          const char *pattern,
                          ...)
{
    va_list args;
    optimizer_t *opt = &vm->opt;
    uint64_t offset = 0;
    unsigned version = ++opt->version;
    bool result = true;

    /* Early validation of input parameters */
    if (UNLIKELY(!pattern || !mem || max_len <= 0))
        return false;

    va_start(args, pattern);

    /* Iterate through each character in the pattern */
    for (const char *p = pattern; *p && result; ++p) {
        char sym = *p;

        /* Skip whitespace characters for better pattern readability */
        if (isspace(sym))
            continue;

        /* Check if we've exceeded the available memory range */
        if (UNLIKELY(offset >= (uint64_t) max_len)) {
            result = false;
            break;
        }

        uint16_t val = mem[MASK_ADDR(pc + offset)];

        switch (sym) {
        case '0' ... '9': {
            /* Variable capture and matching */
            int var_idx = sym - '0';
            if (opt->set[var_idx] == version) {
                /* Variable already bound - verify it matches */
                if (UNLIKELY(opt->vars[var_idx] != val)) {
                    result = false;
                }
            } else {
                /* First occurrence - capture the value */
                opt->set[var_idx] = version;
                opt->vars[var_idx] = val;
            }
            break;
        }

        case 'Z':
            /* Match zero value */
            if (UNLIKELY(val != 0)) {
                result = false;
            }
            break;

        case 'N':
            /* Match negative one (all bits set) */
            if (UNLIKELY(val != vm->mask)) {
                result = false;
            }
            break;

        case '>':
            /* Match next program counter address */
            if (UNLIKELY(val != pc + offset + 1)) {
                result = false;
            }
            break;

        case '%': {
            /* Match specific constant from arguments */
            uint16_t expected = (uint16_t) va_arg(args, int);
            if (UNLIKELY(val != expected)) {
                result = false;
            }
            break;
        }

        case '!': {
            /* Capture value to pointer from arguments */
            uint16_t *ptr = va_arg(args, uint16_t *);
            if (ptr) {
                *ptr = val;
            }
            break;
        }

        case '?':
            /* Wildcard - matches any value, no operation needed */
            break;

        case 'P':
            /* Match positive value (non-zero, MSB clear) */
            if (UNLIKELY(val == 0 || (val & (1U << (vm->nbits - 1))))) {
                result = false;
            }
            break;

        case 'M':
            /* Match valid memory address */
            if (UNLIKELY(val >= vm->mem_size && val != vm->mask)) {
                result = false;
            }
            break;

        case 'R': {
            /* Match previously captured variable reference */
            char var_ref = (char) va_arg(args, int);
            int var_idx = var_ref - '0';
            if (UNLIKELY(var_idx < 0 || var_idx > 9 ||
                         opt->set[var_idx] != version ||
                         opt->vars[var_idx] != val)) {
                result = false;
            }
            break;
        }

        default:
            /* Unknown pattern symbol */
            result = false;
            break;
        }

        /* Move to next memory location */
        offset++;
    }

    va_end(args);
    return result;
}

/* Retrieve variables captured during pattern matching in the optimizer.
 * Variables are identified by single digits '0'-'9' and must be set in the
 * current optimizer version context to be considered valid.
 *
 * @opt: Pointer to optimizer state containing variable bindings
 * @var: Variable identifier character ('0' through '9')
 * Return the captured variable value, or 0xFFFF if variable is invalid/unset
 */
static inline uint16_t get_var(const optimizer_t *opt, char var)
{
#if HAS_BUILTIN_CONSTANT_P
    /* Fast path: compile-time bounds check for literal constants */
    if (IS_COMPILE_TIME_CONSTANT(var)) {
        if (var < '0' || var > '9')
            return (uint16_t) -1;
        const int idx = var - '0';
        return (opt->set[idx] == opt->version) ? opt->vars[idx] : (uint16_t) -1;
    }
#endif

    /* Runtime path: single bounds check with early return */
    const unsigned char uvar = (unsigned char) var;
    if (UNLIKELY(uvar < '0' || uvar > '9'))
        return (uint16_t) -1;

    const int idx = (int) (uvar - '0');

    /* Branchless version check and value retrieval */
    const bool is_set = (opt->set[idx] == opt->version);
    return is_set ? opt->vars[idx] : (uint16_t) -1;
}

/* Identifies common SUBLEQ sequences and replaces them with single extended
 * instructions. This optimization is crucial for improving the performance
 * of programs compiled to SUBLEQ, especially for high-level languages like
 * Forth which involve frequent stack, memory, and arithmetic operations that
 * translate into many primitive SUBLEQ instructions.
 *
 * @vm: Virtual machine context
 * @proglen: The total number of loaded SUBLEQ words in memory (vm->m)
 */
static void optimize(vm_t *vm, uint64_t proglen)
{
    optimizer_t *opt = &vm->opt;
    const uint16_t *mem = vm->mem;
    insn_t *insn_mem = vm->insn_mem;

    memset(opt->zero_reg, 0, sizeof(opt->zero_reg));
    memset(opt->one_reg, 0, sizeof(opt->one_reg));
    memset(opt->neg1_reg, 0, sizeof(opt->neg1_reg));

    for (uint64_t i = 0; i < proglen; i++) {
        opt->zero_reg[i] = (mem[i] == 0);
        opt->one_reg[i] = (mem[i] == 1);
        opt->neg1_reg[i] = (mem[i] == vm->mask);

        insn_mem[i].opcode = SUBLEQ;
        insn_mem[i].src = mem[MASK_ADDR(i)];
        insn_mem[i].dst = mem[MASK_ADDR(i + 1)];
        insn_mem[i].aux = mem[MASK_ADDR(i + 2)];
    }

    for (uint64_t i = 0; i < proglen; i++) {
        size_t scan_depth = (i + OPTIMIZER_SCAN_DEPTH > proglen)
                                ? proglen - i
                                : OPTIMIZER_SCAN_DEPTH;
        if (scan_depth == 0)
            continue;

        /* ISTORE: m[m[D]] = S */
        if (match_pattern(vm, i, mem, (int) scan_depth,
                          "0Z> 11> 22> Z3> Z4> ZZ> 56> 77> Z7> 6Z> ZZ> 66>")) {
            insn_mem[i].opcode = ISTORE;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '0'));
            insn_mem[i].src = MASK_ADDR(get_var(opt, '5'));
            opt->matches[ISTORE]++;
            continue;
        }

        /* ILOAD and LDINC fusion */
        uint16_t iload_src_ptr = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth,
                          "00> !Z> Z0> ZZ> 11> ?Z> Z1> ZZ>", &iload_src_ptr) &&
            validate_jump_target(get_var(opt, '0'), i,
                                 ILOAD_PATTERN_JUMP_OFFSET)) {
            /* ILOAD pattern matched. Save its destination address before the
             * next match_pattern call invalidates the optimizer version. */
            uint16_t iload_dst = get_var(opt, '1');

            uint16_t inc_src = 0, inc_dst = 0;

            /* Check for a subsequent INC pattern. */
            if (scan_depth >= INSN_INCR_LDINC &&
                match_pattern(vm, i + LDINC_INCREMENT_OFFSET, mem,
                              (int) (scan_depth - LDINC_INCREMENT_OFFSET),
                              "!!>", &inc_src, &inc_dst) &&
                inc_src != inc_dst && opt->neg1_reg[MASK_ADDR(inc_src)] &&
                inc_dst == iload_src_ptr) {
                /* Success: Fuse into LDINC */
                insn_mem[i].opcode = LDINC;
                insn_mem[i].dst = MASK_ADDR(iload_dst);
                insn_mem[i].src = MASK_ADDR(iload_src_ptr);
                opt->matches[LDINC]++;
                continue;
            }

            /* If not fused, fall back to a regular ILOAD */
            insn_mem[i].opcode = ILOAD;
            insn_mem[i].dst = MASK_ADDR(iload_dst);
            insn_mem[i].src = MASK_ADDR(iload_src_ptr);
            opt->matches[ILOAD]++;
            continue;
        }

        /* LSHIFT: Left shift by constant */
        uint16_t shift_count = 0;
        uint16_t shift_dst = 0;
        uint64_t shift_pos = 0;
        while (shift_pos < scan_depth) {
            uint16_t q0 = 0, q1 = 0;
            if (scan_depth - shift_pos < 9)
                break;
            if (match_pattern(vm, i + shift_pos, mem,
                              (int) (scan_depth - shift_pos), "!Z> Z!> ZZ>",
                              &q0, &q1) &&
                q0 == q1) {
                if (shift_count == 0)
                    shift_dst = q0;
                else if (shift_dst != q0)
                    break;
                shift_count++;
                shift_pos += 9;
            } else {
                break;
            }
        }
        if (shift_count >= 2) {
            insn_mem[i].opcode = LSHIFT;
            insn_mem[i].dst = MASK_ADDR(shift_dst);
            insn_mem[i].src = shift_count;
            opt->matches[LSHIFT]++;
            continue;
        }

        /* IADD: m[m[D]] += S */
        if (match_pattern(vm, i, mem, (int) scan_depth,
                          "01> 23> 44> 14> 3Z> 11> 33>")) {
            insn_mem[i].opcode = IADD;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '0'));
            insn_mem[i].src = MASK_ADDR(get_var(opt, '2'));
            opt->matches[IADD]++;
            continue;
        }

        /* INV: Bitwise NOT */
        uint16_t inv_temp = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth,
                          "00> 10> 11> 2Z> Z1> ZZ> !1>", &inv_temp) &&
            opt->one_reg[inv_temp]) {
            insn_mem[i].opcode = INV;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '1'));
            opt->matches[INV]++;
            continue;
        }

        /* ISUB: m[m[D]] -= S */
        if (match_pattern(vm, i, mem, (int) scan_depth,
                          "01> 33> 14> 5Z> 11>")) {
            insn_mem[i].opcode = ISUB;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '0'));
            insn_mem[i].src = MASK_ADDR(get_var(opt, '5'));
            opt->matches[ISUB]++;
            continue;
        }

        /* IJMP: PC = m[D] */
        uint16_t ijmp_temp = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "00> !Z> Z0> ZZ> ZZ>",
                          &ijmp_temp) &&
            validate_jump_target(get_var(opt, '0'), i,
                                 IJMP_PATTERN_JUMP_OFFSET)) {
            insn_mem[i].opcode = IJMP;
            insn_mem[i].dst = MASK_ADDR(ijmp_temp);
            opt->matches[IJMP]++;
            continue;
        }

        /* MOV: Copy data */
        uint16_t mov_src = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "00> !Z> Z0> ZZ>",
                          &mov_src)) {
            uint16_t dst = MASK_ADDR(get_var(opt, '0'));
            uint16_t src = MASK_ADDR(mov_src);
            if (dst != src) {
                insn_mem[i].opcode = MOV;
                insn_mem[i].dst = dst;
                insn_mem[i].src = src;
                opt->matches[MOV]++;
                continue;
            }
        }

        /* DOUBLE or ADD */
        uint16_t arith_src = 0, arith_dst = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "!Z> Z!> ZZ>",
                          &arith_src, &arith_dst)) {
            if (arith_src == arith_dst) {
                insn_mem[i].opcode = DOUBLE;
                insn_mem[i].dst = MASK_ADDR(arith_dst);
                insn_mem[i].src = MASK_ADDR(arith_src);
                opt->matches[DOUBLE]++;
            } else {
                insn_mem[i].opcode = ADD;
                insn_mem[i].dst = MASK_ADDR(arith_dst);
                insn_mem[i].src = MASK_ADDR(arith_src);
                opt->matches[ADD]++;
            }
            continue;
        }

        /* NEG: Two's complement negation (dst = 0 - src)
         * Pattern: SUBLEQ DST, DST, PC+3 (DST becomes 0)
         *          SUBLEQ SRC, DST, PC+6 (DST becomes 0 - SRC)
         * '0' is DST, '1' is SRC
         */
        if (match_pattern(vm, i, mem, (int) scan_depth, "00> 10>")) {
            insn_mem[i].opcode = NEG;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '0'));
            insn_mem[i].src = MASK_ADDR(get_var(opt, '1'));
            opt->matches[NEG]++;
            continue;
        }

        /* ZERO: Clear memory */
        if (match_pattern(vm, i, mem, (int) scan_depth, "00>")) {
            insn_mem[i].opcode = ZERO;
            insn_mem[i].dst = MASK_ADDR(get_var(opt, '0'));
            opt->matches[ZERO]++;
            continue;
        }

        /* HALT: Terminate */
        uint16_t halt_addr = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "ZZ!", &halt_addr) &&
            halt_addr == vm->mask) {
            insn_mem[i].opcode = HALT;
            opt->matches[HALT]++;
            continue;
        }

        /* JMP: Unconditional jump */
        uint16_t jmp_target = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "00!", &jmp_target)) {
            if (jmp_target == i) { /* Check for infinite loop */
                insn_mem[i].opcode = HALT;
                opt->matches[HALT]++;
            } else {
                insn_mem[i].opcode = JMP;
                insn_mem[i].dst = jmp_target;
                /* var '0' is the address being zeroed by the JMP sequence */
                insn_mem[i].src = MASK_ADDR(get_var(opt, '0'));
                opt->matches[JMP]++;
            }
            continue;
        }

        /* GET: Input character */
        uint16_t get_dst = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "N!>", &get_dst)) {
            insn_mem[i].opcode = GET;
            insn_mem[i].dst = MASK_ADDR(get_dst);
            opt->matches[GET]++;
            continue;
        }

        /* PUT: Output character */
        uint16_t put_src = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "!N>", &put_src)) {
            insn_mem[i].opcode = PUT;
            insn_mem[i].src = MASK_ADDR(put_src);
            opt->matches[PUT]++;
            continue;
        }

        /* INC/DEC/SUB */
        uint16_t sub_src = 0, sub_dst = 0;
        if (match_pattern(vm, i, mem, (int) scan_depth, "!!>", &sub_src,
                          &sub_dst) &&
            sub_src != sub_dst) {
            if (opt->neg1_reg[MASK_ADDR(sub_src)]) {
                insn_mem[i].opcode = INC;
                insn_mem[i].dst = MASK_ADDR(sub_dst);
                opt->matches[INC]++;
            } else if (opt->one_reg[MASK_ADDR(sub_src)]) {
                insn_mem[i].opcode = DEC;
                insn_mem[i].dst = MASK_ADDR(sub_dst);
                opt->matches[DEC]++;
            } else {
                insn_mem[i].opcode = SUB;
                insn_mem[i].dst = MASK_ADDR(sub_dst);
                insn_mem[i].src = MASK_ADDR(sub_src);
                opt->matches[SUB]++;
            }
            continue;
        }

        /* Default to SUBLEQ */
        insn_mem[i].opcode = SUBLEQ;
        insn_mem[i].src = mem[MASK_ADDR(i)];
        insn_mem[i].dst = mem[MASK_ADDR(i + 1)];
        insn_mem[i].aux = mem[MASK_ADDR(i + 2)];
        opt->matches[SUBLEQ]++;
    }
}

/* Generate hot spots analysis from PC heat map */
static void profiler_analyze_hot_spots(vm_t *vm)
{
    profiler_t *prof = &vm->prof;

    if (!prof->enabled || !prof->pc_heat_map)
        return;

    prof->hot_spot_count = 0;

    /* Find hot spots and sort by execution count */
    for (uint64_t pc = 0;
         pc < vm->mem_size && prof->hot_spot_count < MAX_HOT_SPOTS; pc++) {
        if (prof->pc_heat_map[pc] > 100) { /* Only significant hot spots */
            hot_spot_t spot = {
                .pc = pc,
                .exec_count = prof->pc_heat_map[pc],
                .opcode = vm->insn_mem[pc].opcode,
            };

            /* Insert in sorted order (simple insertion sort for small array) */
            size_t insert_pos = prof->hot_spot_count;
            for (size_t i = 0; i < prof->hot_spot_count; i++) {
                if (spot.exec_count > prof->hot_spots[i].exec_count) {
                    insert_pos = i;
                    break;
                }
            }

            /* Shift existing entries */
            for (size_t i = prof->hot_spot_count; i > insert_pos; i--) {
                if (i < MAX_HOT_SPOTS)
                    prof->hot_spots[i] = prof->hot_spots[i - 1];
            }

            /* Insert new hot spot */
            if (insert_pos < MAX_HOT_SPOTS) {
                prof->hot_spots[insert_pos] = spot;
                if (prof->hot_spot_count < MAX_HOT_SPOTS)
                    prof->hot_spot_count++;
            }
        }
    }
}

/* Report performance statistics */
static int report_stats(vm_t *vm)
{
    optimizer_t *opt = &vm->opt;
    profiler_t *prof = &vm->prof;
    double elapsed = (double) (opt->end - opt->start) / CLOCKS_PER_SEC;
    int64_t total_ops = 0, total_substitutions = 0;
    FILE *err = stderr;

    for (int i = 0; i < IMAX; i++) {
        total_ops += opt->exec_count[i];
        if (i != SUBLEQ)
            total_substitutions += opt->matches[i];
    }

    const char *div = "+--------+---------------+--------------+----------+\n";
    if (fputs(div, err) < 0)
        return -1;
    if (fprintf(err,
                "| Instr. | Substitutions | Instr. count | Instr. %% |\n") < 0)
        return -1;
    if (fputs(div, err) < 0)
        return -1;

    if (fprintf(err, "| SUBLEQ | %13d | %12" PRId64 " | %7.1f%% |\n",
                opt->matches[SUBLEQ], opt->exec_count[SUBLEQ],
                total_ops ? 100.0 * opt->exec_count[SUBLEQ] / total_ops : 0.0) <
        0)
        return -1;

    for (int i = 1; i < IMAX; i++) {
        if (opt->matches[i] == 0 && opt->exec_count[i] == 0)
            continue;
        if (fprintf(err, "| %-6s | %13d | %12" PRId64 " | %7.1f%% |\n",
                    insn_names[i], opt->matches[i], opt->exec_count[i],
                    total_ops ? 100.0 * opt->exec_count[i] / total_ops : 0.0) <
            0)
            return -1;
    }

    if (fputs(div, err) < 0)
        return -1;
    if (fprintf(err, "| Totals | %13" PRId64 " | %12" PRId64 " |          |\n",
                total_substitutions, total_ops) < 0)
        return -1;
    if (fputs(div, err) < 0)
        return -1;
    if (fprintf(err, "|         Execution time %.3f seconds             |\n",
                elapsed) < 0)
        return -1;
    if (fputs(div, err) < 0)
        return -1;

    /* Profiler report */
    if (vm->profiler_enabled && prof->enabled) {
        prof->end_time = clock();
        double prof_elapsed =
            (double) (prof->end_time - prof->start_time) / CLOCKS_PER_SEC;

        fprintf(err, "\n=== Lightweight Profiler Report ===\n");
        fprintf(err, "Total instructions executed: %" PRIu64 "\n",
                prof->total_instructions);
        fprintf(err, "Memory accesses: %" PRIu64 "\n", prof->memory_accesses);
        fprintf(err, "Instructions per second: %.0f\n",
                prof_elapsed > 0 ? prof->total_instructions / prof_elapsed : 0);

        if (prof->total_instructions > 0) {
            fprintf(err, "Memory accesses per instruction: %.2f\n",
                    (double) prof->memory_accesses / prof->total_instructions);
        }

        /* Hot spots analysis */
        profiler_analyze_hot_spots(vm);
        if (prof->hot_spot_count > 0) {
            fprintf(err, "\nTop %zu Hot Spots:\n",
                    prof->hot_spot_count > 10 ? 10 : prof->hot_spot_count);
            fprintf(err, "    PC   | Exec Count |   %%   | Opcode\n");
            fprintf(err, "---------|------------|-------|-------\n");

            for (size_t i = 0; i < prof->hot_spot_count && i < 10; i++) {
                hot_spot_t *spot = &prof->hot_spots[i];
                double percent =
                    prof->total_instructions > 0
                        ? 100.0 * spot->exec_count / prof->total_instructions
                        : 0;

                fprintf(err, " %6" PRIu64 "  | %10" PRIu64 " | %5.1f | %-6s\n",
                        spot->pc, spot->exec_count, percent,
                        spot->opcode < IMAX ? insn_names[spot->opcode] : "???");
            }
        }

        /* Export profiler data to file */
        FILE *prof_file = fopen("profiler_report.txt", "w");
        if (prof_file) {
            fprintf(prof_file, "SUBLEQ VM Lightweight Profiler Report\n");
            fprintf(prof_file, "=====================================\n");
            fprintf(prof_file, "Execution time: %.3f seconds\n", prof_elapsed);
            fprintf(prof_file, "Total instructions: %" PRIu64 "\n",
                    prof->total_instructions);
            fprintf(prof_file, "Memory accesses: %" PRIu64 "\n",
                    prof->memory_accesses);
            fprintf(
                prof_file, "Instructions per second: %.0f\n",
                prof_elapsed > 0 ? prof->total_instructions / prof_elapsed : 0);

            fprintf(prof_file, "\nInstruction Mix:\n");
            for (int i = 0; i < IMAX; i++) {
                if (opt->exec_count[i] > 0) {
                    fprintf(prof_file, "  %-8s: %12" PRId64 " (%6.2f%%)\n",
                            insn_names[i], opt->exec_count[i],
                            total_ops > 0
                                ? 100.0 * opt->exec_count[i] / total_ops
                                : 0);
                }
            }

            if (prof->hot_spot_count > 0) {
                fprintf(prof_file,
                        "\nHot Spots (PC addresses with highest execution "
                        "counts):\n");
                for (size_t i = 0; i < prof->hot_spot_count; i++) {
                    hot_spot_t *spot = &prof->hot_spots[i];
                    double percent = prof->total_instructions > 0
                                         ? 100.0 * spot->exec_count /
                                               prof->total_instructions
                                         : 0;

                    fprintf(prof_file,
                            "  PC %6" PRIu64 ": %10" PRIu64
                            " executions (%5.1f%%) [%s]\n",
                            spot->pc, spot->exec_count, percent,
                            spot->opcode < IMAX ? insn_names[spot->opcode]
                                                : "unknown");
                }
            }

            fclose(prof_file);
            fprintf(
                err,
                "\nDetailed profiler report saved to: profiler_report.txt\n");
        }
    }

    return 0;
}

/* Execute the virtual machine */
static int execute_vm(vm_t *vm)
{
    vm->opt.start = clock();
    /* Initial call to dispatch, passing NULL for the unused insn pointer. */
    dispatch(vm, vm->pc, NULL);
    vm->opt.end = clock();
    return vm->error;
}

typedef void (*handler_func_t)(vm_t *vm, uint64_t pc, const insn_t *insn);
/* The dispatch table, mapping opcodes to their handler functions */
static handler_func_t const dispatch_table[IMAX] = {
#define _(inst, inc) [inst] = handle_##inst,
    INSN_LIST
#undef _
};

/* Dispatch to instruction handlers. */
HOT_PATH static void dispatch(vm_t *vm, uint64_t pc, const insn_t *unused_insn)
{
    (void) unused_insn;

    if (UNLIKELY(pc >= vm->mem_size / 2 || vm->error))
        return;

    /* Read the instruction once, and pass a pointer to the handler. */
    const insn_t *insn = &vm->insn_mem[pc];
    uint8_t opcode = insn->opcode;
    vm->opt.exec_count[opcode]++;

    /* Use the dispatch table for a direct function call. The handler
     * will then tail-call back to this dispatch function, continuing the
     * execution cycle without growing the stack.
     */
    MUST_TAIL return dispatch_table[opcode](vm, pc, insn);
}

int main(int argc, char **argv)
{
    vm_t vm = {
        .in = stdin,
        .out = stdout,
        .nbits = 16,
        .mem_size = SZ,
        .stats_enabled = false,
        .optimize_enabled = true,
        .profiler_enabled = false,
    };
    vm.mask = MASK_BITS(vm.nbits);

    vm.mem = calloc(SZ, sizeof(uint16_t));
    if (!vm.mem) {
        fprintf(stderr, "Error: Failed to allocate main memory.\n");
        return 1;
    }

    vm.insn_mem = calloc(SZ, sizeof(insn_t));
    if (!vm.insn_mem) {
        fprintf(stderr, "Error: Failed to allocate instruction memory.\n");
        free(vm.mem);
        return 1;
    }

    const char *image_file = NULL;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-O")) /* Disable optimization */
            vm.optimize_enabled = false;
        else if (!strcmp(argv[i], "-s")) /* Enable statistics */
            vm.stats_enabled = true;
        else if (!strcmp(argv[i], "-p")) /* Enable lightweight profiler */
            vm.profiler_enabled = true;
        else if (!image_file) /* Image file path */
            image_file = argv[i];
        else
            fprintf(stderr, "Warning: Ignoring extra argument '%s'\n", argv[i]);
    }

    if (!image_file) {
        fprintf(stderr, "Usage: %s <subleq.dec> [-O] [-s] [-p]\n", argv[0]);
        fprintf(stderr, "  -O    Disable optimization\n");
        fprintf(stderr, "  -s    Enable statistics\n");
        fprintf(stderr, "  -p    Enable lightweight profiler\n");
        free(vm.mem);
        free(vm.insn_mem);
        return 1;
    }

    FILE *file = fopen(image_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Failed to open file '%s'\n", image_file);
        free(vm.mem);
        free(vm.insn_mem);
        return 1;
    }

    long val;
    uint64_t count = 0;
    char sep;
    while (fscanf(file, "%ld%c", &val, &sep) == 2) {
        if (val < SHRT_MIN || val > SHRT_MAX) {
            fprintf(stderr,
                    "Error: Value %ld at position %" PRIu64
                    " exceeds 16-bit signed limit\n",
                    val, count);
            fclose(file);
            free(vm.mem);
            free(vm.insn_mem);
            return 1;
        }

        vm.mem[MASK_ADDR(vm.load_size)] = (uint16_t) val;
        vm.load_size++;
        count++;

        if (sep != ',' && !isspace(sep) && sep != EOF) {
            fprintf(stderr,
                    "Error: Invalid format at position %" PRIu64
                    " (expected comma or whitespace, got '%c')\n",
                    count, sep);
            fclose(file);
            free(vm.mem);
            free(vm.insn_mem);
            return 1;
        }
        if (sep == EOF) /* End of file reached as separator */
            break;
    }

    if (ferror(file) && !feof(file)) {
        fprintf(stderr, "Error: Failed to read '%s'\n", image_file);
        fclose(file);
        free(vm.mem);
        free(vm.insn_mem);
        return 1;
    }
    if (fclose(file) < 0) {
        fprintf(stderr, "Error: Failed to close file '%s'\n", image_file);
        free(vm.mem);
        free(vm.insn_mem);
        return 2;
    }
    vm.max_addr = vm.load_size; /* Max address initialized to loaded size */

    /* Initialize profiler */
    profiler_init(&vm);

    if (vm.optimize_enabled) {
        optimize(&vm, vm.load_size);
    } else {
        fprintf(stderr,
                "Optimizations disabled. Running as basic interpreter.\n");
        for (uint64_t i = 0; i < vm.load_size; i++) {
            vm.insn_mem[MASK_ADDR(i)].opcode = SUBLEQ;
            vm.insn_mem[MASK_ADDR(i)].src = vm.mem[MASK_ADDR(i)];
            vm.insn_mem[MASK_ADDR(i)].dst = vm.mem[MASK_ADDR(i + 1)];
            vm.insn_mem[MASK_ADDR(i)].aux = vm.mem[MASK_ADDR(i + 2)];
        }
    }

    int status = execute_vm(&vm);
    if (vm.stats_enabled && report_stats(&vm) < 0)
        status = -1; /* Indicate error if stats reporting fails */

    /* Cleanup profiler */
    profiler_cleanup(&vm);

    free(vm.mem);
    free(vm.insn_mem);
    return status;
}
