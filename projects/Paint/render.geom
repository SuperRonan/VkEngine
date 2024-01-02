#version 460

layout(points) in;
layout(line_strip, max_vertices=2) out;

layout(push_constant) uniform constants
{
    vec2 p0;
	vec2 p1;
    float time;
} _pc;

layout(location = 0) in int vid[1]; 

layout(location = 0) out float t;

void main()
{
    if(true)
    {
        
        {
            gl_Position = vec4(_pc.p0, 0, 1);
            t = 0;
            EmitVertex();
        }
        
        {
            gl_Position = vec4(_pc.p1, 0, 1);
            t = 1;
            EmitVertex();
        }
    }
    else
    {
        float time = _pc.time;
        const float ct = cos(time);
        const float st = sin(time);

        gl_Position = vec4(ct, st, 0, 1);
        t = 0;
        EmitVertex();

        gl_Position = vec4(-ct, -st, 0, 1);
        t = 1;
        EmitVertex();
    }
    
    EndPrimitive();
}