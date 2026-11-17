/*
 * Functions for dealing with problems brought about by weapons
 *
 * @(#)weapons.c	4.34 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"

#define NO_WEAPON -1

static const struct init_weaps {
    char *iw_dam;	/* Damage when wielded */
    char *iw_hrl;	/* Damage when thrown */
    int iw_launch;	/* Launching weapon */
    int iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
    { "2x4",	"1x3",	NO_WEAPON,	0,		},	/* Mace */
    { "3x4",	"1x2",	NO_WEAPON,	0,		},	/* Long sword */
    { "1x1",	"1x1",	NO_WEAPON,	0,		},	/* Bow */
    { "1x1",	"2x3",	BOW,		ISMANY|ISMISL,	},	/* Arrow */
    { "1x6",	"1x4",	NO_WEAPON,	ISMISL,		},	/* Dagger */
    { "4x4",	"1x2",	NO_WEAPON,	0,		},	/* 2h sword */
    { "1x1",	"1x3",	NO_WEAPON,	ISMANY|ISMISL,	},	/* Dart */
    { "1x1",	"1x1",	NO_WEAPON,	0,		},	/* Crossbow */
    { "1x2",	"2x5",	CROSSBOW,	ISMANY|ISMISL,	},	/* Crossbow bolt */
    { "2x3",	"1x6",	NO_WEAPON,	ISMISL,		},	/* Spear */
};

/*
 * missile:
 *	Fire a missile in a given direction
 */

void
missile(int ydelta, int xdelta)
{
    THING *obj;

    /*
     * Get which thing we are hurling
     */
    if ((obj = get_item(JSTR("投げる武器", "throw"), WEAPON)) == NULL)
	return;
    if (!dropcheck(obj) || is_current(obj))
	return;
    obj = leave_pack(obj, TRUE, FALSE);
    if (obj == NULL)
	return;
    do_motion(obj, ydelta, xdelta);
    /*
     * AHA! Here it has hit something.  If it is a wall or a door,
     * or if it misses (combat) the monster, put it on the floor
     */
    if (moat(obj->o_pos.y, obj->o_pos.x) == NULL)
	fall(obj, TRUE);
    else if (hit_monster(unc(obj->o_pos), obj))
    {
	if (obj->o_type == POTION)
	    discard(obj);
    }
    else
	fall(obj, TRUE);
}

/*
 * do_motion:
 *	Do the actual motion on the screen done by an object traveling
 *	across the room
 */

void
do_motion(THING *obj, int ydelta, int xdelta)
{
    int ch;

    /*
     * Come fly with us ...
     */
    obj->o_pos = hero;
    for (;;)
    {
	/*
	 * Erase the old one
	 */
	if (!ce(obj->o_pos, hero) && cansee(unc(obj->o_pos)))
		{
	    ch = chat(obj->o_pos.y, obj->o_pos.x);
	    if (ch == FLOOR && !show_floor())
		ch = ' ';
	    jrogue_mvaddch_map(obj->o_pos.y, obj->o_pos.x, ch);
	    }
	/*
	 * Get the new position
	 */
	obj->o_pos.y += ydelta;
	obj->o_pos.x += xdelta;
	if (step_ok(ch = winat(obj->o_pos.y, obj->o_pos.x)) && ch != DOOR)
	{
	    /*
	     * It hasn't hit anything yet, so display it
	     * If it alright.
	     */
	    if (cansee(unc(obj->o_pos)))
	    {
		if (obj->o_type == '*' && (obj->o_flags & ISMISL))
		    mvaddch_projectile(obj->o_pos.y, obj->o_pos.x, obj->o_type,
			JROGUE_PAIR_PROJECTILE_CYAN);
		else
		    jrogue_mvaddch_map(obj->o_pos.y, obj->o_pos.x, obj->o_type);
		msleep(24);
		jrogue_restore_cursor();
		refresh();
	    }
	    continue;
	}
	break;
    }
}

/*
 * fall:
 *	Drop an item someplace around here.
 */

void
fall(THING *obj, int pr)
{
    PLACE *pp;
    coord fpos;

    switch (fallpos(obj, &fpos))
    {
    case 1:
	pp = INDEX(fpos.y, fpos.x);
	pp->p_ch = obj->o_type;
	obj->o_pos = fpos;
	if (cansee(fpos.y, fpos.x))
	{
	    if (pp->p_monst != NULL)
		pp->p_monst->t_oldch = obj->o_type;
	    else
		jrogue_mvaddch_map(fpos.y, fpos.x, obj->o_type);
	}
	attach(lvl_obj, obj);
	return;
    case 2:
	discard(obj);
	return;
    }
    if (pr)
    {
	if (has_hit)
	{
	    endmsg();
	    has_hit = FALSE;
	}
	if (jrogue_lang_is_japanese)
	    msg(terse ? "%sは消えてしまった。" :
		"%sは地面に落ちると、どこかに消えてしまった。", inv_name(obj, TRUE));
	else
	    /* inv_name() supplies the article ("a", "some", etc.). */
	    msg(terse ? "%s vanishes." :
		"%s vanishes as it hits the ground.", inv_name(obj, FALSE));
    }
    discard(obj);
}

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon
 */

void
init_weapon(THING *weap, int which)
{
    const struct init_weaps *iwp;

    weap->o_type = WEAPON;
    weap->o_which = which;
    iwp = &init_dam[which];
    jrogue_strlcpy(weap->o_damage, iwp->iw_dam, sizeof weap->o_damage);
    jrogue_strlcpy(weap->o_hurldmg, iwp->iw_hrl, sizeof weap->o_hurldmg);
    weap->o_launch = iwp->iw_launch;
    weap->o_flags = iwp->iw_flags;
    weap->o_hplus = 0;
    weap->o_dplus = 0;
    if (weap->o_flags & ISMANY)
    {
	weap->o_count = rnd(8) + 8;
	weap->o_group = group++;
    }
    else
    {
	weap->o_count = 1;
	weap->o_group = 0;
    }
}

/*
 * hit_monster:
 *	Does the missile hit the monster?
 */
int
hit_monster(int y, int x, THING *obj)
{
    coord mp;

    mp.y = y;
    mp.x = x;
    return fight(&mp, obj, TRUE);
}

/*
 * num:
 *	Figure out the plus number for armor/weapons
 */
const char *
num(int n1, int n2, int type)
{
    static char numbuf[64];

    snprintf(numbuf, sizeof numbuf, n1 < 0 ? "%d" : "+%d", n1);
    if (type == WEAPON)
	snprintf(numbuf + strlen(numbuf), sizeof numbuf - strlen(numbuf), n2 < 0 ? ",%d" : ",+%d", n2);
    return numbuf;
}

/*
 * wield:
 *	Pull out a certain weapon
 */

void
wield(void)
{
    THING *obj, *oweapon;
    char *sp;

    oweapon = cur_weapon;
    if (!dropcheck(cur_weapon))
    {
	cur_weapon = oweapon;
	return;
    }
    cur_weapon = oweapon;
    if ((obj = get_item(JSTR("装備する武器", "wield"), WEAPON)) == NULL)
    {
bad:
	after = FALSE;
	return;
    }

    if (obj->o_type == ARMOR)
    {
	msg(JSTR("よろいを武器にすることはできない。", "you can't wield armor"));
	goto bad;
    }
    if (is_current(obj))
        goto bad;

    sp = inv_name(obj, TRUE);
    cur_weapon = obj;
    if (jrogue_lang_is_japanese)
    {
	if (!terse)
	    addmsg("武器を");
	msg("%s (%c)に持ちかえた。", sp, pack_char(obj));
    }
    else if (terse)
	msg("now wielding %s (%c)", sp, pack_char(obj));
    else
	msg("you are now wielding %s (%c)", sp, pack_char(obj));
}

/*
 * fallpos:
 *	Pick a random position around the give (y, x) coordinates
 */
static THING *
fall_obj_at(int y, int x)
{
    THING *op;

    for (op = lvl_obj; op != NULL; op = next(op))
	if (op->o_pos.y == y && op->o_pos.x == x)
	    return op;
    return NULL;
}

int
fallpos(THING *obj, coord *newpos)
{
    int y, x, cnt, ch;
    THING *onfloor;

    cnt = 0;
    for (y = obj->o_pos.y - 1; y <= obj->o_pos.y + 1; y++)
	for (x = obj->o_pos.x - 1; x <= obj->o_pos.x + 1; x++)
	{
	    /*
	     * Check to make certain the spot is empty.  The DOS 1.48 code also
	     * combines thrown grouped missiles with the same group already
	     * lying on the floor.
	     */
	    if ((y == hero.y && x == hero.x) ||
		y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
		continue;
	    ch = chat(y, x);
	    if (ch == FLOOR || ch == PASSAGE)
	    {
		if (rnd(++cnt) == 0)
		{
		    newpos->y = y;
		    newpos->x = x;
		}
		continue;
	    }
	    if (step_ok(ch) && (onfloor = fall_obj_at(y, x)) != NULL &&
		onfloor->o_type == obj->o_type && onfloor->o_group != 0 &&
		onfloor->o_group == obj->o_group)
	    {
		onfloor->o_count += obj->o_count;
		return 2;
	    }
	}
    return (cnt != 0);
}
