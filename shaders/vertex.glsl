#version 150

in vec2 position;
in vec2 textureCoordinates;
in vec4 color;
out vec4 colorForFS;
out vec2 textureCoordinatesForFS;

void main() {
	gl_Position = vec4(position, 0.0, 1.0);
	colorForFS = color;
	textureCoordinatesForFS = textureCoordinates;
}