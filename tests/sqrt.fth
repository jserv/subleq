.( example: square root )

decimal

\ This program finds the integer square root of a number n by starting with
\ a root of 1 and systematically stepping up. It calculates the square of each
\ successive integer (1, 2, 3, ...) and stops as soon as the calculated square
\ exceeds the input number n. The integer root that produced the last valid
\ square (the one not greater than n) is the result.

: sqrt ( n -- root )
    dup 2 < if exit then        \ Handles 0 and 1 directly.
    >r                          \ Stash n on the return stack for comparison.

    \ Initialize the stack with the first candidate:
    \ root = 1, square_of_root = 1
    1 1                         \ Stack: ( square root )

    begin
        \ Check if the current square is still less than n.
        over r@ <               \ Is current_square < n?
    while
        \ If so, calculate the next root and its corresponding square.
        \ Stack at start of while: ( current_square current_root )
        dup 2 * 1 +             \ Calculate 2*root + 1
        rot +                   \ Add to current_square to get the
                                \ next_square (i.e. root^2 + 2*root + 1)
        swap 1 +                \ Increment root to get next_root
    repeat

    \ The loop has ended. The root on the stack is our result.
    swap drop                   \ Drop the final square, keeping the root.
    r> drop                     \ Clean up n from the return stack.
;

.( to test the routines, type: )
.( 16    sqrt . => ) 16    sqrt .
.( 625   sqrt . => ) 625   sqrt .
.( 2401  sqrt . => ) 2401  sqrt .
