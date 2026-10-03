#!/usr/bin/env bash
#
# Sloth in bash -- a self-contained ANS Forth engine.
#
# Goal: Sloth available with no installation, using only bash, for bootstrapping
# a system. The native kernel mirrors platforms/js (32-bit signed cells, byte
# addressable memory, xt < 0 selects a primitive, xt > 0 is a colon body). Most
# of Sloth is Forth (4th/ans.4th), loaded unchanged via INCLUDED.
#
# Performance: bash command substitution (~450us here) dominates, so helpers
# write to globals instead of echoing and arithmetic is inlined in the hot
# primitives. See platforms/bash/README.md.
#
# Reference: platforms/js/src/sloth.js. See proj.sloth.org.

export LC_ALL=C

sCELL=4
CBUF=64
DATA_SIZE=$((1 << 20))
STACK_SIZE=64
RETURN_STACK_SIZE=64
STACK_OVERFLOW=-3
STACK_UNDERFLOW=-4
RETURN_STACK_OVERFLOW=-5
RETURN_STACK_UNDERFLOW=-6

USER_BASE=$((1 << 24))
SCRATCH=$((1 << 25))
LINEBUF=$((1 << 26))

UV_CURRENT=0
UV_ORDER=4
UV_LOCALS_WORDLIST=8
UV_CONTEXT=12
UV_BASE=76
UV_STATE=80
UV_IBUF=84
UV_IPOS=88
UV_ILEN=92
UV_SOURCE_ID=96
UV_SOURCE_POS=100
UV_LATESTXT=104
UV_INTERPRET=108
UV_ROOT_PATH_LENGTH=112
UV_PATH_START=116
UV_PATH_END=120
UV_PATHS=124
UV_INCLUDED_FILES=380
UV_PRECISION=384

FORTH_WL=8
INTERNAL_WL=4
HIDDEN=1
IMMEDIATE=2

declare -a MEM S R PRIM BYTE2CHAR F FBYTE
declare -A HASH PXT CHAR2BYTE
declare -a FILEFD SRC_STACK FHFD FHPOS
declare -A FHPATH ALLOCSZ
fp=0
FCELL_SIZE=8
FLOAT_STACK_SIZE=64
FLOAT_STACK_OVERFLOW=-44
FLOAT_STACK_UNDERFLOW=-45
FRES=""
FDEC=""
FBYTES=""
FPINT=""
FVAL=""
FIDN=0
HEAP_PTR=$((1 << 27))
IOR_FILE=-37
IOR_ALLOC=-59
IOR_RESIZE=-61
sp=0
rp=0
ip=-1
HERE=12
THR=0
PN=0
LASTXT=0
NEXTFID=0
NON_TTY=1
KEY_ENTER=10
SLOTH_PATHS=()

# Out-parameters for the hot helpers (see performance note above).
CF=0 FCH=0 UGET=0 GXT=0 GLAT=0 ORDV=0 PINT=0 FWORD=0 SWORD=0 XCT=0
MTS=""

# --- Memory ------------------------------------------------------------

fetch() { FCH=${MEM[$(( $1 >> 2 ))]:-0}; }

store() {
  local nv=$(( $2 & 0xFFFFFFFF ))
  (( nv >= 0x80000000 )) && (( nv -= 0x100000000 ))
  MEM[$(( $1 >> 2 ))]=$nv
}

cf() {
  CF=$(( ( ${MEM[$(( $1 >> 2 ))]:-0} >> ( ($1 & 3) * 8 ) ) & 0xFF ))
}

cs() {
  local v=$(( $2 & 0xFF )) w=$(( $1 >> 2 )) sh=$(( ($1 & 3) * 8 ))
  local old=${MEM[$w]:-0} nv
  nv=$(( (old & ~(0xFF << sh)) | (v << sh) ))
  (( nv &= 0xFFFFFFFF ))
  (( nv >= 0x80000000 )) && (( nv -= 0x100000000 ))
  MEM[$w]=$nv
}

user_get() { fetch $(( USER_BASE + $1 )); UGET=$FCH; }
user_set() { store $(( USER_BASE + $1 )) "$2"; }

build_char_tables() {
  local i oct c
  for ((i = 0; i < 256; i++)); do
    printf -v oct '%03o' "$i"
    printf -v c "\\$oct"
    BYTE2CHAR[i]=$c
    if [ -n "$c" ]; then CHAR2BYTE["$c"]=$i; fi
  done
}

ord() { ORDV=${CHAR2BYTE[$1]:-0}; }

mem_to_string() {
  local a=$1 l=$2 i
  MTS=""
  for ((i = 0; i < l; i++)); do
    cf $(( a + i ))
    MTS+=${BYTE2CHAR[$CF]}
  done
}

push() { S[sp]=$1; ((sp++)); }

check_data() { # n r : need n items, leave r -> throws on under/overflow
  if (( sp < $1 )); then THR=$STACK_UNDERFLOW; return 1; fi
  if (( sp - $1 + $2 > STACK_SIZE )); then THR=$STACK_OVERFLOW; return 1; fi
  return 0
}

# --- Dictionary --------------------------------------------------------

align() { HERE=$(( (HERE + 3) & ~3 )); }
comma() { store "$HERE" "$1"; ((HERE += sCELL)); }
c_comma() { cs "$HERE" "$1"; ((HERE += 1)); }
compile() { comma "$1"; }

get_latest() { user_get "$UV_CURRENT"; fetch "$UGET"; GLAT=$FCH; }
set_latest() { user_get "$UV_CURRENT"; store "$UGET" "$1"; }
get_xt() { GXT=${MEM[$(( ($1 + 4) >> 2 ))]:-0}; }
set_xt() { store $(( $1 + 4 )) "$2"; }
set_flag() { cf $(( $1 + 8 )); cs $(( $1 + 8 )) $(( CF | $2 )); }
unset_flag() { cf $(( $1 + 8 )); cs $(( $1 + 8 )) $(( CF & ~ $2 )); }
has_flag() { cf $(( $1 + 8 )); (( (CF & $2) == $2 )); }

header_string() {
  local name=$1
  local l=${#name} i w
  align
  w=$HERE
  get_latest; comma "$GLAT"
  set_latest "$w"
  comma 0
  c_comma 0
  c_comma "$l"
  for ((i = 0; i < l; i++)); do
    ord "${name:i:1}"
    c_comma "$ORDV"
  done
  align
  store $(( w + 4 )) "$HERE"
  user_get "$UV_CURRENT"
  HASH["$UGET|${name^^}"]=$w
}

xt_of() { XCT=${PXT[$1]}; }
literal() { xt_of '(LIT)'; compile "$XCT"; comma "$1"; }

mem_eq_ci() {
  local a1=$1 a2=$2 l=$3 i x y
  for ((i = 0; i < l; i++)); do
    cf $(( a1 + i )); x=$CF
    cf $(( a2 + i )); y=$CF
    (( x >= 97 && x <= 122 )) && (( x -= 32 ))
    (( y >= 97 && y <= 122 )) && (( y -= 32 ))
    (( x == y )) || return 1
  done
  return 0
}

search_word() {
  local n=$1 l=$2 i key="" order idx wl w
  for ((i = 0; i < l; i++)); do
    cf $(( n + i ))
    key+=${BYTE2CHAR[$CF]}
  done
  key=${key^^}
  user_get "$UV_ORDER"; order=$UGET
  for ((idx = -1; idx < order; idx++)); do
    user_get $(( UV_CONTEXT + idx * sCELL )); wl=$UGET
    (( wl != 0 )) || continue
    w=${HASH["$wl|$key"]:-0}
    (( w != 0 )) || continue
    if ! has_flag "$w" "$HIDDEN"; then
      SWORD=$w
      return
    fi
    fetch "$wl"; w=$FCH
    while (( w > 0 )); do
      if ! has_flag "$w" "$HIDDEN"; then
        cf $(( w + 9 ))
        if [ "$CF" = "$l" ] && mem_eq_ci $(( w + 10 )) "$n" "$l"; then
          SWORD=$w
          return
        fi
      fi
      fetch "$w"; w=$FCH
    done
  done
  SWORD=0
}

find_word() {
  user_get "$UV_CURRENT"
  FWORD=${HASH["$UGET|${1^^}"]:-0}
}

parse_int() {
  local s=$1 base=$2
  local i=0 neg=0 n=${#s} c o v=0
  (( n > 0 )) || return 1
  c=${s:0:1}
  if [ "$c" = "+" ] || [ "$c" = "-" ]; then
    [ "$c" = "-" ] && neg=1
    i=1
  fi
  (( i < n )) || return 1
  for ((; i < n; i++)); do
    c=${s:i:1}
    ord "$c"; o=$ORDV
    if (( o >= 48 && o <= 57 )); then o=$(( o - 48 ))
    elif (( o >= 65 && o <= 90 )); then o=$(( o - 55 ))
    elif (( o >= 97 && o <= 122 )); then o=$(( o - 87 ))
    else return 1; fi
    (( o < base )) || return 1
    v=$(( v * base + o ))
  done
  if (( neg )); then PINT=$(( -v )); else PINT=$v; fi
  return 0
}

# --- Inner interpreter -------------------------------------------------

inner() {
  local t=$rp xtv q
  while (( THR == 0 && t <= rp && ip >= 0 )); do
    xtv=${MEM[$(( ip >> 2 ))]:-0}
    ((ip += sCELL))
    if (( xtv < 0 )); then
      q=$(( -1 - xtv ))
      "${PRIM[$q]}"
    else
      if (( ip >= 0 || rp > 0 )); then R[rp]=$ip; ((rp++)); fi
      ip=$xtv
    fi
  done
}

eval_word() {
  local q=$1
  if (( q < 0 )); then
    q=$(( -1 - q ))
    "${PRIM[$q]}"
  else
    if (( ip >= 0 || rp > 0 )); then R[rp]=$ip; ((rp++)); fi
    ip=$q
    inner
  fi
}

catch_xt() {
  local q=$1 tsp=$sp trp=$rp tip=$ip
  THR=0
  eval_word "$q"
  if (( THR != 0 )); then
    local v=$THR
    THR=0
    sp=$tsp; rp=$trp; ip=$tip
    push "$v"
  else
    push 0
  fi
}

# --- Registration ------------------------------------------------------

add_prim() { ((PN++)); PRIM[$PN]=$1; LASTXT=$(( -1 - PN )); }
code() { add_prim "$2"; PXT[$1]=$LASTXT; header_string "$1"; get_latest; set_xt "$GLAT" "$LASTXT"; }

user_variable() {
  header_string "$1"
  local w
  get_latest; w=$GLAT
  set_xt "$w" "$HERE"
  literal $(( USER_BASE + $2 ))
  xt_of EXIT; compile "$XCT"
  store $(( USER_BASE + $2 )) "$3"
}

# --- Primitives: control ----------------------------------------------

p_exit() { if (( rp > 0 )); then ((rp--)); ip=${R[rp]}; else ip=-1; fi; }
p_lit() { push "${MEM[$(( ip >> 2 ))]:-0}"; ((ip += sCELL)); }
p_rip() { local tip=$ip; local o=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL)); push $(( tip + o - sCELL )); }
p_branch() { local a=$ip; local o=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL)); ip=$(( a + o )); }
p_zbranch() {
  check_data 1 0 || return 0
  local a=$ip f=${S[sp-1]}; ((sp--))
  if (( f == 0 )); then
    local o=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL)); ip=$(( a + o ))
  else
    ((ip += sCELL))
  fi
}
p_string() {
  local l=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL))
  push "$ip"; push "$l"
  ip=$(( (ip + l + 1 + 3) & ~3 ))
}
p_c_string() {
  cf "$ip"
  local l=$CF
  push "$ip"
  ip=$(( (ip + l + 2 + 3) & ~3 ))
}
p_quotation() { local d=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL)); push "$ip"; ip=$(( ip + d )); }

# --- Primitives: stack -------------------------------------------------

p_drop() { check_data 1 0 || return 0; ((sp--)); }
p_dup() { check_data 1 2 || return 0; push "${S[sp-1]}"; }
p_over() { check_data 2 3 || return 0; push "${S[sp-2]}"; }
p_to_r() {
  check_data 1 0 || return 0
  if (( rp >= RETURN_STACK_SIZE )); then THR=$RETURN_STACK_OVERFLOW; return 0; fi
  ((sp--)); R[rp]=${S[sp]}; ((rp++))
}
p_r_from() {
  if (( rp < 1 )); then THR=$RETURN_STACK_UNDERFLOW; return 0; fi
  check_data 0 1 || return 0
  ((rp--)); push "${R[rp]}"
}
p_swap() { check_data 2 2 || return 0; local t=${S[sp-1]}; S[sp-1]=${S[sp-2]}; S[sp-2]=$t; }

# --- Primitives: memory ------------------------------------------------

p_c_fetch() {
  check_data 1 1 || return 0
  local a=${S[sp-1]}
  S[sp-1]=$(( ( ${MEM[$(( a >> 2 ))]:-0} >> ( (a & 3) * 8 ) ) & 0xFF ))
}
p_c_store() { check_data 2 0 || return 0; local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2)); cs "$a" "$v"; }
p_fetch() { check_data 1 1 || return 0; local a=${S[sp-1]}; S[sp-1]=${MEM[$(( a >> 2 ))]:-0}; }
p_store() { check_data 2 0 || return 0; local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2)); store "$a" "$v"; }
p_cells() {
  check_data 1 1 || return 0
  local r=$(( ${S[sp-1]} * sCELL ))
  (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_chars() { check_data 1 1 || return 0; :; }
p_here() { check_data 0 1 || return 0; push "$HERE"; }
p_align() { align; }
p_allot() { check_data 1 0 || return 0; ((sp--)); HERE=$(( HERE + ${S[sp]} )); }
p_unused() { check_data 0 1 || return 0; push $(( DATA_SIZE - HERE )); }

# --- Primitives: arithmetic and logic ----------------------------------

p_invert() {
  check_data 1 1 || return 0
  local r=$(( ~ ${S[sp-1]} )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_and() {
  check_data 2 1 || return 0
  local v=${S[sp-1]}; ((sp--))
  local r=$(( ${S[sp-1]} & v )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_l_shift() {
  check_data 2 1 || return 0
  local n=${S[sp-1]}; ((sp--))
  local r=$(( ${S[sp-1]} << n )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_minus() {
  check_data 2 1 || return 0
  local v=${S[sp-1]}; ((sp--))
  local r=$(( ${S[sp-1]} - v )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_plus() {
  check_data 2 1 || return 0
  local v=${S[sp-1]}; ((sp--))
  local r=$(( ${S[sp-1]} + v )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_r_shift() {
  check_data 2 1 || return 0
  local n=${S[sp-1]}; ((sp--))
  local r=$(( (${S[sp-1]} & 0xFFFFFFFF) >> n )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_star() {
  check_data 2 1 || return 0
  local v=${S[sp-1]}; ((sp--))
  local r=$(( ${S[sp-1]} * v )); (( r &= 0xFFFFFFFF )); (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_two_slash() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} >> 1 )); }
p_u_m_star() {
  check_data 2 2 || return 0
  local b=$(( ${S[sp-1]} & 0xFFFFFFFF )) a=$(( ${S[sp-2]} & 0xFFFFFFFF )); ((sp-=2))
  local p=$(( a * b ))
  local lo=$(( p & 0xFFFFFFFF ))
  local hi=$(( (p >> 32) & 0xFFFFFFFF ))
  (( lo >= 0x80000000 )) && (( lo -= 0x100000000 ))
  (( hi >= 0x80000000 )) && (( hi -= 0x100000000 ))
  push "$lo"; push "$hi"
}
p_u_m_slash_mod() {
  check_data 3 2 || return 0
  local u=$(( ${S[sp-1]} & 0xFFFFFFFF )) hi=${S[sp-2]} lo=${S[sp-3]}; ((sp-=3))
  if (( u == 0 )); then THR=-10; return 0; fi
  local d=$(( ((hi & 0xFFFFFFFF) << 32) | (lo & 0xFFFFFFFF) ))
  local rem=$(( d % u )) quo=$(( d / u ))
  (( rem >= 0x80000000 )) && (( rem -= 0x100000000 ))
  (( quo >= 0x80000000 )) && (( quo -= 0x100000000 ))
  push "$rem"; push "$quo"
}
p_equals() { check_data 2 1 || return 0; local v=${S[sp-1]}; ((sp--)); if (( ${S[sp-1]} == v )); then S[sp-1]=-1; else S[sp-1]=0; fi; }
p_less() { check_data 2 1 || return 0; local v=${S[sp-1]}; ((sp--)); if (( ${S[sp-1]} < v )); then S[sp-1]=-1; else S[sp-1]=0; fi; }

# --- Primitives: strings -----------------------------------------------

p_move() {
  check_data 3 0 || return 0
  local u=${S[sp-1]} a2=${S[sp-2]} a1=${S[sp-3]}; ((sp-=3))
  local i
  if (( a1 >= a2 )); then
    for ((i = 0; i < u; i++)); do cf $(( a1 + i )); cs $(( a2 + i )) "$CF"; done
  else
    for ((i = u - 1; i >= 0; i--)); do cf $(( a1 + i )); cs $(( a2 + i )) "$CF"; done
  fi
}

# --- Primitives: input/output and parsing ------------------------------

p_emit() {
  check_data 1 0 || return 0
  ((sp--))
  local oct
  printf -v oct '%03o' $(( ${S[sp]} & 255 ))
  printf "\\$oct"
}
p_key() {
  if (( NON_TTY )); then push "$KEY_ENTER"; else local ch; IFS= read -rsn1 ch; ord "$ch"; push "$ORDV"; fi
}
p_source() { user_get "$UV_IBUF"; push "$UGET"; user_get "$UV_ILEN"; push "$UGET"; }

p_word() {
  check_data 1 1 || return 0
  local c=${S[sp-1]}; ((sp--))
  local ibuf ilen ipos start len i
  user_get "$UV_IBUF"; ibuf=$UGET
  user_get "$UV_ILEN"; ilen=$UGET
  user_get "$UV_IPOS"; ipos=$UGET
  if (( c == 32 )); then
    while (( ipos < ilen )); do cf $(( ibuf + ipos )); (( CF <= c )) || break; ((ipos++)); done
  else
    while (( ipos < ilen )); do cf $(( ibuf + ipos )); (( CF == c )) || break; ((ipos++)); done
  fi
  start=$(( ibuf + ipos ))
  if (( c == 32 )); then
    while (( ipos < ilen )); do cf $(( ibuf + ipos )); (( CF > c )) || break; ((ipos++)); done
  else
    while (( ipos < ilen )); do cf $(( ibuf + ipos )); (( CF != c )) || break; ((ipos++)); done
  fi
  len=$(( ibuf + ipos - start ))
  if (( len > 0xFFFF )); then THR=-18; return 0; fi
  cs $(( HERE + CBUF )) "$len"
  for ((i = 0; i < len; i++)); do cf $(( start + i )); cs $(( HERE + CBUF + 1 + i )) "$CF"; done
  push $(( HERE + CBUF ))
  if (( ipos < ilen )); then ((ipos++)); fi
  user_set "$UV_IPOS" "$ipos"
}

p_refill() {
  user_get "$UV_SOURCE_ID"; local sid=$UGET
  if (( sid == -1 )); then push 0; return; fi
  local line
  if (( sid <= 0 )); then
    if ! IFS= read -r line; then
      if [ -z "$line" ]; then push 0; return; fi
    fi
  else
    local fd=${FILEFD[$sid]}
    if ! IFS= read -r -u "$fd" line; then
      if [ -z "$line" ]; then push 0; return; fi
    fi
  fi
  user_get "$UV_IBUF"; local ibuf=$UGET
  local i
  for ((i = 0; i < ${#line}; i++)); do ord "${line:i:1}"; cs $(( ibuf + i )) "$ORDV"; done
  user_set "$UV_ILEN" "${#line}"
  user_set "$UV_IPOS" 0
  push -1
}

p_save_input() {
  check_data 0 6 || return 0
  user_get "$UV_SOURCE_POS"; push "$UGET"
  user_get "$UV_SOURCE_ID"; push "$UGET"
  user_get "$UV_IBUF"; push "$UGET"
  user_get "$UV_IPOS"; push "$UGET"
  user_get "$UV_ILEN"; push "$UGET"
  push 5
}
p_restore_input() {
  check_data 6 1 || return 0
  ((sp--))
  user_set "$UV_ILEN" "${S[sp-1]}"; ((sp--))
  user_set "$UV_IPOS" "${S[sp-1]}"; ((sp--))
  user_set "$UV_IBUF" "${S[sp-1]}"; ((sp--))
  user_set "$UV_SOURCE_ID" "${S[sp-1]}"; ((sp--))
  user_set "$UV_SOURCE_POS" "${S[sp-1]}"; ((sp--))
  push 0
}
p_file_position() {
  check_data 1 3 || return 0
  local h=${S[sp-1]}
  if [ -z "${FHFD[$h]:-}" ]; then ((sp--)); push 0; push 0; push $IOR_FILE; return 0; fi
  local p=${FHPOS[$h]:-0}
  ((sp--))
  local lo=$(( p & 0xFFFFFFFF )) hi=$(( (p >> 32) & 0xFFFFFFFF ))
  (( lo >= 0x80000000 )) && (( lo -= 0x100000000 ))
  (( hi >= 0x80000000 )) && (( hi -= 0x100000000 ))
  push "$lo"; push "$hi"; push 0
}
p_read_line() {
  check_data 3 3 || return 0
  local h=${S[sp-1]} u1=${S[sp-2]} ca=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push 0; push 0; push $IOR_FILE; return 0; fi
  if (( u1 == 0 )); then push 0; push -1; push 0; return 0; fi
  local fd=${FHFD[$h]} line rc n i
  IFS= read -r -u "$fd" line; rc=$?
  n=${#line}
  (( n > u1 )) && n=$u1
  for ((i = 0; i < n; i++)); do ord "${line:i:1}"; cs $(( ca + i )) "$ORDV"; done
  FHPOS[$h]=$(( ${FHPOS[$h]:-0} + n + 1 ))
  push "$n"
  if (( rc == 0 || n > 0 )); then push -1; else push 0; fi
  push 0
}

p_find() {
  check_data 1 2 || return 0
  local cs=${S[sp-1]}; ((sp--))
  cf "$cs"
  search_word $(( cs + 1 )) "$CF"
  local w=$SWORD
  if (( w == 0 )); then
    push "$cs"; push 0
  elif has_flag "$w" "$IMMEDIATE"; then
    get_xt "$w"; push "$GXT"; push 1
  else
    get_xt "$w"; push "$GXT"; push -1
  fi
}

# --- Primitives: interpretation ----------------------------------------

p_interpret() {
  local ipos ilen cs tok tlen wxt st ch is_double t a temp_base first n
  while (( THR == 0 )); do
    user_get "$UV_IPOS"; ipos=$UGET
    user_get "$UV_ILEN"; ilen=$UGET
    (( ipos < ilen )) || break
    push 32
    p_word
    cs=${S[sp-1]}
    tok=$(( cs + 1 ))
    cf "$cs"; tlen=$CF
    if (( tlen == 0 )); then ((sp--)); break; fi
    if [ -n "$SLOTH_TRACE" ]; then
      user_get "$UV_STATE"
      mem_to_string "$tok" "$tlen"
      printf 'ipos=%s ilen=%s state=%s tok=[%s]\n' "$ipos" "$ilen" "$UGET" "$MTS" >&2
    fi
    p_find
    local flag=${S[sp-1]}; ((sp--))
    if (( flag != 0 )); then
      wxt=${S[sp-1]}; ((sp--))
      user_get "$UV_STATE"; st=$UGET
      if (( st == 0 || flag == 1 )); then eval_word "$wxt"; else compile "$wxt"; fi
    else
      ((sp--))
      if (( tlen == 3 )); then
        cf "$tok"; local c1=$CF
        cf $(( tok + 2 ))
        if (( c1 == 39 && CF == 39 )); then
          cf $(( tok + 1 )); ch=$CF
          user_get "$UV_STATE"
          if (( UGET == 0 )); then push "$ch"; else literal "$ch"; fi
          continue
        fi
      fi
      is_double=0; t=$tlen; a=$tok
      user_get "$UV_BASE"; temp_base=$UGET
      if (( t > 0 )); then
        cf $(( a + t - 1 ))
        if (( CF == 46 )); then ((t--)); is_double=1; fi
      fi
      cf "$a"; first=$CF
      if (( t > 0 && first == 35 )); then temp_base=10; ((t--)); ((a++))
      elif (( t > 0 && first == 36 )); then temp_base=16; ((t--)); ((a++))
      elif (( t > 0 && first == 37 )); then temp_base=2; ((t--)); ((a++)); fi
      mem_to_string "$a" "$t"
      if parse_int "$MTS" "$temp_base"; then
        n=$PINT
        user_get "$UV_STATE"
        if (( UGET == 0 )); then
          push "$n"
          if (( is_double )); then if (( n < 0 )); then push -1; else push 0; fi; fi
        else
          literal "$n"
          if (( is_double )); then if (( n < 0 )); then literal -1; else literal 0; fi; fi
        fi
      else
        if f_parse "$MTS"; then
          user_get "$UV_STATE"
          if (( UGET == 0 )); then fpush "$FVAL"; else f_literal "$FVAL"; fi
        else
          if [ -n "$SLOTH_DEBUG" ]; then printf 'UNDEFINED: [%s]\n' "$MTS" >&2; fi
          THR=-13
          return 0
        fi
      fi
    fi
  done
}

p_evaluate() {
  check_data 2 0 || return 0
  local l=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  local pibuf pipos pilen psid pspos
  user_get "$UV_IBUF"; pibuf=$UGET
  user_get "$UV_IPOS"; pipos=$UGET
  user_get "$UV_ILEN"; pilen=$UGET
  user_get "$UV_SOURCE_ID"; psid=$UGET
  user_get "$UV_SOURCE_POS"; pspos=$UGET
  user_set "$UV_SOURCE_ID" -1
  user_set "$UV_IBUF" "$a"
  user_set "$UV_IPOS" 0
  user_set "$UV_ILEN" "$l"
  user_get "$UV_INTERPRET"
  catch_xt "$UGET"
  user_set "$UV_SOURCE_ID" "$psid"
  user_set "$UV_IBUF" "$pibuf"
  user_set "$UV_IPOS" "$pipos"
  user_set "$UV_ILEN" "$pilen"
  user_set "$UV_SOURCE_POS" "$pspos"
  local e=${S[sp-1]}; ((sp--))
  if (( e != 0 )); then THR=$e; fi
}

p_execute() { check_data 1 0 || return 0; ((sp--)); eval_word "${S[sp]}"; }

# --- Primitives: defining words ----------------------------------------

p_colon() {
  user_get "$UV_STATE"
  if (( UGET != 0 )); then THR=-29; return 0; fi
  push 32; p_word
  local cs=${S[sp-1]}; ((sp--))
  cf "$cs"
  mem_to_string $(( cs + 1 )) "$CF"
  header_string "$MTS"
  get_latest; local w=$GLAT
  get_xt "$w"; user_set "$UV_LATESTXT" "$GXT"
  set_flag "$w" "$HIDDEN"
  user_set "$UV_STATE" 1
}
p_colon_no_name() {
  user_get "$UV_STATE"
  if (( UGET != 0 )); then THR=-29; return 0; fi
  push "$HERE"
  user_set "$UV_LATESTXT" "$HERE"
  user_set "$UV_STATE" 1
}
p_semicolon() {
  xt_of EXIT; compile "$XCT"
  user_set "$UV_STATE" 0
  get_latest; local w=$GLAT
  get_xt "$w"
  user_get "$UV_LATESTXT"
  if (( GXT == UGET )); then unset_flag "$w" "$HIDDEN"; fi
}
p_recurse() { user_get "$UV_LATESTXT"; compile "$UGET"; }
p_immediate() { get_latest; set_flag "$GLAT" "$IMMEDIATE"; }
p_compile_comma() { check_data 1 0 || return 0; ((sp--)); compile "${S[sp]}"; }
p_postpone() {
  push 32; p_word
  local cs=${S[sp-1]} tok=$(( ${S[sp-1]} + 1 ))
  cf "$cs"; local tlen=$CF
  if (( tlen == 0 )); then ((sp--)); return 0; fi
  p_find
  local i=${S[sp-1]} xt=${S[sp-2]}; ((sp-=2))
  if (( i == 0 )); then return 0; fi
  if (( i == -1 )); then literal "$xt"; xt_of 'COMPILE,'; compile "$XCT"; else compile "$xt"; fi
}
p_create_name() {
  local tlen=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  mem_to_string "$a" "$tlen"
  header_string "$MTS"
  xt_of '(RIP)'; compile "$XCT"
  compile $(( 4 * sCELL ))
  xt_of EXIT; compile "$XCT"
  compile "$XCT"
}
p_create() {
  push 32; p_word
  local c=${S[sp-1]}; ((sp--))
  push $(( c + 1 )); cf "$c"; push "$CF"
  p_create_name
}
p_do_does() { check_data 1 0 || return 0; get_latest; get_xt "$GLAT"; ((sp--)); store $(( GXT + 2 * sCELL )) "${S[sp]}"; }
p_does() {
  literal $(( HERE + 4 * sCELL ))
  xt_of '(DOES)'; compile "$XCT"
  xt_of EXIT; compile "$XCT"
}
p_start_quotation() {
  user_get "$UV_STATE"; local state=$UGET
  if (( state <= 0 )); then user_set "$UV_STATE" $(( state - 1 )); else user_set "$UV_STATE" $(( state + 1 )); fi
  user_get "$UV_STATE"
  if (( UGET == -1 )); then push $(( HERE + 2 * sCELL )); fi
  user_get "$UV_LATESTXT"; push "$UGET"
  xt_of '(QUOTATION)'; compile "$XCT"
  push "$HERE"
  comma 0
  user_set "$UV_LATESTXT" "$HERE"
}
p_end_quotation() {
  user_get "$UV_STATE"; local s=$UGET
  local a=${S[sp-1]}; ((sp--))
  xt_of EXIT; compile "$XCT"
  store "$a" $(( HERE - a - sCELL ))
  ((sp--)); user_set "$UV_LATESTXT" "${S[sp]}"
  if (( s < 0 )); then user_set "$UV_STATE" $(( s + 1 )); else user_set "$UV_STATE" $(( s - 1 )); fi
}

# --- Primitives: exceptions --------------------------------------------

p_catch() { check_data 1 1 || return 0; ((sp--)); catch_xt "${S[sp]}"; }
p_throw() {
  local e=${S[sp-1]}; ((sp--))
  if (( e == -2 )); then
    local l=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
    mem_to_string "$a" "$l"
    printf 'Error: %s\n' "$MTS" >&2
  fi
  THR=$e
}

# --- Primitives: executing and environment -----------------------------

p_debug() {
  local post=${S[sp-1]} inner=${S[sp-2]} pre=${S[sp-3]} q=${S[sp-4]}; ((sp-=4))
  push "$ip"; eval_word "$pre"
  eval_word "$q"
  if (( q > 0 )); then
    local t=$rp
    while (( t <= rp && ip >= 0 )); do
      push "$ip"; eval_word "$inner"
      local xtv=${MEM[$(( ip >> 2 ))]:-0}; ((ip += sCELL))
      if (( xtv < 0 )); then local qq=$(( -1 - xtv )); "${PRIM[$qq]}"; else
        if (( ip >= 0 || rp > 0 )); then R[rp]=$ip; ((rp++)); fi; ip=$xtv
      fi
    done
  fi
  push "$ip"; eval_word "$post"
}
p_environment() {
  local q=${S[sp-1]}; ((sp--))
  case $q in
    0) push 64 ;; 3) push 8 ;; 5) push 255 ;; 10) push 64 ;; 11) push 64 ;;
    12) push 64 ;; 100) push -1 ;; -1) push 5 ;; -2) push 10 ;; -3) push 127 ;;
  esac
}
p_self() { push 1; }
p_dict() { push 0; }
p_empty_rs() { rp=0; }

# --- MEMORY word set ---------------------------------------------------

p_allocate() {
  check_data 1 2 || return 0
  local u=${S[sp-1]}; ((sp--))
  if (( u < 0 )); then push 0; push $IOR_ALLOC; return 0; fi
  local a=$HEAP_PTR
  (( HEAP_PTR += (u + 7) & ~7 ))
  ALLOCSZ[$a]=$u
  push "$a"; push 0
}
p_free() {
  check_data 1 1 || return 0
  local a=${S[sp-1]}; ((sp--))
  unset 'ALLOCSZ[$a]'
  push 0
}
p_resize() {
  check_data 2 2 || return 0
  local u=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  if (( u < 0 )); then push "$a"; push $IOR_RESIZE; return 0; fi
  local old=${ALLOCSZ[$a]:-0}
  local na=$HEAP_PTR
  (( HEAP_PTR += (u + 7) & ~7 ))
  local n=$old i
  (( u < n )) && n=$u
  for ((i = 0; i < n; i++)); do cf $(( a + i )); cs $(( na + i )) "$CF"; done
  ALLOCSZ[$na]=$u
  unset 'ALLOCSZ[$a]'
  push "$na"; push 0
}
p_b_fetch() { check_data 1 1 || return 0; cf "${S[sp-1]}"; S[sp-1]=$CF; }
p_b_store() { check_data 2 0 || return 0; local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2)); cs "$a" "$v"; }
p_w_fetch() {
  check_data 1 1 || return 0
  local a=${S[sp-1]} b0 b1 r
  cf "$a"; b0=$CF
  cf $(( a + 1 )); b1=$CF
  r=$(( b0 | (b1 << 8) ))
  (( r >= 0x8000 )) && (( r -= 0x10000 ))
  S[sp-1]=$r
}
p_w_store() {
  check_data 2 0 || return 0
  local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2))
  cs "$a" $(( v & 0xFF ))
  cs $(( a + 1 )) $(( (v >> 8) & 0xFF ))
}
p_l_fetch() {
  check_data 1 1 || return 0
  local a=${S[sp-1]} i r=0
  for ((i = 0; i < 4; i++)); do cf $(( a + i )); (( r |= CF << (i * 8) )); done
  (( r &= 0xFFFFFFFF ))
  (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_l_store() {
  check_data 2 0 || return 0
  local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2))
  local i
  for ((i = 0; i < 4; i++)); do cs $(( a + i )) $(( (v >> (i * 8)) & 0xFF )); done
}
p_x_fetch() {
  check_data 1 1 || return 0
  local a=${S[sp-1]} i r=0
  for ((i = 0; i < 4; i++)); do cf $(( a + i )); (( r |= CF << (i * 8) )); done
  (( r &= 0xFFFFFFFF ))
  (( r >= 0x80000000 )) && (( r -= 0x100000000 ))
  S[sp-1]=$r
}
p_x_store() {
  check_data 2 0 || return 0
  local a=${S[sp-1]} v=${S[sp-2]}; ((sp-=2))
  local i
  for ((i = 0; i < 8; i++)); do cs $(( a + i )) $(( (v >> (i * 8)) & 0xFF )); done
}

# --- FILE word set -----------------------------------------------------

file_open() { # $1 path, $2 fam, $3 create -> OPEN_FD ("" on failure)
  local path=$1 fam=$2 create=$3
  local f=$(( fam & 0x0f )) fd
  OPEN_FD=""
  if (( create )); then
    : >"$path" 2>/dev/null || return 0
    if (( f == 1 )); then
      exec {fd}<"$path" 2>/dev/null || return 0
    else
      exec {fd}<>"$path" 2>/dev/null || return 0
    fi
  else
    [ -e "$path" ] || return 0
    if (( f == 1 )); then
      exec {fd}<"$path" 2>/dev/null || return 0
    else
      exec {fd}<>"$path" 2>/dev/null || return 0
    fi
  fi
  OPEN_FD=$fd
}

# Sets NEW_HANDLE ("" on failure).
file_new_handle() {
  local path=$1 fam=$2 create=$3
  file_open "$path" "$fam" "$create"
  if [ -z "$OPEN_FD" ]; then NEW_HANDLE=""; return; fi
  ((FIDN++)); NEW_HANDLE=$FIDN
  FHFD[$FIDN]=$OPEN_FD
  FHPATH[$FIDN]=$path
  FHPOS[$FIDN]=0
}

p_bin() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} | 16 )); }
p_r_o() { check_data 0 1 || return 0; push 1; }
p_r_w() { check_data 0 1 || return 0; push 2; }
p_w_o() { check_data 0 1 || return 0; push 3; }

p_create_file() {
  check_data 3 2 || return 0
  local fam=${S[sp-1]} u=${S[sp-2]} a=${S[sp-3]}; ((sp-=3))
  mem_to_string "$a" "$u"
  file_new_handle "$MTS" "$fam" 1
  if [ -z "$NEW_HANDLE" ]; then push 0; push $IOR_FILE; return 0; fi
  push "$NEW_HANDLE"; push 0
}
p_open_file() {
  check_data 3 2 || return 0
  local fam=${S[sp-1]} u=${S[sp-2]} a=${S[sp-3]}; ((sp-=3))
  mem_to_string "$a" "$u"
  file_new_handle "$MTS" "$fam" 0
  if [ -z "$NEW_HANDLE" ]; then push 0; push $IOR_FILE; return 0; fi
  push "$NEW_HANDLE"; push 0
}
p_close_file() {
  check_data 1 1 || return 0
  local h=${S[sp-1]}
  if [ -z "${FHFD[$h]:-}" ]; then S[sp-1]=$IOR_FILE; return 0; fi
  eval "exec ${FHFD[$h]}>&-"
  unset 'FHFD[$h]' 'FHPATH[$h]' 'FHPOS[$h]'
  S[sp-1]=0
}
p_file_size() {
  check_data 1 3 || return 0
  local h=${S[sp-1]} sz
  if [ -z "${FHFD[$h]:-}" ]; then ((sp--)); push 0; push 0; push $IOR_FILE; return 0; fi
  if ! sz=$(stat -c %s "${FHPATH[$h]}" 2>/dev/null); then
    ((sp--)); push 0; push 0; push $IOR_FILE; return 0
  fi
  ((sp--))
  local lo=$(( sz & 0xFFFFFFFF )) hi=$(( (sz >> 32) & 0xFFFFFFFF ))
  (( lo >= 0x80000000 )) && (( lo -= 0x100000000 ))
  (( hi >= 0x80000000 )) && (( hi -= 0x100000000 ))
  push "$lo"; push "$hi"; push 0
}
p_flush_file() {
  check_data 1 1 || return 0
  local h=${S[sp-1]}
  if [ -z "${FHFD[$h]:-}" ]; then S[sp-1]=$IOR_FILE; return 0; fi
  sync 2>/dev/null || true
  S[sp-1]=0
}
p_resize_file() {
  check_data 3 1 || return 0
  local h=${S[sp-1]} hi=${S[sp-2]} lo=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push $IOR_FILE; return 0; fi
  local n=$(( ((hi & 0xFFFFFFFF) << 32) | (lo & 0xFFFFFFFF) ))
  truncate -s "$n" "${FHPATH[$h]}" 2>/dev/null || { push $IOR_FILE; return 0; }
  push 0
}
p_delete_file() {
  check_data 2 1 || return 0
  local u=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  mem_to_string "$a" "$u"
  if rm -f -- "$MTS" 2>/dev/null; then push 0; else push $IOR_FILE; fi
}
p_rename_file() {
  check_data 4 1 || return 0
  local u2=${S[sp-1]} c2=${S[sp-2]} u1=${S[sp-3]} c1=${S[sp-4]}; ((sp-=4))
  mem_to_string "$c1" "$u1"; local n1=$MTS
  mem_to_string "$c2" "$u2"; local n2=$MTS
  if mv -- "$n1" "$n2" 2>/dev/null; then push 0; else push $IOR_FILE; fi
}
p_file_status() {
  check_data 2 2 || return 0
  local u=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  mem_to_string "$a" "$u"
  push 0
  if [ -e "$MTS" ]; then push 0; else push $IOR_FILE; fi
}
p_read_file() {
  check_data 3 2 || return 0
  local h=${S[sp-1]} u1=${S[sp-2]} ca=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push 0; push $IOR_FILE; return 0; fi
  local pos=${FHPOS[$h]:-0}
  local n=0 b
  local bytes
  bytes=$(dd if="${FHPATH[$h]}" bs=1 skip="$pos" count="$u1" 2>/dev/null | od -An -tu1 -v 2>/dev/null)
  local i=0
  for b in $bytes; do cs $(( ca + i )) "$b"; ((i++)); done
  n=$i
  FHPOS[$h]=$(( pos + n ))
  push "$n"; push 0
}
p_write_file() {
  check_data 3 1 || return 0
  local h=${S[sp-1]} u=${S[sp-2]} a=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push $IOR_FILE; return 0; fi
  local fd=${FHFD[$h]} i oct
  for ((i = 0; i < u; i++)); do
    cf $(( a + i ))
    printf -v oct '%03o' "$CF"
    printf "\\$oct" >&"$fd"
  done
  FHPOS[$h]=$(( ${FHPOS[$h]:-0} + u ))
  push 0
}
p_write_line() {
  check_data 3 1 || return 0
  local h=${S[sp-1]} u=${S[sp-2]} a=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push $IOR_FILE; return 0; fi
  local fd=${FHFD[$h]} i oct
  for ((i = 0; i < u; i++)); do
    cf $(( a + i ))
    printf -v oct '%03o' "$CF"
    printf "\\$oct" >&"$fd"
  done
  printf '\n' >&"$fd"
  FHPOS[$h]=$(( ${FHPOS[$h]:-0} + u + 1 ))
  push 0
}
p_reposition_file() {
  check_data 3 1 || return 0
  local h=${S[sp-1]} hi=${S[sp-2]} lo=${S[sp-3]}; ((sp-=3))
  if [ -z "${FHFD[$h]:-}" ]; then push $IOR_FILE; return 0; fi
  local off=$(( ((hi & 0xFFFFFFFF) << 32) | (lo & 0xFFFFFFFF) ))
  local path=${FHPATH[$h]} fd=${FHFD[$h]}
  eval "exec $fd>&-"
  exec {fd}<"$path" 2>/dev/null || { push $IOR_FILE; return 0; }
  FHFD[$h]=$fd
  local remaining=$off chunk skipped
  while (( remaining > 0 )); do
    chunk=$remaining
    (( chunk > 4096 )) && chunk=4096
    IFS= read -r -N "$chunk" -u "$fd" _skip || true
    skipped=${#_skip}
    (( skipped == 0 )) && break
    (( remaining -= skipped ))
  done
  FHPOS[$h]=$(( off - remaining ))
  push 0
}

# --- FLOAT word set ----------------------------------------------------
#
# bash has no floating point arithmetic; awk does the math. Float values live
# on a separate stack as decimal strings (%.17g round-trips an IEEE double).
# F@/F! convert between that and raw IEEE-754 bytes using bash's `printf %a`.
# SF@/SF!/DF@/DF! are aliases of the double versions.

fcheck() {
  if (( fp < $1 )); then THR=$FLOAT_STACK_UNDERFLOW; return 1; fi
  if (( fp - $1 + $2 > FLOAT_STACK_SIZE )); then THR=$FLOAT_STACK_OVERFLOW; return 1; fi
  return 0
}
fpush() { F[fp]=$1; ((fp++)); }
f_parse() { # $1 decimal/hex-ish string -> FVAL (0 on failure); accepts 0E, 1E- like parse_float
  local s=$1
  s=${s//D/e}
  s=${s//d/e}
  if [[ $s =~ ^(.+[eE][+-]?)$ ]]; then
    s=${BASH_REMATCH[1]}
    s=${s%[eE+-]}
    s=${s%[eE+-]}
  fi
  if [[ $s =~ ^[+-]?([0-9]+\.?[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?$ ]] && [[ $s =~ [0-9] ]]; then
    FVAL=$(awk -v S="$s" 'BEGIN{printf "%.17g", S+0}' 2>/dev/null)
    [ -n "$FVAL" ] && return 0
  fi
  return 1
}
f_normv() { case $1 in +inf | inf) FRES=inf ;; -inf) FRES=-inf ;; *nan*) FRES=nan ;; *) FRES=$1 ;; esac; }
f_awk1() {
  local a=$1
  case $a in inf) a=1e999 ;; -inf) a=-1e999 ;; esac
  local o
  o=$(awk -v A="$a" "BEGIN{printf \"%.17g\", $2}" 2>/dev/null)
  f_normv "$o"
}
f_awk2() {
  local a=$1 b=$2
  case $a in inf) a=1e999 ;; -inf) a=-1e999 ;; esac
  case $b in inf) b=1e999 ;; -inf) b=-1e999 ;; esac
  local o
  o=$(awk -v A="$a" -v B="$b" "BEGIN{printf \"%.17g\", $3}" 2>/dev/null)
  f_normv "$o"
}
fun() {
  fcheck 1 1 || return 0
  ((fp--)); local a=${F[fp]}
  if [ "$a" = nan ]; then fpush nan; return 0; fi
  f_awk1 "$a" "$1"
  fpush "$FRES"
}
fbin() {
  fcheck 2 1 || return 0
  ((fp--)); local b=${F[fp]}
  ((fp--)); local a=${F[fp]}
  if [ "$a" = nan ] || [ "$b" = nan ]; then fpush nan; return 0; fi
  f_awk2 "$a" "$b" "$1"
  fpush "$FRES"
}

f_dec_to_bytes() {
  local dec=$1 h sign=0
  FBYTE=(0 0 0 0 0 0 0 0)
  h=$(printf '%a' "$dec" 2>/dev/null) || h="0x0p+0"
  case $h in
    *nan* | *NaN*) FBYTE=(0 0 0 0 0 0 248 127); return ;;
  esac
  case $h in -*) sign=1; h=${h#-} ;; esac
  case $h in inf*) FBYTE=(0 0 0 0 0 0 240 $(( (sign << 7) | 0x7F ))); return ;; esac
  h=${h#0x}
  local mant exps exp intp fracp M k
  mant=${h%%[pP]*}
  exps=${h#*[pP]}
  exp=$(( exps + 0 ))
  intp=${mant%%.*}
  fracp=""
  case $mant in *.*) fracp=${mant#*.} ;; esac
  M=$(( 16#$intp ))
  [ -n "$fracp" ] && M=$(( (M << (4 * ${#fracp})) + 16#$fracp ))
  k=$(( exp - 4 * ${#fracp} ))
  if (( M == 0 )); then
    FBYTE=(0 0 0 0 0 0 $(( sign << 7 )) 0)
    return
  fi
  local expfield fracfield msb t
  msb=0; t=$M
  while (( t > 1 )); do (( t >>= 1 )); ((msb++)); done
  local E=$(( msb + k ))
  if (( E > 1023 )); then
    expfield=0x7FF; fracfield=0
  elif (( E < -1022 )); then
    expfield=0
    local sh=$(( k + 1074 ))
    if (( sh >= 0 )); then fracfield=$(( (M << sh) & 0xFFFFFFFFFFFFF )); else fracfield=$(( M >> (-sh) )); fi
  else
    expfield=$(( E + 1023 ))
    fracfield=$(( (M << (52 - msb)) - 0x10000000000000 ))
  fi
  fracfield=$(( fracfield & 0xFFFFFFFFFFFFF ))
  FBYTE[0]=$(( fracfield & 0xFF ))
  FBYTE[1]=$(( (fracfield >> 8) & 0xFF ))
  FBYTE[2]=$(( (fracfield >> 16) & 0xFF ))
  FBYTE[3]=$(( (fracfield >> 24) & 0xFF ))
  FBYTE[4]=$(( (fracfield >> 32) & 0xFF ))
  FBYTE[5]=$(( (fracfield >> 40) & 0xFF ))
  FBYTE[6]=$(( ((fracfield >> 48) & 0x0F) | ((expfield & 0x0F) << 4) ))
  FBYTE[7]=$(( (sign << 7) | ((expfield >> 4) & 0x7F) ))
}

f_bytes_to_dec() {
  local b0=$1 b1=$2 b2=$3 b3=$4 b4=$5 b5=$6 b6=$7 b7=$8
  local sign=$(( (b7 >> 7) & 1 ))
  local expfield=$(( ((b7 & 0x7F) << 4) | ((b6 >> 4) & 0x0F) ))
  local frac=$(( ((b6 & 0x0F) << 48) | (b5 << 40) | (b4 << 32) | (b3 << 24) | (b2 << 16) | (b1 << 8) | b0 ))
  if (( expfield == 0x7FF )); then
    if (( frac == 0 )); then if (( sign )); then FDEC=-inf; else FDEC=inf; fi; else FDEC=nan; fi
    return
  fi
  local m exp
  if (( expfield == 0 )); then
    if (( frac == 0 )); then if (( sign )); then FDEC=-0; else FDEC=0; fi; return; fi
    m=$frac; exp=-1074
  else
    m=$(( frac + 0x10000000000000 )); exp=$(( expfield - 1023 - 52 ))
  fi
  local hs hstr
  printf -v hs '%x' "$m"
  hstr="0x${hs}p${exp}"
  if (( sign )); then hstr="-${hstr}"; fi
  FDEC=$(printf '%.17g' "$hstr" 2>/dev/null) || FDEC=0
}

f_literal() {
  xt_of '(FLIT)'; comma "$XCT"
  f_dec_to_bytes "$1"
  local i
  for ((i = 0; i < 8; i++)); do cs "$HERE" "${FBYTE[$i]}"; ((HERE++)); done
}

p_f_lit() {
  fcheck 0 1 || return 0
  local i
  for ((i = 0; i < 8; i++)); do cf $(( ip + i )); FBYTE[i]=$CF; done
  f_bytes_to_dec "${FBYTE[0]}" "${FBYTE[1]}" "${FBYTE[2]}" "${FBYTE[3]}" \
    "${FBYTE[4]}" "${FBYTE[5]}" "${FBYTE[6]}" "${FBYTE[7]}"
  fpush "$FDEC"
  ((ip += 8))
}
p_f_align() { HERE=$(( (HERE + 7) & ~7 )); }
p_f_aligned() { check_data 1 1 || return 0; S[sp-1]=$(( (${S[sp-1]} + 7) & ~7 )); }
p_f_literal() { fcheck 1 0 || return 0; ((fp--)); f_literal "${F[fp]}"; }
p_floats() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} * 8 )); }
p_float_plus() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} + 8 )); }
p_s_f_aligned() { check_data 1 1 || return 0; S[sp-1]=$(( (${S[sp-1]} + 3) & ~3 )); }
p_d_f_aligned() { check_data 1 1 || return 0; S[sp-1]=$(( (${S[sp-1]} + 7) & ~7 )); }
p_s_floats() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} * 4 )); }
p_d_floats() { check_data 1 1 || return 0; S[sp-1]=$(( ${S[sp-1]} * 8 )); }

p_f_depth() { check_data 0 1 || return 0; push "$fp"; }
p_f_drop() { fcheck 1 0 || return 0; ((fp--)); }
p_f_dup() { fcheck 1 2 || return 0; fpush "${F[fp-1]}"; }
p_f_over() { fcheck 2 3 || return 0; fpush "${F[fp-2]}"; }
p_f_rot() {
  fcheck 3 3 || return 0
  local c=${F[fp-1]} b=${F[fp-2]} a=${F[fp-3]}
  F[fp-3]=$b; F[fp-2]=$c; F[fp-1]=$a
}
p_f_swap() { fcheck 2 2 || return 0; local t=${F[fp-1]}; F[fp-1]=${F[fp-2]}; F[fp-2]=$t; }

p_f_less() {
  fcheck 2 0 || return 0; check_data 0 1 || return 0
  local b=${F[fp-1]} a=${F[fp-2]}; ((fp-=2))
  if [ "$a" = nan ] || [ "$b" = nan ]; then push 0; return 0; fi
  case $a$b in
    *inf*)
      awk -v A="$a" -v B="$b" 'BEGIN{exit !(A<B)}' 2>/dev/null && push -1 || push 0 ;;
    *) f_awk2 "$a" "$b" "(A<B)?1:0"; [ "$FRES" = 1 ] && push -1 || push 0 ;;
  esac
}
p_f_zero_less() {
  fcheck 1 0 || return 0; check_data 0 1 || return 0
  local a=${F[fp-1]}; ((fp--))
  case $a in nan) push 0 ;; -inf) push -1 ;; inf) push 0 ;; *) f_awk1 "$a" "(A<0)?1:0"; [ "$FRES" = 1 ] && push -1 || push 0 ;; esac
}
p_f_zero_equals() {
  fcheck 1 0 || return 0; check_data 0 1 || return 0
  local a=${F[fp-1]}; ((fp--))
  case $a in nan | inf | -inf) push 0 ;; 0 | 0.0 | -0) push -1 ;; *) f_awk1 "$a" "(A==0)?1:0"; [ "$FRES" = 1 ] && push -1 || push 0 ;; esac
}
p_f_fetch() {
  check_data 1 0 || return 0; fcheck 0 1 || return 0
  local a=${S[sp-1]}; ((sp--))
  local i
  for ((i = 0; i < 8; i++)); do cf $(( a + i )); FBYTE[i]=$CF; done
  f_bytes_to_dec "${FBYTE[0]}" "${FBYTE[1]}" "${FBYTE[2]}" "${FBYTE[3]}" \
    "${FBYTE[4]}" "${FBYTE[5]}" "${FBYTE[6]}" "${FBYTE[7]}"
  fpush "$FDEC"
}
p_f_store() {
  check_data 1 0 || return 0; fcheck 1 0 || return 0
  local a=${S[sp-1]}; ((sp--))
  ((fp--)); f_dec_to_bytes "${F[fp]}"
  local i
  for ((i = 0; i < 8; i++)); do cs $(( a + i )) "${FBYTE[$i]}"; done
}
p_d_to_f() {
  check_data 2 0 || return 0; fcheck 0 1 || return 0
  local lo=${S[sp-2]} hi=${S[sp-1]}; ((sp-=2))
  local v
  v=$(awk -v H="$hi" -v L="$lo" 'BEGIN{printf "%.17g", ((H*4294967296)+ (L<0?L+4294967296:L))}' 2>/dev/null)
  [ -z "$v" ] && v="0"
  fpush "$v"
}
p_f_to_d() {
  fcheck 1 0 || return 0; check_data 0 2 || return 0
  ((fp--)); local a=${F[fp]}
  local v
  v=$(awk -v A="$a" 'BEGIN{ if(A!=A)A=0; if(A>9223372036854775807)A=9223372036854775807; else if(A<-9223372036854775808)A=-9223372036854775808; else A=int(A); printf "%.0f",A }' 2>/dev/null)
  [ -z "$v" ] && v=0
  local lo=$(( v & 0xFFFFFFFF )) hi=$(( (v >> 32) & 0xFFFFFFFF ))
  (( lo >= 0x80000000 )) && (( lo -= 0x100000000 ))
  (( hi >= 0x80000000 )) && (( hi -= 0x100000000 ))
  push "$lo"; push "$hi"
}

p_f_plus() { fbin "A+B"; }
p_f_minus() { fbin "A-B"; }
p_f_star() { fbin "A*B"; }
p_f_star_star() { fbin "A^B"; }
p_f_slash() {
  fcheck 2 1 || return 0
  ((fp--)); local b=${F[fp]}
  ((fp--)); local a=${F[fp]}
  if [ "$a" = nan ] || [ "$b" = nan ]; then fpush nan; return 0; fi
  case $b in
    0 | 0.0 | -0 | -0.0)
      case $a in 0 | 0.0 | -0 | -0.0) fpush nan ;; *) case $a in -*) fpush -inf ;; *) fpush inf ;; esac ;; esac
      return 0 ;;
  esac
  f_awk2 "$a" "$b" "A/B"; fpush "$FRES"
}
p_floor() { fun "int(A)-((A<0 && A!=int(A))?1:0)"; }
p_f_round() {
  fcheck 1 1 || return 0
  ((fp--)); local a=${F[fp]}
  case $a in nan) fpush nan; return 0 ;; inf | -inf) fpush "$a"; return 0 ;; esac
  f_awk1 "$a" "int(A)-((A<0 && A!=int(A))?1:0)"
  local f=$FRES
  f_awk1 "$a" "A-((int(A)-((A<0 && A!=int(A))?1:0)))"
  local d=$FRES
  if f_awk1 "$d" "(A>0.5)?2:((A<0.5)?0:((int(A)%2==0)?0:1))"; then :; fi
  case $FRES in
    2) f_awk1 "$f" "A+1"; fpush "$FRES" ;;
    0) fpush "$f" ;;
    *) f_awk1 "$f" "(A%2==0)?A:A+1"; fpush "$FRES" ;;
  esac
}
p_f_max() { fbin "(A>B)?A:B"; }
p_f_min() { fbin "(A<B)?A:B"; }
p_f_abs() { fun "(A<0)?-A:A"; }
p_f_negate() { fun "-A"; }
p_f_proximate() {
  fcheck 3 0 || return 0; check_data 0 1 || return 0
  local r3=${F[fp-1]} r2=${F[fp-2]} r1=${F[fp-3]}; ((fp-=3))
  case $r3 in nan) push 0; return 0 ;; inf) push -1; return 0 ;; -inf) push 0; return 0 ;; esac
  local o
  o=$(awk -v R1="$r1" -v R2="$r2" -v R3="$r3" 'function ab(x){return x<0?-x:x} BEGIN{
    if (R3>0) c=(ab(R1-R2)<R3); else if (R3<0) c=(ab(R1-R2)<ab(R3)*(ab(R1)+ab(R2))); else c=(R1==R2 && (1/R1)==(1/R2));
    print c?1:0 }' 2>/dev/null)
  [ "$o" = 1 ] && push -1 || push 0
}
p_f_sqrt() { fun "sqrt(A)"; }
p_f_ln() { fun "log(A)"; }
p_f_exp() { fun "exp(A)"; }
p_f_exp_m_one() { fun "exp(A)-1"; }
p_f_log_ten() { fun "log(A)/log(10)"; }
p_f_lnp1() { fun "log(A+1)"; }
p_f_alog() { fun "10^A"; }
p_f_sine() { fun "sin(A)"; }
p_f_cos() { fun "cos(A)"; }
p_f_sincos() {
  fcheck 1 2 || return 0
  ((fp--)); local a=${F[fp]}
  if [ "$a" = nan ]; then fpush nan; fpush nan; return 0; fi
  f_awk1 "$a" "sin(A)"; local s=$FRES
  f_awk1 "$a" "cos(A)"; fpush "$s"; fpush "$FRES"
}
p_f_tan() { fun "sin(A)/cos(A)"; }
p_f_asin() { fun "atan2(A, sqrt(1-A*A))"; }
p_f_acos() { fun "atan2(sqrt(1-A*A), A)"; }
p_f_atan() { fun "atan2(A,1)"; }
p_f_atan2() { fbin "atan2(A,B)"; }
p_f_sinh() { fun "(exp(A)-exp(-A))/2"; }
p_f_cosh() { fun "(exp(A)+exp(-A))/2"; }
p_f_tanh() { fun "(exp(A)-exp(-A))/(exp(A)+exp(-A))"; }
p_f_asinh() { fun "log(A+sqrt(A*A+1))"; }
p_f_acosh() { fun "log(A+sqrt(A*A-1))"; }

p_to_float() {
  check_data 2 1 || return 0
  local u=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  if (( u == 0 )); then push 0; return 0; fi
  local allspace=1 i
  for ((i = 0; i < u; i++)); do cf $(( a + i )); (( CF == 32 )) || { allspace=0; break; }; done
  if (( allspace )); then fpush 0; push -1; return 0; fi
  cf $(( a + u - 1 ))
  if (( CF == 32 )); then push 0; return 0; fi
  mem_to_string "$a" "$u"
  if f_parse "$MTS"; then
    fpush "$FVAL"; push -1
  else
    push 0
  fi
}

p_represent() {
  fcheck 1 0 || return 0; check_data 2 3 || return 0
  local u=${S[sp-1]} addr=${S[sp-2]}; ((sp-=2))
  ((fp--)); local r=${F[fp]}
  local n f1 f2 digits i
  case $r in
    nan | inf | -inf)
      local marker
      case $r in nan) marker=nan ;; inf) marker=+infinity ;; -inf) marker=-infinity ;; esac
      n=0; f1=0; f2=0; digits=$marker
      while (( ${#digits} < u )); do digits+=' '; done
      digits=${digits:0:u} ;;
    0 | -0)
      n=0; [ "$r" = -0 ] && f1=-1 || f1=0; f2=-1; digits=$(printf '%0*d' "$u" 0) ;;
    *)
      local out
      out=$(awk -v R="$r" -v U="$u" 'BEGIN{
        neg=(R<0); a=(R<0)?-R:R;
        s=sprintf("%.*e", U-1, a);
        p=index(s,"e"); mant=substr(s,1,p-1); e=substr(s,p+1)+0;
        gsub(/\./,"",mant);
        while(length(mant)<U) mant=mant "0";
        if(length(mant)>U) mant=substr(mant,1,U);
        printf "%d %d %d %s", e+1, neg?-1:0, -1, mant }' 2>/dev/null)
      read -r n f1 f2 digits <<<"$out"
      while (( ${#digits} < u )); do digits+='0'; done
      digits=${digits:0:u} ;;
  esac
  for ((i = 0; i < u; i++)); do
    local ch=${digits:i:1}
    [ -z "$ch" ] && ch=0
    ord "$ch"; cs $(( addr + i )) "$ORDV"
  done
  push "$n"; push "$f1"; push "$f2"
}

p_f_dot() {
  fcheck 1 0 || return 0
  ((fp--)); local r=${F[fp]}
  user_get "$UV_PRECISION"; local prec=$UGET
  case $r in
    nan) printf 'NaN '; return 0 ;;
    inf) printf 'Inf '; return 0 ;;
    -inf) printf '-Inf '; return 0 ;;
  esac
  local out
  out=$(awk -v R="$r" -v P="$prec" 'BEGIN{
    if (R==0) {printf "0. "}
    else if (R==int(R)) {printf "%.0f. ", R}
    else { ad=length(sprintf("%.0f", (R<0)?-R:R)); d=P-ad; if(d<0)d=0; printf "%.*f ", d, R } }' 2>/dev/null)
  printf '%s' "$out"
}
p_f_s_dot() {
  fcheck 1 0 || return 0
  ((fp--)); local r=${F[fp]}
  user_get "$UV_PRECISION"; local prec=$UGET
  case $r in
    nan) printf 'NaN '; return 0 ;;
    inf) printf 'Inf '; return 0 ;;
    -inf) printf '-Inf '; return 0 ;;
  esac
  awk -v R="$r" -v P="$prec" 'BEGIN{
    s=sprintf("%.*e", P-1, R); p=index(s,"e");
    m=substr(s,1,p-1); e=substr(s,p+1)+0;
    sign=(e<0)?"-":"+"; e=(e<0)?-e:e; d=sprintf("%02d", e);
    printf "%sE%s%s ", m, sign, d }' 2>/dev/null
}
p_f_e_dot() {
  fcheck 1 0 || return 0
  ((fp--)); local r=${F[fp]}
  case $r in
    nan) printf 'NaN '; return 0 ;;
    inf) printf 'Inf '; return 0 ;;
    -inf) printf '-Inf '; return 0 ;;
  esac
  awk -v R="$r" 'BEGIN{
    if (R==0) {printf "0.0E+00 "; exit}
    e=int(log((R<0)?-R:R)/log(10)/3)*3; sc=R/(10^e);
    s=sprintf("%.3f", sc); sign=(e<0)?"-":"+"; e=(e<0)?-e:e;
    printf "%sE%s%02d ", s, sign, e }' 2>/dev/null
}
p_f_dot_s() {
  local out="F:<$fp> " i
  for ((i = 0; i < fp; i++)); do
    out+=$(awk -v R="${F[i]}" 'BEGIN{ if(R=="inf"||R=="-inf"||R=="nan"){printf "%s ",R} else printf "%.6f ", R }' 2>/dev/null)
  done
  printf '%s' "$out"
}

p_s_f_fetch() { p_f_fetch; }
p_s_f_store() { p_f_store; }
p_d_f_fetch() { p_f_fetch; }
p_d_f_store() { p_f_store; }

# --- INCLUDED (host side) ----------------------------------------------

src_push() {
  user_get "$UV_SOURCE_POS"; local a=$UGET
  user_get "$UV_SOURCE_ID"; local b=$UGET
  user_get "$UV_IBUF"; local c=$UGET
  user_get "$UV_IPOS"; local d=$UGET
  user_get "$UV_ILEN"; local e=$UGET
  SRC_STACK+=("$a|$b|$c|$d|$e")
}
src_pop() {
  local n=${#SRC_STACK[@]}; ((n--))
  local v=${SRC_STACK[$n]}; unset 'SRC_STACK[n]'
  local a b c d f
  IFS='|' read -r a b c d f <<<"$v"
  user_set "$UV_SOURCE_POS" "$a"; user_set "$UV_SOURCE_ID" "$b"
  user_set "$UV_IBUF" "$c"; user_set "$UV_IPOS" "$d"; user_set "$UV_ILEN" "$f"
}

resolve_path() {
  local name=$1 d
  [ -f "$name" ] && { printf '%s' "$name"; return 0; }
  for d in "${SLOTH_PATHS[@]}"; do
    [ -f "$d$name" ] && { printf '%s' "$d$name"; return 0; }
  done
  return 1
}

host_include() {
  local name=$1 path fd fid lineno=0 ior=0 flag e
  if ! path=$(resolve_path "$name"); then THR=-38; return 1; fi
  exec {fd}<"$path" || { THR=-38; return 1; }
  ((NEXTFID++)); fid=$NEXTFID
  FILEFD[$fid]=$fd
  src_push
  add_included_file "$name"
  user_set "$UV_SOURCE_ID" "$fid"
  user_set "$UV_IBUF" "$LINEBUF"
  user_set "$UV_IPOS" 0
  user_set "$UV_ILEN" 0
  while :; do
    p_refill
    flag=${S[sp-1]}; ((sp--))
    (( flag == 0 )) && break
    user_get "$UV_INTERPRET"
    catch_xt "$UGET"
    e=${S[sp-1]}; ((sp--))
    if (( e != 0 )); then
      user_get "$UV_IBUF"; local ib=$UGET
      user_get "$UV_ILEN"; local il=$UGET
      mem_to_string "$ib" "$il"
      printf 'File: %s\nLine (%d): %s\n' "$path" "$lineno" "$MTS" >&2
      ior=$e
      break
    fi
    ((lineno++))
  done
  exec {fd}>&-
  src_pop
  if (( ior != 0 )); then THR=$ior; return 1; fi
  return 0
}

p_included() {
  check_data 2 0 || return 0
  local l=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  mem_to_string "$a" "$l"
  host_include "$MTS"
}

add_included_file() {
  local s=$1
  local l=${#s} i
  local node=$HERE
  user_get "$UV_INCLUDED_FILES"; comma "$UGET"
  user_set "$UV_INCLUDED_FILES" "$node"
  comma "$l"
  for ((i = 0; i < l; i++)); do ord "${s:i:1}"; c_comma "$ORDV"; done
  align
}

is_file_included() {
  local a1=$1 u1=$2 name u2 a2
  user_get "$UV_INCLUDED_FILES"; name=$UGET
  while (( name != 0 )); do
    fetch $(( name + 4 )); u2=$FCH
    a2=$(( name + 8 ))
    if (( u1 == u2 )) && mem_eq_ci "$a1" "$a2" "$u2"; then return 0; fi
    fetch "$name"; name=$FCH
  done
  return 1
}

p_required() {
  check_data 2 0 || return 0
  local u=${S[sp-1]} a=${S[sp-2]}; ((sp-=2))
  if ! is_file_included "$a" "$u"; then
    mem_to_string "$a" "$u"
    host_include "$MTS"
  fi
}

p_require() {
  push 32; p_word
  local cs=${S[sp-1]}; ((sp--))
  cf "$cs"
  push $(( cs + 1 )); push "$CF"
  p_required
}

# --- Bootstrap ---------------------------------------------------------

bootstrap() {
  build_char_tables
  MEM=(); S=(); R=(); PRIM=(); HASH=(); PXT=(); FILEFD=(); SRC_STACK=(); F=()
  sp=0; rp=0; fp=0; ip=-1; HERE=12; THR=0; PN=0; LASTXT=0; NEXTFID=0
  HEAP_PTR=$((1 << 27))
  MEM[1]=0; MEM[2]=0
  store $(( USER_BASE + UV_CURRENT )) 8
  store $(( USER_BASE + UV_ORDER )) 2
  store $(( USER_BASE + UV_LOCALS_WORDLIST )) 0
  store $(( USER_BASE + UV_CONTEXT )) 8
  store $(( USER_BASE + UV_CONTEXT + 4 )) 4

  code EXIT p_exit
  code '(LIT)' p_lit
  code '(RIP)' p_rip
  code '(BRANCH)' p_branch
  code '(?BRANCH)' p_zbranch

  user_variable '(CURRENT)' "$UV_CURRENT" 8
  user_variable '#ORDER' "$UV_ORDER" 2
  user_variable '(LOCALS-WORDLIST)' "$UV_LOCALS_WORDLIST" 0
  user_variable CONTEXT "$UV_CONTEXT" 8
  user_variable BASE "$UV_BASE" 10
  user_variable STATE "$UV_STATE" 0
  user_variable '(IBUF)' "$UV_IBUF" 0
  user_variable '>IN' "$UV_IPOS" 0
  user_variable '(ILEN)' "$UV_ILEN" 0
  user_variable '(SOURCE-ID)' "$UV_SOURCE_ID" 0
  user_variable '(SOURCE-POS)' "$UV_SOURCE_POS" 0
  user_variable '(LATESTXT)' "$UV_LATESTXT" 0
  add_prim p_interpret
  user_variable '(INTERPRET)' "$UV_INTERPRET" "$LASTXT"
  user_variable '(SLOTH_ROOT_PATH_LENGTH)' "$UV_ROOT_PATH_LENGTH" 0
  user_variable '(SLOTH_PATH_START)' "$UV_PATH_START" $(( USER_BASE + UV_PATHS ))
  user_variable '(SLOTH_PATH_END)' "$UV_PATH_END" $(( USER_BASE + UV_PATHS ))
  user_variable '(SLOTH_PATHS)' "$UV_PATHS" 0
  user_variable '(INCLUDED-FILES)' "$UV_INCLUDED_FILES" 0
  user_variable '(PRECISION)' "$UV_PRECISION" 15

  code DROP p_drop
  code DUP p_dup
  code OVER p_over
  code '>R' p_to_r
  code 'R>' p_r_from
  code SWAP p_swap
  code 'C@' p_c_fetch
  code 'C!' p_c_store
  code '@' p_fetch
  code '!' p_store
  code CELLS p_cells
  code CHARS p_chars
  code HERE p_here
  code ALIGN p_align
  code ALLOT p_allot
  code UNUSED p_unused
  code CATCH p_catch
  code THROW p_throw
  code INVERT p_invert
  code AND p_and
  code LSHIFT p_l_shift
  code '-' p_minus
  code '+' p_plus
  code RSHIFT p_r_shift
  code '*' p_star
  code '2/' p_two_slash
  code 'UM*' p_u_m_star
  code 'UM/MOD' p_u_m_slash_mod
  code '=' p_equals
  code '<' p_less
  code '(STRING)' p_string
  code '(CSTRING)' p_c_string
  code MOVE p_move
  code EMIT p_emit
  code KEY p_key
  code SOURCE p_source
  code WORD p_word
  code REFILL p_refill
  code SAVE-INPUT p_save_input
  code RESTORE-INPUT p_restore_input
  code FILE-POSITION p_file_position
  code READ-LINE p_read_line
  code INCLUDED p_included
  code REQUIRED p_required
  code REQUIRE p_require
  code FIND p_find
  code '(QUOTATION)' p_quotation
  code '[:' p_start_quotation
  pImmediate
  code ';]' p_end_quotation
  pImmediate
  code BYE p_bye
  code ':' p_colon
  code ':NONAME' p_colon_no_name
  code ';' p_semicolon
  pImmediate
  code RECURSE p_recurse
  pImmediate
  code IMMEDIATE p_immediate
  code POSTPONE p_postpone
  pImmediate
  code 'COMPILE,' p_compile_comma
  code CREATE-NAME p_create_name
  code CREATE p_create
  code '(DOES)' p_do_does
  code 'DOES>' p_does
  pImmediate
  code EVALUATE p_evaluate
  code EXECUTE p_execute
  code DEBUG p_debug
  code '(ENVIRONMENT)' p_environment
  code '(SELF)' p_self
  code '(DICT)' p_dict
  code '(EMPTY-RETURN-STACK)' p_empty_rs
  code ALLOCATE p_allocate
  code FREE p_free
  code RESIZE p_resize
  code 'B@' p_b_fetch
  code 'B!' p_b_store
  code 'W@' p_w_fetch
  code 'W!' p_w_store
  code 'L@' p_l_fetch
  code 'L!' p_l_store
  code 'X@' p_x_fetch
  code 'X!' p_x_store
  code BIN p_bin
  code 'R/O' p_r_o
  code 'R/W' p_r_w
  code 'W/O' p_w_o
  code CREATE-FILE p_create_file
  code OPEN-FILE p_open_file
  code CLOSE-FILE p_close_file
  code FILE-SIZE p_file_size
  code REPOSITION-FILE p_reposition_file
  code FLUSH-FILE p_flush_file
  code RESIZE-FILE p_resize_file
  code DELETE-FILE p_delete_file
  code RENAME-FILE p_rename_file
  code FILE-STATUS p_file_status
  code READ-FILE p_read_file
  code WRITE-FILE p_write_file
  code WRITE-LINE p_write_line

  code '(FLIT)' p_f_lit
  code FALIGN p_f_align
  code FALIGNED p_f_aligned
  code FLITERAL p_f_literal
  pImmediate
  code FLOATS p_floats
  code 'FLOAT+' p_float_plus
  code SFALIGNED p_s_f_aligned
  code DFALIGNED p_d_f_aligned
  code SFLOATS p_s_floats
  code DFLOATS p_d_floats
  code FDEPTH p_f_depth
  code FDROP p_f_drop
  code FDUP p_f_dup
  code FOVER p_f_over
  code FROT p_f_rot
  code FSWAP p_f_swap
  code 'F<' p_f_less
  code 'F0<' p_f_zero_less
  code 'F0=' p_f_zero_equals
  code 'F@' p_f_fetch
  code 'F!' p_f_store
  code 'SF@' p_s_f_fetch
  code 'SF!' p_s_f_store
  code 'DF@' p_d_f_fetch
  code 'DF!' p_d_f_store
  code 'D>F' p_d_to_f
  code 'F>D' p_f_to_d
  code FABS p_f_abs
  code 'F+' p_f_plus
  code 'F-' p_f_minus
  code 'F*' p_f_star
  code 'F**' p_f_star_star
  code 'F/' p_f_slash
  code FLOOR p_floor
  code FMAX p_f_max
  code FMIN p_f_min
  code FNEGATE p_f_negate
  code FROUND p_f_round
  code 'F~' p_f_proximate
  code FATAN2 p_f_atan2
  code FSQRT p_f_sqrt
  code FLN p_f_ln
  code FSIN p_f_sine
  code FCOS p_f_cos
  code FSINCOS p_f_sincos
  code FTAN p_f_tan
  code FASIN p_f_asin
  code FACOS p_f_acos
  code FATAN p_f_atan
  code FEXP p_f_exp
  code FEXPM1 p_f_exp_m_one
  code FLOG p_f_log_ten
  code FLNP1 p_f_lnp1
  code FALOG p_f_alog
  code FSINH p_f_sinh
  code FCOSH p_f_cosh
  code FTANH p_f_tanh
  code FASINH p_f_asinh
  code FACOSH p_f_acosh
  code '>FLOAT' p_to_float
  code REPRESENT p_represent
  code 'F.' p_f_dot
  code 'FS.' p_f_s_dot
  code 'FE.' p_f_e_dot
  code 'F.S' p_f_dot_s
}

pImmediate() { get_latest; set_flag "$GLAT" "$IMMEDIATE"; }
p_bye() { exit 0; }

# --- Host interface ----------------------------------------------------

host_evaluate() {
  local s=$1
  local l=${#s} i
  for ((i = 0; i < l; i++)); do ord "${s:i:1}"; cs $(( SCRATCH + i )) "$ORDV"; done
  push "$SCRATCH"; push "$l"
  p_evaluate
}

repl() {
  local line
  while IFS= read -r line; do
    sp=0; THR=0
    host_evaluate "$line"
    if (( THR != 0 )); then printf '< %s >\n' "$THR"; fi
  done
}

# --- Kernel self-tests -------------------------------------------------

TPASS=0; TFAIL=0
ck() {
  local want=$1 msg=$2 got
  if (( sp > 0 )); then got=${S[sp-1]}; else got="<empty>"; fi
  if [ "$got" = "$want" ]; then ((TPASS++)); printf 'ok   %s\n' "$msg"
  else ((TFAIL++)); printf 'FAIL %s: got %s want %s (sp=%d thr=%d)\n' "$msg" "$got" "$want" "$sp" "$THR"; fi
  sp=0; THR=0
}
ev() { sp=0; fp=0; THR=0; host_evaluate "$1"; }
fck() {
  local want=$1 msg=$2 got=${F[0]:-}
  if [ "$got" = "$want" ]; then ((TPASS++)); echo "ok   $msg"
  else ((TFAIL++)); echo "FAIL $msg: got [$got] want [$want]"; fi
  fp=0; sp=0; THR=0
}

t_memory() {
  ev '64 allocate'
  local ma=${S[0]} mi=${S[1]}
  sp=0; THR=0
  if (( mi == 0 && ma > 0 )); then ((TPASS++)); echo "ok   allocate"
  else ((TFAIL++)); echo "FAIL allocate: ma=$ma mi=$mi"; fi
  ev "$ma 12345 swap !"
  ev "$ma @"
  ck 12345 "memory store/fetch"
  ev "$ma 200 swap c!"
  ev "$ma c@"
  ck 200 "memory byte"
  ev "$ma 30000 swap w!"
  ev "$ma w@"
  ck 30000 "memory word"
  ev "$ma free"
  ck 0 "free"
}

t_file() {
  local path="/tmp/sloth_ftest.$$" i data="hi there" plen dlen
  plen=${#path}; dlen=${#data}
  for ((i = 0; i < plen; i++)); do ord "${path:i:1}"; cs $(( SCRATCH + i )) "$ORDV"; done
  for ((i = 0; i < dlen; i++)); do ord "${data:i:1}"; cs $(( SCRATCH + 256 + i )) "$ORDV"; done
  sp=0; THR=0; push $SCRATCH; push $plen; push 3; p_create_file
  local fid=${S[0]}
  if (( S[1] == 0 && fid > 0 )); then ((TPASS++)); echo "ok   create-file"
  else ((TFAIL++)); echo "FAIL create-file: fid=$fid ior=${S[1]}"; fi
  sp=0; THR=0; push $(( SCRATCH + 256 )); push $dlen; push "$fid"; p_write_line
  ck 0 "write-line"
  sp=0; THR=0; push "$fid"; p_close_file
  ck 0 "close-file"
  sp=0; THR=0; push $SCRATCH; push $plen; push 1; p_open_file
  local fid2=${S[0]}
  if (( S[1] == 0 && fid2 > 0 )); then ((TPASS++)); echo "ok   open-file"
  else ((TFAIL++)); echo "FAIL open-file: fid=$fid2 ior=${S[1]}"; fi
  sp=0; THR=0; push $(( SCRATCH + 512 )); push 80; push "$fid2"; p_read_line
  local got
  mem_to_string $(( SCRATCH + 512 )) "${S[0]}"
  got=$MTS
  if [ "$got" = "$data" ]; then ((TPASS++)); echo "ok   read-line"
  else ((TFAIL++)); echo "FAIL read-line: got [$got] want [$data]"; fi
  sp=0; THR=0; push "$fid2"; p_close_file
  sp=0; THR=0
  rm -f -- "$path"
}

t_float() {
  ev '1.5 2.25 f+'
  fck 3.75 "float f+"
  ev '3.0 4.0 f*'
  fck 12 "float f*"
  ev '2.0 fsqrt'
  fck 1.4142135623730951 "float fsqrt"
  ev '1.5 fnegate'
  fck -1.5 "float fnegate"
  ev '64 allocate'
  local ma=${S[0]}
  sp=0; fp=0; THR=0
  ev "3.5 $ma f!"
  ev "$ma f@"
  fck 3.5 "float f!/f@"
  ev "$ma f@ 1.0 f+"
  fck 4.5 "float stored add"
  local i dat="3.14"
  for ((i = 0; i < 4; i++)); do ord "${dat:i:1}"; cs $(( SCRATCH + 600 + i )) "$ORDV"; done
  sp=0; fp=0; THR=0; push $(( SCRATCH + 600 )); push 4; p_to_float
  if (( S[0] == -1 )) && awk -v A="${F[0]:-x}" 'BEGIN{exit !(A==3.14)}' 2>/dev/null; then
    ((TPASS++)); echo "ok   >float"
  else ((TFAIL++)); echo "FAIL >float: flag=${S[0]} f=[${F[0]:-}]"; fi
  sp=0; fp=0; THR=0
}

run_tests() {
  bootstrap

  ev ': sq dup * ;'
  ev '9 sq'
  ck 81 "colon: 9 sq"

  ev ': inc 1 + ;'
  ev '41 inc'
  ck 42 "colon: 41 inc"

  ev ': a 1 ; : b a ; b'
  ck 1 "colon: nested"

  ev '$FF'
  ck 255 "hex literal"

  ev '%1010'
  ck 10 "binary literal"

  ev '#42'
  ck 42 "decimal literal"

  ev '10 3 um*'
  if (( sp == 2 && S[0] == 30 && S[1] == 0 )); then ((TPASS++)); echo "ok   um*"
  else ((TFAIL++)); echo "FAIL um*: sp=$sp lo=${S[0]:-} hi=${S[1]:-}"; fi
  sp=0; THR=0

  ev '-1 2/'
  ck -1 "arithmetic: -1 2/"

  ev ': boom -99 throw ;'
  local w x
  find_word boom; w=$FWORD
  get_xt "$w"; x=$GXT
  sp=0; THR=0; push "$x"; p_catch
  ck -99 "catch/throw"

  find_word SWAP; w=$FWORD
  get_xt "$w"
  sp=0; THR=0; push "$GXT"; p_catch
  ck -4 "underflow throws"

  ev ': mk create cells allot does> swap cells + ;'
  ev '4 mk arr'
  ev '42 1 arr !'
  ev '1 arr @'
  ck 42 "create/does>"

  t_memory
  t_file
  t_float

  printf '\n%d passed, %d failed\n' "$TPASS" "$TFAIL"
  (( TFAIL == 0 ))
}

# --- Main --------------------------------------------------------------

main() {
  local script_dir
  script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
  SLOTH_PATHS=("$script_dir/../../4th/" "4th/")
  if [ -n "${SLOTH_ROOT:-}" ]; then
    SLOTH_PATHS=("$SLOTH_ROOT/4th/" "$SLOTH_ROOT/" "${SLOTH_PATHS[@]}")
  fi

  if [ "${1:-}" = "--test" ]; then
    run_tests
    exit $?
  fi

  bootstrap
  host_include "ans.4th"
  if (( THR != 0 )); then
    printf 'Fatal: ans.4th could not be included (throw %s)\n' "$THR" >&2
    exit 1
  fi

  case "${1:-}" in
    "")
      if [ -t 0 ]; then
        repl
      else
        host_evaluate "$(</dev/stdin)"
        if (( THR != 0 )); then printf '< %s >\n' "$THR" >&2; exit 1; fi
      fi
      ;;
    *)
      host_include "$1"
      if (( THR != 0 )); then printf '< %s >\n' "$THR" >&2; exit 1; fi
      ;;
  esac
}

main "$@"
