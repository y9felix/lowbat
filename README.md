# lowbat

Lightweight low battery layer-shell notification overlay for Wayland.

`lowbat` monitors sysfs battery power supply (`/sys/class/power_supply/BAT0`) and displays an unobtrusive or critical centered warning overlay when capacity drops below configured thresholds.

Features:
- Wayland layer-shell overlay (works over fullscreen applications)
- Multiple alert thresholds (low warning and critical)
- Configurable colors, scaling, and dimensions
- Minimal polling overhead using sysfs

## Requirements

- `wayland-client`
- `wayland-scanner`
- C99 compiler (`cc`)
- GNU Make

## Installation

```sh
make
sudo make install
```

## Configuration

Settings and battery paths are defined in `config.h`:

```sh
cp config.def.h config.h
# edit config.h
make
```

## License

MIT License.
