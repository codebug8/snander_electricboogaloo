# snander electric boogaloo

This is a fork of SNANDer by McMMC. Mainly for use with the MStar DDC
protocol that is needed to flash MStar/SigmaStar SoC based systems
over the i2c ISP interface.

I've also done the following: 
- Replaced the makefile with meson
- Fixed a few annoying bugs
- Replaced reading the files into memory with mmap()
  This allows writing or reading files that are bigger than your memory.
- Broken stuff I'm not using
- Refactored how almost all of the data flow works so the different
  bits are abstracted.

## How to build

Only linux is supported right now. Sorry

```
cd src/
meson setup builddir
cd builddir
meson compile
```

If you want to enable `mstar_ddc`:

```
cd src/
meson configure -Dmstar_ddc=true
```

### Interfaces

For mstar ddc anything that Linux can use an i2c master.
USB i2c masters:
  - https://github.com/harbaum/I2C-Tiny-USB
  - https://github.com/Nicolai-Electronics/rp2040-i2c-interface

### How to use

Check the built in help.
