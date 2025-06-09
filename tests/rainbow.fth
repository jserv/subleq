\ ---------- ANSI color ----------

\ helper: send ESC (27) then '[' (91)
: esc[   27 emit  91 emit ;

\ colour words (foreground 30–37) – all bytes emitted literally
: black   esc[  51 emit 48 emit  109 emit ;   \ ESC[30m
: red     esc[  51 emit 49 emit  109 emit ;   \ ESC[31m
: green   esc[  51 emit 50 emit  109 emit ;   \ ESC[32m
: yellow  esc[  51 emit 51 emit  109 emit ;   \ ESC[33m
: blue    esc[  51 emit 52 emit  109 emit ;   \ ESC[34m
: magenta esc[  51 emit 53 emit  109 emit ;   \ ESC[35m
: cyan    esc[  51 emit 54 emit  109 emit ;   \ ESC[36m
: white   esc[  51 emit 55 emit  109 emit ;   \ ESC[37m
: reset   esc[  48 emit        109 emit ;     \ ESC[0m

\ print "rainbow" with seven different colours
: rainbow
  red     82 emit          \ "R"
  yellow  97 emit          \ "a"
  green  105 emit          \ "i"
  cyan   110 emit          \ "n"
  blue   98  emit          \ "b"
  magenta 111 emit         \ "o"
  white  119 emit          \ "w"
  reset  cr ;

\ ------------- run once -------------
rainbow
