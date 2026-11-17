/*
 * This file contains misc functions for dealing with armor
 * @(#)armor.c	4.14 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include "rogue.h"

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */
void
wear(void)
{
    THING *obj;
    char *sp;

    /*
     * Match DOS 1.48: report the armor already being worn before asking
     * the player to select another piece of armor.
     */
    if (cur_armor != NULL)
    {
	if (jrogue_lang_is_japanese)
	{
	    addmsg("他のよろいを着ている。");
	    if (!terse)
		addmsg("まずよろいを脱がなければならない。");
	    endmsg();
	}
	else
	    msg(terse
		? "you are already wearing armor."
		: "you are already wearing armor.  You'll have to take it off first.");
	after = FALSE;
	return;
    }
    if ((obj = get_item(JSTR("身につけるよろい", "wear"), ARMOR)) == NULL)
	return;
    if (obj->o_type != ARMOR)
    {
	msg(JSTR("それはよろいではない。", "you can't wear that"));
	return;
    }
    waste_time();
    obj->o_flags |= ISKNOW;
    sp = inv_name(obj, TRUE);
    cur_armor = obj;
    if (!terse)
	addmsg(JSTR("あなたは", "you are now "));
    msg(JSTR("%sを身につけた。", "wearing %s"), sp);
}

/*
 * take_off:
 *	Get the armor off of the players back
 */
void
take_off(void)
{
    THING *obj;

    if ((obj = cur_armor) == NULL)
    {
	after = FALSE;
	msg(JSTR("あなたはよろいを着ていない。", "you aren't wearing any armor"));
	return;
    }
    if (!dropcheck(cur_armor))
	return;
    cur_armor = NULL;
    if (jrogue_lang_is_japanese)
    {
	addmsg("今まで着ていた");
	msg("%c) %sを脱いだ。", pack_char(obj), inv_name(obj, TRUE));
    }
    else
	msg("you used to be wearing %c) %s", pack_char(obj), inv_name(obj, TRUE));
}

/*
 * waste_time:
 *	Do nothing but let other things happen
 */
void
waste_time(void)
{
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    do_daemons(AFTER);
    do_fuses(AFTER);
}
