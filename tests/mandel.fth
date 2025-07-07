.( example: Mandelbrot set using 4-bit fixed point )

16 constant scale              \ scale = 2^4, so 1.0 = 16
64 constant thresh  48 constant maxiter

\ Complex plane bounds (scaled)
-32 constant xmin   16 constant xmax   1 constant xstep
-20 constant ymin   20 constant ymax   1 constant ystep

\ Character palette for iteration display
create palette
    char . c, char - c, char - c, char = c, char o c,
    char + c, char * c, char # c, char % c, bl c,

: output ( iter -- ) 5 / palette + c@ emit ;
: muls ( a b -- ab/scale ) * scale / ;

: c2 ( ar ai -- ar' ai' )     \ Complex square: z² = (ar+ai*i)²
    2dup muls 2* >r           \ ai' = 2*ar*ai/scale
    dup muls >r               \ save ai²/scale
    dup muls r> -             \ ar' = ar²/scale - ai²/scale
    r> ;

: abs2 ( ar ai -- |z|²/scale ) dup muls swap dup muls + ;

: iter ( cr ci n zr zi -- cr ci n' zr' zi' )
    c2 >r >r 1+               \ increment iteration count
    r> 3 pick +               \ zr' = zr² - zi² + cr
    r> 3 pick + ;             \ zi' = 2*zr*zi + ci

: point ( cr ci -- n )        \ Calculate escape iterations for point
    0 0 0                     \ n zr zi
    begin
        iter 2dup abs2 thresh >   \ exceeded escape radius?
        >r 2 pick maxiter > r> or \ or hit max iterations?
    until
    drop drop rot rot drop drop ;

variable x variable y variable ci

: mandel
    ymin y !
    begin y @ ymax > 0= while
        y @ ci !  xmin x !
        begin x @ xmax > 0= while
            x @ ci @ point output
            x @ xstep + x !
        repeat
        cr  y @ ystep + y !
    repeat ;

mandel
