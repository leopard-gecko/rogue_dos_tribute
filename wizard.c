/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * @(#)wizard.c	4.30 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"

/*
 * whatis:
 *	What a certin object is
 */

void
whatis(int insist, int type)
{
    THING *obj;

    if (pack == NULL)
    {
	msg(JSTR("識別するものを何も持っていない。", "you don't have anything in your pack to identify"));
	return;
    }

    for (;;)
    {
	obj = get_item(JSTR("識別する持ちもの", "identify"), type);
	if (insist)
	{
	    if (n_objs == 0)
		return;
	    else if (obj == NULL)
		msg(JSTR("何かを識別しなければならない。", "you must identify something"));
	    else if (type && obj->o_type != type &&
	       !(type == R_OR_S && (obj->o_type == RING || obj->o_type == STICK)) )
		    msg(JSTR("識別できるのは%s。", "you must identify a %s"), type_name(type));
	    else
		break;
	}
	else
	    break;
    }

    if (obj == NULL)
	return;

    switch (obj->o_type)
    {
        case SCROLL:
	    set_know(obj, scr_info);
        when POTION:
	    set_know(obj, pot_info);
	when STICK:
	    set_know(obj, ws_info);
        when WEAPON:
	    obj->o_flags |= ISKNOW;
	    if (obj->o_enemy != 0)
		obj->o_flags |= ISREVEAL;
        when ARMOR:
	    obj->o_flags |= ISKNOW;
        when RING:
	    set_know(obj, ring_info);
    }
    msg("%s", inv_name(obj, FALSE));
}

/*
 * set_know:
 *	Set things up when we really know what a thing is
 */

void
set_know(THING *obj, struct obj_info *info)
{
    char **guess;

    info[obj->o_which].oi_know = TRUE;
    obj->o_flags |= ISKNOW;
    guess = &info[obj->o_which].oi_guess;
    if (*guess)
    {
	free(*guess);
	*guess = NULL;
    }
}

/*
 * type_name:
 *	Return a pointer to the name of the type
 */
const char *
type_name(int type)
{
    struct h_list *hp;
    struct h_list tlist[] = {
	{POTION, JSTR("水薬", "potion"),		FALSE},
	{SCROLL, JSTR("巻き物", "scroll"),		FALSE},
	{FOOD,	 JSTR("食糧", "food"),		FALSE},
	{R_OR_S, JSTR("指輪とつえ・魔法の棒", "ring, wand or staff"),	FALSE},
	{RING,	 JSTR("指輪", "ring"),		FALSE},
	{STICK,	 JSTR("つえまたは魔法の棒", "wand or staff"),	FALSE},
	{WEAPON, JSTR("武器", "weapon"),		FALSE},
	{ARMOR,	 JSTR("よろい", "suit of armor"),	FALSE},
    };

    for (hp = tlist; hp->h_ch; hp++)
	if (type == hp->h_ch)
	    return hp->h_desc;
    /* NOTREACHED */
    return(0);
}

#ifdef MASTER

static int
wizard_item_max(int type)
{
    switch (type)
    {
	case POTION: return MAXPOTIONS;
	case SCROLL: return MAXSCROLLS;
	case FOOD: return 2;
	case WEAPON: return MAXWEAPONS;
	case ARMOR: return MAXARMORS;
	case AMULET: return 1;
	case RING: return MAXRINGS;
	case STICK: return MAXSTICKS;
	case GOLD: return 1;
	default: return 0;
    }
}

static int
wizard_item_type_ok(int type)
{
    return wizard_item_max(type) > 0;
}

static int
wizard_readchar(void)
{
    int ch;

    jrogue_show_cursor(TRUE);
    ch = readchar();
    jrogue_restore_cursor();
    return ch;
}


/*
 * create_obj:
 *	wizard command for getting anything he wants
 */

void
create_obj(void)
{
    THING *obj;
    int ch, bless;
    int maxobj;

    obj = new_item();
    if (obj == NULL)
	return;
    msg(JSTR("アイテムの種類：", "type of item: "));
    obj->o_type = wizard_readchar();
    mpos = 0;

    if (!wizard_item_type_ok(obj->o_type))
    {
	msg(JSTR("その種類のアイテムは作れない。", "that is not a valid item type"));
	discard(obj);
	return;
    }

    maxobj = wizard_item_max(obj->o_type);
    obj->o_which = 0;
    if (obj->o_type != GOLD)
    {
	msg(JSTR("どの'%c'を作る？ (0-%c)", "which %c do you want? (0-%c)"),
	    obj->o_type, maxobj <= 10 ? '0' + maxobj - 1 : 'a' + maxobj - 11);
	ch = wizard_readchar();
	obj->o_which = (isdigit(ch) ? ch - '0' : ch - 'a' + 10);
	if (obj->o_which < 0 || obj->o_which >= maxobj)
	{
	    mpos = 0;
	    msg(JSTR("その番号のアイテムは作れない。", "that item number is out of range"));
	    discard(obj);
	    return;
	}
    }
    obj->o_group = 0;
    obj->o_count = 1;
    mpos = 0;
    if (obj->o_type == WEAPON || obj->o_type == ARMOR)
    {
	msg(JSTR("祝福は？ (+,-,n)", "blessing? (+,-,n)"));
	bless = wizard_readchar();
	mpos = 0;
	if (bless == '-')
	    obj->o_flags |= ISCURSED;
	if (obj->o_type == WEAPON)
	{
	    init_weapon(obj, obj->o_which);
	    if (bless == '-')
		obj->o_hplus -= rnd(3)+1;
	    if (bless == '+')
		obj->o_hplus += rnd(3)+1;
	}
	else
	{
	    obj->o_arm = a_class[obj->o_which];
	    if (bless == '-')
		obj->o_arm += rnd(3)+1;
	    if (bless == '+')
		obj->o_arm -= rnd(3)+1;
	}
    }
    else if (obj->o_type == RING)
	switch (obj->o_which)
	{
	    case R_PROTECT:
	    case R_ADDSTR:
	    case R_ADDHIT:
	    case R_ADDDAM:
		msg(JSTR("祝福は？ (+,-,n)", "blessing? (+,-,n)"));
		bless = wizard_readchar();
		mpos = 0;
		if (bless == '-')
		    obj->o_flags |= ISCURSED;
		obj->o_arm = (bless == '-' ? -1 : rnd(2) + 1);
	    when R_AGGR:
	    case R_TELEPORT:
		obj->o_flags |= ISCURSED;
	}
    else if (obj->o_type == STICK)
	fix_stick(obj);
    else if (obj->o_type == GOLD)
    {
	msg(JSTR("いくら？", "how much?"));
	jrogue_show_cursor(TRUE);
	get_num(&obj->o_goldval, stdscr);
	jrogue_restore_cursor();
    }
    add_pack(obj, FALSE);
}
#endif

/*
 * telport:
 *	Bamf the hero someplace else
 */

void
teleport(void)
{
    coord c;

    jrogue_mvaddch_map(hero.y, hero.x, floor_at());
    find_floor(NULL, &c, FALSE, TRUE);
    if (roomin(&c) != proom)
    {
	leave_room(&hero);
	hero = c;
	enter_room(&hero);
    }
    else
    {
	hero = c;
	look(TRUE);
    }
    jrogue_mvaddch_map(hero.y, hero.x, PLAYER);
    /*
     * turn off ISHELD in case teleportation was done while fighting
     * a Flytrap
     */
    if (on(player, ISHELD)) {
	player.t_flags &= ~ISHELD;
	f_restor();
    }
    no_move = 0;
    count = 0;
    running = FALSE;
    flush_type();
}

#ifdef MASTER
/*
 * passwd:
 *	See if user knows password
 */
int
passwd(void)
{
    char *sp;
    int c;
    static char buf[MAXSTR];

    msg(JSTR("魔法使いの合言葉は？", "wizard's Password:"));
    mpos = 0;
    sp = buf;
    jrogue_show_cursor(TRUE);
    while (jrogue_prepare_input_attr(stdscr),
           (c = readchar()) != '\n' && c != '\r' && c != ESCAPE)
	if (c == md_killchar())
	    sp = buf;
	else if (c == md_erasechar() && sp > buf)
	    sp--;
	else
	    *sp++ = (char) c;
    jrogue_restore_cursor();
    if (sp == buf)
	return FALSE;
    *sp = '\0';
    return (strcmp(PASSWD, md_crypt(buf, "mT")) == 0);
}

/*
 * show_map:
 *	Print out the map for the wizard
 */

void
show_map(void)
{
    int y, x, real;

    wclear(hw);
    jrogue_wset_normal_attr(hw);
    for (y = 1; y < NUMLINES - 1; y++)
	for (x = 0; x < NUMCOLS; x++)
	{
	    real = flat(y, x) & F_REAL;
	    if (!real)
		wstandout(hw);
	    wmove(hw, y, x);
	    waddch2(hw, chat(y, x));
	    if (!(real & F_REAL)) {
		wstandend(hw);
		jrogue_wset_normal_attr(hw);
	    }
	}
    show_win(JSTR("--続く（この階の地図）--", "---More (level map)---"));
}
#endif
