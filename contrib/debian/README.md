
Debian
====================
This directory contains files used to package unemploycoind/unemploycoin-qt
for Debian-based Linux systems. If you compile unemploycoind/unemploycoin-qt yourself, there are some useful files here.

## unemploycoin: URI support ##


unemploycoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install unemploycoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your unemploycoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/raven128.png` to `/usr/share/pixmaps`

unemploycoin-qt.protocol (KDE)

