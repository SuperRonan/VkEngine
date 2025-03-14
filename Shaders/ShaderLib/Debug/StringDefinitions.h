#pragma once

#ifndef I_WANT_TO_DEBUG
#define I_WANT_TO_DEBUG 0
#endif

#ifndef GLOBAL_ENABLE_SHADER_DEBUG
#define GLOBAL_ENABLE_SHADER_DEBUG 0
#endif


#ifndef SHADER_STRING_CAPACITY
#define SHADER_STRING_CAPACITY 32
#endif

#if ((SHADER_STRING_CAPACITY % 4) != 0)
#error "SHADER_STRING_CAPACITY must be a multiple of 4"
#endif

#define SHADER_STRING_PACKED_CAPACITY (SHADER_STRING_CAPACITY / 4)

#ifndef ENABLE_SHADER_STRING
#define ENABLE_SHADER_STRING (GLOBAL_ENABLE_SHADER_DEBUG && I_WANT_TO_DEBUG)
#endif

#ifndef HEX_USE_CAPS
#define HEX_USE_CAPS 0
#endif

#ifndef DEFAULT_NUMBER_BASIS
#define DEFAULT_NUMBER_BASIS 10
#endif

#ifndef DEFAULT_SHOW_PLUS
#define DEFAULT_SHOW_PLUS false
#endif

#ifndef DEFAULT_FLOAT_PRECISION
#define DEFAULT_FLOAT_PRECISION 3
#endif


#define CHARCODE_space 		0x20 // ` `
#define CHARCODE_lpar 		0x28 // `(`
#define CHARCODE_rpar 		0x29 // `)`
#define CHARCODE_plus 		0x2B // `+`
#define CHARCODE_comma		0x2C // `,`
#define CHARCODE_minus 		0x2D // `-`
#define CHARCODE_dot 		0x2E // `.`
#define CHARCODE_0 			0x30 // `0`
#define CHARCODE_A 			0x41 // `A`
#define CHARCODE_at 		0x40 // `@`
#define CHARCODE_a 			0x61 // `a`
#define CHARCODE_x			0x78 // `x`
#define CHARCODE_lbracket	0x5B // `[`
#define CHARCODE_rbracket	0x5D // `]`
#define CHARCODE_lbrace		0x7B // `{`
#define CHARCODE_rbrace		0x7D // `}`
#define CHARCODE_e			0x65 // `e`
#define CHARCODE_lchevron	0x3C // `<`
#define CHARCODE_rchevron	0x3E // `>`

#if HEX_USE_CAPS
#define CHARCODE_hex_base CHARCODE_A
#else
#define CHARCODE_hex_base CHARCODE_a
#endif

