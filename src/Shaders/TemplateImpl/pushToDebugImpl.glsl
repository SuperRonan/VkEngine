



#ifndef PTD_TEMPLATE_N
#error "PTD_TEMPLATE_N not defined!"
#endif 

#ifndef PTD_TEMPLATE_type
#error "PTD_TEMPLATE_N not defined!"
#endif

#define gvecN PTD_TEMPLATE_type ## PTD_TEMPLATE_N

Caret pushToDebug(const in gvecN v, Caret c, bool ln, vec4 ft_color, vec4 bg_color, vec2 glyph_size, uint flags) 
{ 
    for(int i=0; i<N; ++i)	
    { 
        c = pushToDebug(v[i], c, ln, debugDimColor(i), bg_color, glyph_size, flags);
    } 
    return c; 
}


