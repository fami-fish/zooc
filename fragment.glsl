// Opengl version >= 1.3
#version 130 

out mediump vec4 color;         // resultant color of pixel

in mediump  vec2 texcoord;      // texture coordinates after zooming/panning

uniform sampler2D tex;          // texture (screenshot)
uniform vec2      cursorPos;    // cursor position in X11 coordinate space
uniform vec2      windowSize;   // X11 window size

uniform float flShadow;         // Percentage to dim around flashlight
uniform float flRadius;         // Radius of flashlight
uniform float cameraScale;      // Camera scaling

void main()
{
    // Opengl counts y differently, so we have to take the position from the 
    // bottom of the screen (windowSize.y - cursorPos.y).
    vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);

    float dist = distance(cursor, gl_FragCoord);

    // delta doesn't need to be resolution independant since we wont scale
    // the actual flashlight circle, just the texture behind it.
    float delta = fwidth(dist);

    // Anti aliasing as described in: https://rubendv.be/posts/fwidth/
    float alpha = smoothstep(flRadius * cameraScale - delta, flRadius * cameraScale, dist);

    color = mix(
        texture(tex, texcoord), vec4(0.0, 0.0, 0.0, 0.0), 
        min(alpha, flShadow)
    );
}
