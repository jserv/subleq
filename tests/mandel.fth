\ ------------ Mandelbrot set (4-bit fixed) ------------

\ scale = 2^4  → 1.0 = 16
16   constant scale
64   constant thresh           \ 4 * scale
48   constant maxiter

\ x range −2.0 .. +1.0  (scaled)
-32  constant xmin
 16  constant xmax
  1  constant xstep            \ 0.0625

\ y range −1.25 .. +1.25  (scaled)
-20  constant ymin
 20  constant ymax
  1  constant ystep            \ 0.0625

\ palette ".--=o+*#% "  (10 chars, last is space)
create palette
  char . c,  char - c,  char - c,  char = c,
  char o c,  char + c,  char * c,  char # c,
  char % c,  bl c,

: output   ( iter -- )  5 /  palette + c@ emit ;

: muls     ( a b -- ab/scale )   *  scale / ;

: c2       ( ar ai -- ar' ai' )           \ z squared
  2dup muls  2* >r          \ imag' = 2*ar*ai/scale
  dup muls >r               \ save ai^2/scale
  dup muls                  \ ar^2/scale
  r>  -                     \ real' = ar^2 − ai^2
  r> ;                      \ leave ar' ai'

: abs2     ( ar ai -- |z|^2/scale )   dup muls  swap  dup muls  + ;

\ cr ci n zr zi → cr ci n' zr'' zi''
: iter
  c2
  >r >r                      \ push zi' zr'
  1+                         \ n'
  r>  3 pick +               \ zr'' = zr' + cr
  r>  3 pick + ;             \ zi'' = zi' + ci

: point   ( cr ci -- n )
  0 0 0
  begin
    iter
    2dup abs2  thresh >      \ escaped?
    >r
    2 pick maxiter >         \ hit cap?
    r> or
  until
  drop drop  rot rot  drop drop ;

variable x
variable y
variable ci

: mandel
  ymin y !
  begin
    y @  ymax > 0=           \ rows
  while
    y @ ci !
    xmin x !
    begin
      x @  xmax > 0=         \ cols
    while
      x @  ci @  point  output
      x @  xstep +  x !
    repeat
    cr
    y @  ystep +  y !
  repeat ;

\ -------------------- draw --------------------
mandel
