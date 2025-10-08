# NekoWM setup

## Setting up user and groups

NekoWM runs under regular user account. In order to do that it needs to be able
to access input devices and display.

To enable input devices:
```
sudo usermod -a -G input $USER
```

To enable SPI display:
```
sudo usermod -a -G gpio,spi $USER
```

To enable HDMI/VGA display:
```
sudo usermod -a -G video $USER
```

Where `$USER` is the username the NekoWM will run under.


## Setting up nekowm.conf

After that the configuration file may need to be created. In the case of SPI
display NekoWM needs to know which display it talks to:

The `/etc/nekowm.conf` looks like:
```
{
 "backend_opts": "display:$DISPLAY_MODEL",
 ...
}
```
Where `$DISPLAY_MODEL` is set to gfxprim display model.

Other options:

- "rotate" values "90", "180", "270"

- "font\_family" can be set to gfxprim compiled-in family font name
                 available fonts can be listed with `nekowm -f help`

- "theme" can be set to 'light' or 'dark'

## Booting into nekowm

To boot directly to NekoWM without need to login enable the `nekowm.service` as
a user with:

```
systemctl enable --user nekowm
sudo loginctl enable-linger user
```

Or you can enable the NekoWM login daemon with:
```
sudo systemctl enable nekowm-login.service
```

## Setting up keybindings

### Keys and description

| Key Name         | Default     | Description                                                                   |
|------------------|-------------|-------------------------------------------------------------------------------|
| "WM\_Mod"        | KeyLeftMeta | Key needed to be pressed so that folling keys take effect.                    |
| "App\_Quit"      | KeyQ        | Quits currently shown and focused application.                                |
| "WM\_Exit"       | KeyX        | Shuts down all running applications and exits NekoWM.                         |
| "WM\_Force"      | KeyF        | Forces an action, e.g. NekoWM stuck at shutdown.                              |
| "List\_Apps"     | KeyL        | Hides currently displayed application and shows list of running applications. |
| "Switch\_Focus"  | KeyTab      | Switches focus when screen is split into more views (windows).                |
| "View\_Left"     | KeyLeft     | Switches to a view (virtual screen) on the left side.                         |
| "View\_Right"    | KeyRight    | Switches to a view (virtual screen) on the right side.                        |
| "Rotate\_Screen" | KeyR        | Rotates screen by 90 degrees.                                                 |
| "Power\_Off"     | KeyP        | Starts poweroff sequence.                                                     |

Certain key combinations are hardcoded these are:

- "WM\_Mod" + F1..F12 that switches between views (virtual screens)

### Configuration file

Keybindings can be changed in `$HOME/.config/nekowm/keybindings.json` configuration file.

Example:
```
{
 "WM_Mod": "KeyLeftAlt",
 "View_Left": "KeyLeftBrace",
 "View_Right": "KeyRighttBrace"
}
```
