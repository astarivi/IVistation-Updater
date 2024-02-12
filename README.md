# IVistation Updater

`IVistation Updater` is a nxdk based, C written, simple update installer for the OG Xbox project `IVistation`.

It unpacks the adjacent `update.tar` file to the current directory, replacing everything in the process.
If it fails, the updater can replace the IVistation binary with a [LithiumX](https://github.com/Ryzee119/LithiumX)
binary for further recovery.

## Why?

Updates are an important part of any project. IVistation uses an external update installer instead of installing in
XBMC, this is to avoid file locks. Therefore, updates can replace any file without issue. (And extracting files
in C is way faster than in Python, although that may need some tuning).

This approach can potentially offer recovery if an update fails (by replacing with LithiumX), or is interrupted 
(by running the updater again).

## Contributions

IVistation Updater aims to be a single `.xbe` file with no external file dependencies. Any changes fitting of this
are welcome.

## License

[microtar](https://github.com/rxi/microtar), MIT License
[Monserrat](https://fonts.google.com/specimen/Montserrat/about), OFL (Open Font License)
[LithiumX](https://github.com/Ryzee119/LithiumX), MIT License, Distributed as binary
[nxdk](https://github.com/XboxDev/nxdk), multiple licenses
