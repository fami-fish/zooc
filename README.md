# Zooc
A C rewrite of tsoding's boomer with some added features

## Controls

| Control                                   | Description                                                   |
|-------------------------------------------|---------------------------------------------------------------|
| <kbd>0</kbd>                              | Reset the application state (position, scale, velocity, etc). |
| <kbd>q</kbd> or <kbd>ESC</kbd>            | Quit the application.                                         |
| <kbd>r</kbd>                              | Reload configuration.                                         |
| <kbd>Ctrl</kbd> + <kbd>r</kbd>            | Reload the shaders (only for Developer mode)                  |
| <kbd>f</kbd>                              | Toggle flashlight effect.                                     |
| Drag with left mouse button               | Move the image around.                                        |
| <kbd>hjkl + arrow keys</kbd>              | Move the image around with the keyboard.                      |
| Scroll wheel or <kbd>=</kbd>/<kbd>-</kbd> | Zoom in/out.                                                  |
| <kbd>Ctrl</kbd> + Scroll wheel            | Change the radius of the flaslight.                           |

## Configuration
The configuration file is located at `$XDG_CONFIG_HOME/zooc/config.conf`. 
It follows this format:

```
<param-1> = <value-1>
<param-2> = <value-2>
```
However the `=` is optional and can be omitted:
```
<param-1> <value-1>
<param-2> <value-2>
```

Values can be of float or boolean type. Floats are parsed with `strtof`, which
allows follows the format described [here](https://cplusplus.com/reference/cstdlib/strtof/).
Boolean types (case insensitive) are parsed as such:

`t`,`true`,`1`
`f`,`false`,`0`

## Building
deps: `glew`

```sh
make zooc clean
```
