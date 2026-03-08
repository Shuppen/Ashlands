#ifdef GL_ES
precision mediump float;
precision mediump int;
#endif

uniform sampler2D u_texture;
uniform vec3 u_ambient_color;
uniform vec3 u_light_dir;
uniform vec3 u_light_color;
uniform vec3 u_fog_color;
uniform vec3 u_camera_pos;
uniform float u_fog_density;

varying vec2 v_texcoord;
varying vec3 v_normal;
varying vec3 v_world_pos;
varying vec4 v_color;

void main() {
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(-u_light_dir);
    float diffuse = max(dot(normal, light_dir), 0.0);
    vec4 texel = texture2D(u_texture, v_texcoord) * v_color;
    vec3 lit = texel.rgb * (u_ambient_color + u_light_color * diffuse);
    float distance_to_camera = length(v_world_pos - u_camera_pos);
    float fog = clamp(exp(-u_fog_density * distance_to_camera), 0.0, 1.0);
    vec3 final_rgb = mix(u_fog_color, lit, fog);
    gl_FragColor = vec4(final_rgb, texel.a);
}
