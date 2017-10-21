#version 450

in vec2 f_texcoord;
uniform sampler2D mytexture;
out vec4 FragColor;

void main(void) {
  vec2 flipped_texcoord = vec2(f_texcoord.x, 1.0 - f_texcoord.y);
  FragColor = texture(mytexture, flipped_texcoord);
}
