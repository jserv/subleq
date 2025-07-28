.( example: Conway's Game of Life )

' ( <ok> ! ( disable ok prompt [non-portable] )

\ Conway's Game of Life - 5-Generation Glider Demonstration
\ Shows complete glider movement cycle with proper visualization

\ 5x5 grid variables
variable c00 variable c01 variable c02 variable c03 variable c04
variable c10 variable c11 variable c12 variable c13 variable c14
variable c20 variable c21 variable c22 variable c23 variable c24
variable c30 variable c31 variable c32 variable c33 variable c34
variable c40 variable c41 variable c42 variable c43 variable c44

\ grid management
: clear ( -- )
    0 c00 ! 0 c01 ! 0 c02 ! 0 c03 ! 0 c04 !
    0 c10 ! 0 c11 ! 0 c12 ! 0 c13 ! 0 c14 !
    0 c20 ! 0 c21 ! 0 c22 ! 0 c23 ! 0 c24 !
    0 c30 ! 0 c31 ! 0 c32 ! 0 c33 ! 0 c34 !
    0 c40 ! 0 c41 ! 0 c42 ! 0 c43 ! 0 c44 ! ;

\ display system
: show ( -- )
    cr 43 emit 45 emit 45 emit 45 emit 45 emit 45 emit 43 emit cr
    124 emit
    c00 @ if 42 emit else 46 emit then
    c01 @ if 42 emit else 46 emit then  
    c02 @ if 42 emit else 46 emit then
    c03 @ if 42 emit else 46 emit then
    c04 @ if 42 emit else 46 emit then 124 emit cr
    124 emit
    c10 @ if 42 emit else 46 emit then
    c11 @ if 42 emit else 46 emit then
    c12 @ if 42 emit else 46 emit then
    c13 @ if 42 emit else 46 emit then
    c14 @ if 42 emit else 46 emit then 124 emit cr
    124 emit
    c20 @ if 42 emit else 46 emit then
    c21 @ if 42 emit else 46 emit then
    c22 @ if 42 emit else 46 emit then
    c23 @ if 42 emit else 46 emit then
    c24 @ if 42 emit else 46 emit then 124 emit cr
    124 emit
    c30 @ if 42 emit else 46 emit then
    c31 @ if 42 emit else 46 emit then
    c32 @ if 42 emit else 46 emit then
    c33 @ if 42 emit else 46 emit then
    c34 @ if 42 emit else 46 emit then 124 emit cr
    124 emit
    c40 @ if 42 emit else 46 emit then
    c41 @ if 42 emit else 46 emit then
    c42 @ if 42 emit else 46 emit then
    c43 @ if 42 emit else 46 emit then
    c44 @ if 42 emit else 46 emit then 124 emit cr
    43 emit 45 emit 45 emit 45 emit 45 emit 45 emit 43 emit cr ;

\ glider patterns
: glider0 ( -- )    \ initial glider pattern
    clear
    1 c01 ! 1 c12 ! 1 c20 ! 1 c21 ! 1 c22 ! ;

: glider1 ( -- )    \ first transformation
    clear
    1 c02 ! 1 c10 ! 1 c11 ! 1 c20 ! ;

: glider2 ( -- )    \ vertical line formation
    clear
    1 c01 ! 1 c11 ! 1 c21 ! ;

: glider3 ( -- )    \ third transformation
    clear
    1 c12 ! 1 c20 ! 1 c22 ! ;

: glider4 ( -- )    \ moved position (cycle complete)
    clear
    1 c11 ! 1 c22 ! 1 c30 ! 1 c31 ! 1 c32 ! ;

\ extended glider demonstration

cr .( Conway's Game of Life - Glider )
cr .( The glider moves diagonally every 4 steps )
cr

cr .( Generation 0 - Initial Pattern:)
glider0 show

cr .( Generation 1 - First Transform:)
glider1 show  

cr .( Generation 2 - Vertical Line:)
glider2 show

cr .( Generation 3 - Moved Position:)
glider3 show

cr .( Generation 4 - Cycle Complete:)
glider4 show
