begin-structure color
	\ Using cfield is not correct on non 8 byte/char platforms
	cfield: color.r
	cfield: color.g
	cfield: color.b
	cfield: color.a
end-structure

: rl-define-color: ( u1 u2 u3 u4 -- )
	here color allot dup >r constant
	\ c! is not correct on non 8 byte/char platforms
	\ should be b!
	r@ color.a c!
	r@ color.b c!
	r@ color.g c!
	r> color.r c!
;

200 200 200 255 rl-define-color: lightgray
130 130 130 255 rl-define-color: gray
80 80 80 255 rl-define-color: darkgray
253 249 0 255 rl-define-color: yellow
255 203 0 255 rl-define-color: gold
255 161 0 255 rl-define-color: orange
255 109 194 255 rl-define-color: pink
230 41 55 255 rl-define-color: red
190 33 55 255 rl-define-color: maroon
0 228 48 255 rl-define-color: green
0 158 47 255 rl-define-color: lime
0 117 44 255 rl-define-color: darkgreen
102 191 255 255 rl-define-color: skyblue
0 121 241 255 rl-define-color: blue
0 82 172 255 rl-define-color: darkblue
200 122 255 255 rl-define-color: purple
135 60 190 255 rl-define-color: violet
112 31 126 255 rl-define-color: darkpurple
211 176 131 255 rl-define-color: beige
127 106 79 255 rl-define-color: brown
76 63 47 255 rl-define-color: darkbrown

255 255 255 255 rl-define-color: white
0 0 0 255 rl-define-color: black
0 0 0 0 rl-define-color: blank 
255 0 255 255 rl-define-color: magenta
245 245 245 255 rl-define-color: raywhite

\ Rectangle

begin-structure rectangle
	sffield: rectangle.x
	sffield: rectangle.y
	sffield: rectangle.width
	sffield: rectangle.height
end-structure

\ Vector2, 2 components

begin-structure vector2
	sffield: vector2.x
	sffield: vector2.y
end-structure

\ Camera2D, defines position/orientation in 2d space

begin-structure Camera2d
	sffield: camera2d.offset.x
	sffield: camera2d.offset.y
	sffield: camera2d.target.x
	sffield: camera2d.target.y
	sffield: camera2d.rotation
	sffield: camera2d.zoom
end-structure

\ Texture, tex data stored in GPU memory (VRAM)

begin-structure texture
	intfield: texture.id
	intfield: texture.width
	intfield: texture.height
	intfield: texture.mipmaps
	intfield: texture.format
end-structure

\ TODO Texture2D is an alias of Texture

\ Other constants

\ Alphanumeric keys
49 constant key-one
50 constant key-two
65 constant key-a
82 constant key-r
83 constant key-s

\ Function keys
257 constant key-enter
262 constant key-right
263 constant key-left

1 constant gesture-tap

\ Mouse buttons
0 constant mouse-button-left
1 constant mouse-button-right
2 constant mouse-button-middle

\ Words that return a structure and need a memory
\ buffer for it may use transient memory.

[UNDEFINED] THERE [IF]
here unused + value THERE
[THEN]

[UNDEFINED] Tallot [IF]
: Tallot ( n -- )
	there here - over < if -8 throw then
	there swap - to there
;
[THEN]

[UNDEFINED] Tmarker [IF]
variable Tmarker
[THEN]

[UNDEFINED] Tmark [IF]
\ Compilation only
: tmark ( -- )
	r> tmarker @ >r >r
	there tmarker !
;
[THEN]

[UNDEFINED] Tfree [IF]
\ Compilation only
: tfree ( -- )
	tmarker @ to there
	r> r> tmarker ! >r
;
[THEN]

: Tget-screen-to-world-2d ( vector2 camera2d -- vector2 )
	Vector2 Tallot there get-screen-to-world-2d there
;

: Tget-mouse-position ( -- vector2 )
	Vector2 Tallot there get-mouse-position there
;

: Tget-mouse-delta ( -- vector2 )
	Vector2 Tallot there get-mouse-delta there
;

: Tfade ( Color r -- Color ) ( F: r -- )
	Color Tallot there fade there
;

: Tget-font-default ( -- font )
	\ I have not defined Font structure yet,
	\ store enough space for anything there.
	\ Font tallot there get-font-default there
	1024 tallot there get-font-default there
;

: Tvector2add ( vector2 vector2 -- vector2 )
	Vector2 Tallot there vector2add there
;

: Tvector2scale ( vector2 -- vector2 ) ( F: r -- )
	Vector2 Tallot there vector2scale there
;

: Ttext-format-2 ( c-addr n n n c-addr -- c-addr l )
	1024 tallot there text-format-2 there swap
;
