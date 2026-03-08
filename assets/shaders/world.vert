attribute vec3 a_position;
attribute vec2 a_texcoord;
attribute vec3 a_normal;
attribute vec4 a_color;

uniform mat4 u_mvp;
uniform mat4 u_model;

varying vec2 v_texcoord;
varying vec3 v_normal;
varying vec3 v_world_pos;
varying vec4 v_color;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
    v_normal = normalize((u_model * vec4(a_normal, 0.0)).xyz);
    v_world_pos = world_pos.xyz;
    v_color = a_color;
}
