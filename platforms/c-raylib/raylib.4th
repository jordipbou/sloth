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
	r@ color.a c!
	r@ color.b c!
	r@ color.g c!
	r> color.r c!
;

200 200 200 255 rl-define-color: lightgray
130 130 130 255 rl-define-color: gray
80 80 80 255 rl-define-color: drakgray
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

257 constant key-enter

1 constant gesture-tap
