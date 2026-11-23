/*
 * Routines to deal with the pack
 *
 * @(#)pack.c	4.40 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include "rogue.h"

static int pack_item_matches_type(const THING *obj, int type);
static int inventory_select(const THING *list, int type, const char *purpose);

/*
 * update_mdest:
 *      Called after picking up an object, before discarding it.
 *      If this was the object of something's desire, that monster will
 *      get mad and run at the hero
 */
void
update_mdest(THING *obj)
{
    register THING *mp;

    for (mp = mlist; mp != NULL; mp = next(mp))
        if (mp->t_dest == &obj->o_pos)
     mp->t_dest = &hero;
}

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */

void
add_pack(THING *obj, int silent)
{
    THING *op, *lp;
    int from_floor;
    int discarded = 0;

    from_floor = FALSE;
    if (obj == NULL)
    {
	if ((obj = find_obj(hero.y, hero.x)) == NULL)
	    return;
	from_floor = TRUE;
    }

    /*
     * Check for and deal with scare monster scrolls
     */
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	if (obj->o_flags & ISFOUND)
	{
	    detach(lvl_obj, obj);
	    jrogue_mvaddch_map(hero.y, hero.x, floor_ch());
	    chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
            update_mdest(obj);
            discarded = 1;
	    discard(obj);
	    msg(terse
		? JSTR("巻き物はちりになった。", "the scroll turns to dust.")
		: JSTR("拾いあげたとたん、巻き物はちりになってしまった。",
		       "the scroll turns to dust as you pick it up."));
	    return;
	}

    if (pack == NULL)
    {
	/*
	 * Even the first object must be detached from lvl_obj before it is
	 * made the pack head.  Otherwise its floor-list links are inherited
	 * by pack, making the remaining floor objects appear in inventory.
	 */
	if (!pack_room(from_floor, obj))
	    return;
	pack = obj;
    }
    else
    {
	lp = NULL;
	for (op = pack; op != NULL; op = next(op))
	{
	    if (op->o_type != obj->o_type)
		lp = op;
	    else
	    {
		while (op->o_type == obj->o_type && op->o_which != obj->o_which)
		{
		    lp = op;
		    if (next(op) == NULL)
			break;
		    else
			op = next(op);
		}
		if (op->o_type == obj->o_type && op->o_which == obj->o_which)
		{
		    if (ISMULT(op->o_type))
		    {
			if (!pack_room(from_floor, obj))
			    return;
			op->o_count++;
dump_it:
			update_mdest(obj);
			discard(obj);
			obj = op;
                        discarded = 1;
			lp = NULL;
			goto out;
		    }
		    else if (obj->o_group)
		    {
			lp = op;
			while (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group != obj->o_group)
			{
			    lp = op;
			    if (next(op) == NULL)
				break;
			    else
				op = next(op);
			}
			if (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group == obj->o_group)
			{
				op->o_count += obj->o_count;
				inpack--;
				if (!pack_room(from_floor, obj))
				    return;
				goto dump_it;
			}
		    }
		    else
			lp = op;
		}
out:
		break;
	    }
	}

	if (lp != NULL)
	{
	    if (!pack_room(from_floor, obj))
		return;
	    else
	    {
		next(obj) = next(lp);
		prev(obj) = lp;
		if (next(lp) != NULL)
		    prev(next(lp)) = obj;
		next(lp) = obj;
	    }
	}
    }

    obj->o_flags |= ISFOUND;

    /*
     * If this was the object of something's desire, that monster will
     * get mad and run at the hero.
     */
    if (!discarded)
        update_mdest(obj);

    if (obj->o_type == AMULET)
    {
	amulet = TRUE;
	saw_amulet = TRUE;
    }
    /*
     * Notify the user
     */
    if (!silent)
    {
if (jrogue_lang_is_japanese)
	{
	    if (!terse)
		addmsg("あなたは");
	    msg("%s (%c)を手に入れた。", inv_name(obj, !terse), pack_char(obj));
	}
	else
	{
	    if (!terse)
		addmsg("you now have ");
	    msg("%s (%c)", inv_name(obj, !terse), pack_char(obj));
	}
    }
}

/*
 * pack_room:
 *	See if there's room in the pack.  If not, print out an
 *	appropriate message
 */
int
pack_room(int from_floor, THING *obj)
{
    if (++inpack > MAXPACK)
    {
if (jrogue_lang_is_japanese)
	{
	    if (!terse)
		addmsg("もう");
	    addmsg("これ以上、物は持てない。");
	    endmsg();
	}
	else
	    msg("you can't carry anything else");
	if (from_floor)
	    move_msg(obj);
	inpack = MAXPACK;
	return FALSE;
    }

    if (from_floor)
    {
	detach(lvl_obj, obj);
	jrogue_mvaddch_map(hero.y, hero.x, floor_ch());
	chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    }

    return TRUE;
}

/*
 * leave_pack:
 *	take an item out of the pack
 */
THING *
leave_pack(THING *obj, int newobj, int all)
{
    THING *nobj;

    inpack--;
    nobj = obj;
    if (obj->o_count > 1 && !all)
    {
	last_pick = obj;
	obj->o_count--;
	if (obj->o_group)
	    inpack++;
	if (newobj)
	{
	    nobj = new_item();
	    if (nobj == NULL)
	    {
		obj->o_count++;
		if (obj->o_group)
		    inpack--;
		inpack++;
		return NULL;
	    }
	    *nobj = *obj;
	    next(nobj) = NULL;
	    prev(nobj) = NULL;
	    nobj->o_count = 1;
	}
    }
    else
    {
	last_pick = NULL;
	detach(pack, obj);
    }
    return nobj;
}

/*
 * pack_obj:
 *	Return the pack object addressed by display-order character ch.
 */
THING *
pack_obj(int ch, int *chp)
{
    THING *obj;
    int och;

    och = 'a';
    for (obj = pack; obj != NULL; obj = next(obj), och++)
	if (ch == och)
	    return obj;
    if (chp != NULL)
	*chp = och;
    return NULL;
}

/*
 * pack_char:
 *	Return which display-order character addresses a pack object.
 */
int
pack_char(const THING *obj)
{
    const THING *item;
    int c;

    c = 'a';
    for (item = pack; item != NULL; item = next(item), c++)
	if (item == obj)
	    return c;
    return '?';
}

/*
 * pack_item_matches_type:
 *	Return TRUE if this object should be listed for this type of action.
 */
static int
pack_item_matches_type(const THING *obj, int type)
{
    if (type == 0)
	return TRUE;
    if (type == CALLABLE)
	return (obj->o_type == POTION || obj->o_type == SCROLL
	    || obj->o_type == RING || obj->o_type == STICK);
    if (type == R_OR_S)
	return (obj->o_type == RING || obj->o_type == STICK);
    if (type == WEAPON && obj->o_type == POTION)
	return TRUE;
    if (type == STICK && obj->o_enemy != 0 && obj->o_charges > 0)
	return TRUE;
    return (type == obj->o_type);
}

/*
 * inventory:
 *	List what is in the pack.  Return TRUE if there is something of
 *	the given type.
 */
int
inventory(const THING *list, int type)
{
    static char inv_temp[MAXSTR];
    int ch;

    n_objs = 0;
    for (ch = 'a'; list != NULL; list = next(list), ch++)
    {
	if (!pack_item_matches_type(list, type))
	    continue;
	n_objs++;
	snprintf(inv_temp, sizeof inv_temp, "%c) %%s", ch);
	msg_esc = TRUE;
	if (add_line(inv_temp, inv_name(list, FALSE)) == ESCAPE)
	{
	    msg_esc = FALSE;
	    msg("");
	    return TRUE;
	}
	msg_esc = FALSE;
    }
    if (n_objs == 0)
    {
	if (terse)
	    msg(type == 0 ? JSTR("何も持っていない。", "empty handed") :
			    JSTR("それは１つも持っていない。", "nothing appropriate"));
	else
	    msg(type == 0 ? JSTR("何も持っていない。", "you are empty handed") :
			    JSTR("それは１つも持っていない。", "you don't have anything appropriate"));
	return FALSE;
    }
    end_line();
    return TRUE;
}

/*
 * inventory_select:
 *	Show an DOS-style selection inventory and return the selected pack
 *	letter.  ESCAPE cancels and space returns to the traditional prompt.
 */
static int
inventory_select(const THING *list, int type, const char *purpose)
{
    const THING *obj;
    unsigned char valid[256];
    char inv_temp[MAXSTR];
    char prompt[MAXSTR];
    int ch, row, pch;

    memset(valid, 0, sizeof valid);
    n_objs = 0;

    wclear(hw);
    jrogue_wset_normal_attr(hw);

    row = 0;
    for (obj = list, pch = 'a'; obj != NULL; obj = next(obj), pch++)
    {
	if (!pack_item_matches_type(obj, type))
	    continue;
	if (row >= LINES - 1)
	    break;
	n_objs++;
	valid[(unsigned char)pch] = TRUE;
	snprintf(inv_temp, sizeof inv_temp, "%c) %s", pch, inv_name(obj, FALSE));
	mvwprintw(hw, row++, 0, "%s", inv_temp);
    }

    if (n_objs == 0)
    {
	wclear(hw);
	touchwin(stdscr);
	if (terse)
	    msg(type == 0 ? JSTR("何も持っていない。", "empty handed") :
			    JSTR("それは１つも持っていない。", "nothing appropriate"));
	else
	    msg(type == 0 ? JSTR("何も持っていない。", "you are empty handed") :
			    JSTR("それは１つも持っていない。", "you don't have anything appropriate"));
	return 0;
    }

    snprintf(prompt, sizeof prompt, JSTR("-%sを選んでください。ESCでキャンセル-",
	"-Select item to %s. Esc to cancel-"), purpose);
    wmove(hw, LINES - 1, 0);
    waddstr(hw, prompt);
    wclrtoeol(hw);
    touchwin(hw);
    jrogue_refresh_window(hw);

    jrogue_show_cursor(TRUE);
    do
	ch = wreadchar(hw);
    while (ch != ESCAPE && ch != ' ' &&
	(ch < 0 || ch > 255 || !valid[(unsigned char)ch]));
    jrogue_restore_cursor();

    clearok(curscr, TRUE);
    wclear(hw);
    touchwin(stdscr);
    msg("");

    return ch;
}

/*
 * pick_up:
 *	Add something to characters pack.
 */

void
pick_up(int ch)
{
    THING *obj;

    obj = find_obj(hero.y, hero.x);
    if (move_on)
	move_msg(obj);
    else
	switch (ch)
	{
	    case GOLD:
		if (obj == NULL)
		    return;
		money(obj->o_goldval);
		detach(lvl_obj, obj);
		update_mdest(obj);
		discard(obj);
		proom->r_goldval = 0;
		break;
	    default:
#ifdef MASTER
		debug(JSTR("どこで'%s'なんかを拾ったの？？？", "Where did you pick a '%s' up???"), unctrl(ch));
#endif
	    case ARMOR:
	    case POTION:
	    case FOOD:
	    case WEAPON:
	    case SCROLL:	
	    case AMULET:
	    case RING:
	    case STICK:
		add_pack(NULL, FALSE);
		break;
	}
}

/*
 * move_msg:
 *	Print out the message if you are just moving onto an object
 */

void
move_msg(const THING *obj)
{
    if (!terse)
	addmsg(JSTR("あなたは", "you "));
    msg(JSTR("%sの上にいる。", "moved onto %s"), inv_name(obj, TRUE));
}

/*
 * picky_inven:
 *	Allow player to inventory a single item
 */

void
picky_inven(void)
{
    THING *obj;
    int mch, och;

    if (pack == NULL)
	msg(JSTR("何も持っていない。", "you aren't carrying anything"));
    else if (next(pack) == NULL)
	msg("a) %s", inv_name(pack, FALSE));
    else
    {
	msg(terse ? JSTR("調べる持ちものは？", "item: ") : JSTR("調べる持ちものの種類は？", "which item do you wish to inventory: "));
	mpos = 0;
	if ((mch = readchar()) == ESCAPE)
	{
	    msg("");
	    return;
	}
	if ((obj = pack_obj(mch, &och)) != NULL)
	{
	    msg("%c) %s", mch, inv_name(obj, FALSE));
	    return;
	}
	msg(JSTR("'%s'は持っていない。", "'%s' not in pack"), unctrl(mch));
    }
}

/*
 * menu_selects_first:
 *	Return TRUE when the DOS-style menu should be shown before the
 *	traditional prompt.
 */
static int
menu_selects_first(int type)
{
    if (strncmp(inventory_menu, "off", 3) == 0)
	return FALSE;
    if (strncmp(inventory_menu, "sel", 3) == 0)
	return (type != FOOD && type != 0);
    return TRUE;
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
THING *
get_item(const char *purpose, int type)
{
    THING *obj;
    int ch, och;
    int use_menu;

    if (pack == NULL)
	msg(JSTR("何も持っていない。", "you aren't carrying anything"));
    else if (again)
	if (last_pick)
	    return last_pick;
	else
	    msg(JSTR("使い果たした。", "you ran out"));
    else
    {
	use_menu = menu_selects_first(type);
	for (;;)
	{
	    if (use_menu)
	    {
		ch = inventory_select(pack, type, purpose);
		use_menu = FALSE;
		if (ch == 0)
		{
		    after = FALSE;
		    return NULL;
		}
		if (ch == ' ')
		    continue;
	    }
	    else
	    {
		/* The old fragment assembly appended " what" in English
		 * verbose mode and omitted the question mark.  Keep the
		 * input and menu paths unchanged; only compose the prompt. */
		if (jrogue_lang_is_japanese)
		    msg(terse
			? "%sは？ ('*'で一覧、ESCで中止) "
			: "%sはどれですか？ ('*'で一覧表示、ESCで中止) ",
			purpose);
		else
		    msg(terse
			? "%s? (* for list, ESC to cancel): "
			: "which object do you want to %s? (* for list, ESC to cancel): ",
			purpose);
		jrogue_show_cursor(TRUE);
		ch = readchar();
		mpos = 0;
		jrogue_restore_cursor();
		/*
		 * Give the poor player a chance to abort the command
		 */
		if (ch == ESCAPE)
		{
		    reset_last();
		    after = FALSE;
		    msg("");
		    return NULL;
		}
		n_objs = 1;		/* normal case: person types one char */
		if (ch == '*')
		{
		    ch = inventory_select(pack, type, purpose);
		    if (ch == 0)
		    {
			after = FALSE;
			return NULL;
		    }
		    if (ch == ESCAPE)
		    {
			reset_last();
			after = FALSE;
			msg("");
			return NULL;
		    }
		    if (ch == ' ')
			continue;
		}
	    }

	    /*
	     * Give the poor player a chance to abort the command after the
	     * menu path too.
	     */
	    if (ch == ESCAPE)
	    {
		reset_last();
		after = FALSE;
		msg("");
		return NULL;
	    }

	    if ((obj = pack_obj(ch, &och)) == NULL)
	    {
		if (och > 'a')
		    msg(JSTR("'a'から'%c'までで指定してください。", "please specify a letter between 'a' and '%c'"), och - 1);
		else
		    msg(JSTR("'%s' それはできない。", "'%s' is not a valid item"), unctrl(ch));
		continue;
	    }
	    else {
		msg("");
		return obj;
	    }
	}
    }
    jrogue_restore_cursor();
    return NULL;
}

/*
 * money:
 *	Add or subtract gold from the pack
 */

void
money(int value)
{
    purse += value;
    jrogue_mvaddch_map(hero.y, hero.x, floor_ch());
    chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    if (value > 0)
    {
    if (!terse)
	    addmsg(JSTR("あなたは", "you found "));
	msg(JSTR("%d個の金塊を見つけた。", "%d gold pieces"), value);
    }
}

/*
 * floor_ch:
 *	Return the appropriate floor character for her room
 */
int
floor_ch(void)
{
    if (proom->r_flags & ISGONE)
	return PASSAGE;
    return (show_floor() ? FLOOR : ' ');
}

/*
 * floor_at:
 *	Return the character at hero's position.
 */
int
floor_at(void)
{
    int ch;

    ch = chat(hero.y, hero.x);
    if (ch == FLOOR)
	ch = floor_ch();
    return ch;
}

/*
 * reset_last:
 *	Reset the last command when the current one is aborted
 */

void
reset_last(void)
{
    last_comm = l_last_comm;
    last_dir = l_last_dir;
    last_pick = l_last_pick;
}
