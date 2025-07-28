.( example: Mandelbrot set )

' ( <ok> ! ( disable ok prompt [non-portable] )

\ This program renders the colorized Mandelbrot set to a text console. It uses
\ 4-bit fixed-point arithmetic to simulate fractional numbers on an
\ integer-only system. The core algorithm iterates the formula z = z^2 + c
\ for each point on a 2D grid and applies ANSI colors based on iteration count.
\
\ Colors represent different escape speeds:
\ - Black: Interior points (never escape)
\ - Blue: Very fast escape (1-4 iterations)
\ - Cyan: Fast escape (5-9 iterations)
\ - Green: Medium escape (10-14 iterations)
\ - Yellow: Slower escape (15-24 iterations)
\ - Magenta: Slow escape (25-34 iterations)
\ - Red: Slowest escape (35+ iterations)

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

\ ANSI color support
: esc[   27 emit  91 emit ;

\ color words for Mandelbrot visualization
: black   esc[  51 emit 48 emit  109 emit ;   \ ESC[30m
: red     esc[  51 emit 49 emit  109 emit ;   \ ESC[31m
: green   esc[  51 emit 50 emit  109 emit ;   \ ESC[32m
: yellow  esc[  51 emit 51 emit  109 emit ;   \ ESC[33m
: blue    esc[  51 emit 52 emit  109 emit ;   \ ESC[34m
: magenta esc[  51 emit 53 emit  109 emit ;   \ ESC[35m
: cyan    esc[  51 emit 54 emit  109 emit ;   \ ESC[36m
: white   esc[  51 emit 55 emit  109 emit ;   \ ESC[37m
: reset   esc[  48 emit        109 emit ;     \ ESC[0m

\ A colorized palette for mapping iteration counts to display characters.
create palette
    char . c, char - c, char - c, char = c, char o c,
    char + c, char * c, char # c, char % c, bl c,

\ --- Variables for Main Loop Control ---

variable x                  \ Stores the current x-coordinate (real part)
variable y                  \ Stores the current y-coordinate (imaginary part)
variable ci                 \ Caches the current y-coord for the inner loop

\ --- Core Words ---

\ colorized output based on iteration count
: color-for-iter ( iter -- )
    dup 0 = if drop black exit then      \ interior: black
    dup 5 < if drop blue exit then       \ very fast escape: blue
    dup 10 < if drop cyan exit then      \ fast escape: cyan
    dup 15 < if drop green exit then     \ medium escape: green
    dup 25 < if drop yellow exit then    \ slower escape: yellow
    dup 35 < if drop magenta exit then   \ slow escape: magenta
    drop red ;                           \ slowest escape: red

: output ( iter -- )        \ Colorized output for Mandelbrot visualization.
    dup color-for-iter       \ Set color based on iteration count
    5 / palette + c@ emit    \ Emit character from palette
    reset ;                  \ Reset color after each character

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
