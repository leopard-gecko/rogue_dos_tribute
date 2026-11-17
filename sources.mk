# Shared source manifest for Rogue DOS Tribute.
# Keep portable game sources in one list: macOS/Linux and Windows must build
# the same game core.  Only native console and file services differ.
ROGUE_COMMON_CFILES = jec/color.c jec/unicode_text.c vers.c extern.c armor.c chase.c command.c daemon.c \
    daemons.c fight.c init.c io.c list.c jec/language.c mach_dep.c \
    main.c mdport.c misc.c monsters.c move.c new_level.c \
    options.c pack.c passages.c potions.c rings.c rip.c \
    rooms.c save.c scrolls.c state.c sticks.c slime.c things.c \
    weapons.c wizard.c xcrypt.c

ROGUE_WINDOWS_CFILES = windows/rogue_windows.c windows/rogue_windows_console.c
ROGUE_COMMON_HEADERS = rogue.h extern.h score.h config.h \
    jec/color.h jec/language.h jec/unicode_text.h
ROGUE_WINDOWS_HEADERS = windows/rogue_windows.h windows/rogue_windows_console.h
