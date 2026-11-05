# bspwm

**bspwm** is a *tiling window manager* for Linux. Instead of letting windows pile up on top of each other, it arranges them side by side so they fill the screen without overlapping. You control everything from the keyboard.

This is a fork of [baskerville/bspwm](https://github.com/baskerville/bspwm) with a few extra features and fixes. It works on **X11 only** (the experimental Wayland support has been removed).

## What you get

- **Tiling:** open a window and the screen is split automatically. Close one and the rest grow to fill the space.
- **Keyboard control:** a small tool called `bspc` tells bspwm what to do (move a window, switch desktop, and so on). Pair it with `sxhkd` to bind those commands to keys.
- **Fully scriptable:** every action is a `bspc` command, so you can put it in scripts.

## What this fork adds

| Feature | What it does |
| --- | --- |
| **Edge snapping** | Drag a window to a screen edge or corner and it snaps into place, like on Windows. Turn it on with `bspc config edge_snap_enabled true`. |
| **Magnetic edges** | Floating windows stick to screen edges and to other windows as you drag them. Adjust with `bspc config magnet_threshold 20`. |
| **Tile limit** | Cap how many tiled windows one desktop can hold (`tile_limit_enabled`, `max_tiles_per_desktop`). |
| **Smarter window rules** | Match windows by name, class or other properties, including patterns. For example, make every Firefox-like window float: `bspc rule -a 'class~=^Fire/i' state=floating` |
| **Freeze monitor layout** | Stop bspwm reacting when monitors are plugged in or out (`ignore_monitor_updates`). |

## What this fork fixes

Safety and reliability problems in the original:

- **Restarting no longer risks a crash.** Very long window or desktop names used to overflow a buffer when bspwm reloaded its saved state.
- **A stuck program can't freeze bspwm.** If something connects to bspwm and then says nothing, it is dropped after 2 seconds instead of blocking everything.
- **Only you can control your bspwm.** The control socket is private to your user account.
- **Bad input is rejected.** A typo like `window_gap 10abc` or `border_width -1` now gives an error instead of being half-understood.
- **Fewer round-trips to the display server.** On your own computer you will not notice a difference. It helps only when running over a slow remote connection.

## Install

### 1. Get the dependencies

You need these libraries (and their development headers):

`libxcb`, `xcb-util`, `xcb-util-keysyms`, `xcb-util-wm`, `libxkbcommon`

You also need a recent C compiler (GCC 13 or newer, or clang 18 or newer).

### 2. Build and install

```bash
make
sudo make install
```

To remove it later: `sudo make uninstall`.

On Arch Linux, a package recipe is included at `pkg/arch/PKGBUILD`.

## Getting started

1. **Copy the example settings** into your home folder:

   ```bash
   make install_cfg
   ```

   This puts a starter `bspwmrc` in `~/.config/bspwm/` and a starter `sxhkdrc` (your key bindings) in `~/.config/sxhkd/`.

2. **Start bspwm.** Either pick **bspwm** from your login screen, or, if you start X by hand, put this line at the end of `~/.xinitrc`:

   ```bash
   exec bspwm
   ```

   The starter `bspwmrc` launches `sxhkd` (the key-binding program) for you, so you don't need to start it yourself.

3. **Use it.** The key bindings live in `~/.config/sxhkd/sxhkdrc`, so open that file to see (and change) them.

### Handy commands

```bash
bspc node -c                          # close the focused window
bspc node -t floating                 # make the focused window float
bspc config window_gap 12             # set the gap between windows
bspc wm -r                            # restart bspwm
bspc quit                             # log out of bspwm
```

## Configuration

| What | Where |
| --- | --- |
| Main settings (run at startup) | `~/.config/bspwm/bspwmrc` |
| Key bindings | `~/.config/sxhkd/sxhkdrc` |
| Ready-made examples | the `examples/` folder |
| Full documentation | `man bspwm` and `man bspc` |

If `XDG_CONFIG_HOME` is set, use that folder instead of `~/.config`.

## Help and development

- To see how it all works, read the manual pages: `man bspwm` and `man bspc`.
- Developers: tests live in `tests/` (`make test`), and speed comparisons live in `benches/`. See `doc/CONTRIBUTING.md` before sending changes.
- Version history: `doc/CHANGELOG.md`.

## License

BSD-2-Clause. See the `LICENSE` file.
