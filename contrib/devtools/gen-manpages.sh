#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

UNEMPLOYCOIND=${UNEMPLOYCOIND:-$SRCDIR/unemploycoind}
RAVENCLI=${RAVENCLI:-$SRCDIR/unemploycoin-cli}
RAVENTX=${RAVENTX:-$SRCDIR/unemploycoin-tx}
RAVENQT=${RAVENQT:-$SRCDIR/qt/unemploycoin-qt}

[ ! -x $UNEMPLOYCOIND ] && echo "$UNEMPLOYCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
UNEMPVER=($($RAVENCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for unemploycoind if --version-string is not set,
# but has different outcomes for unemploycoin-qt and unemploycoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$UNEMPLOYCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $UNEMPLOYCOIND $RAVENCLI $RAVENTX $RAVENQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${UNEMPVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${UNEMPVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
