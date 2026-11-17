/*
 * new_level:
 *	Dig and draw a new level
 *
 * @(#)new_level.c	4.38 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <string.h>
#include "rogue.h"

#define TREAS_ROOM 20	/* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS 10	/* maximum number of treasures in a treasure room */
#define MINTREAS 2	/* minimum number of treasures in a treasure room */


#define IMPLODE_STEPS 10
#define IMPLODE_DELAY_MS 35

static int level_screen_drawn = FALSE;

/*
 * jrogue_implode_screen:
 *	Clear the old dungeon with an DOS-style shrinking box before the
 *	next level is drawn.  This is intentionally only a screen effect; the
 *	level map is cleared and rebuilt by new_level() after the animation.
 */
static void
jrogue_implode_set_box_attr(void)
{
    jrogue_set_normal_attr();
}

static void
jrogue_implode_puts(int y, int x, const char *s)
{
    if (y < 0 || y >= STATLINE || x < 0 || x >= NUMCOLS)
        return;
    move(y, x);
    (void)jrogue_add_utf8_glyph(s);
}

static void
jrogue_implode_putch(int y, int x, int ch)
{
    if (y < 0 || y >= STATLINE || x < 0 || x >= NUMCOLS)
        return;
    mvaddch(y, x, ch);
}

static void
jrogue_implode_draw_box(int top, int left, int bottom, int right, int erase)
{
    int x, y;
    int unicode = (!erase && MB_CUR_MAX > 1);
    const char *ul = unicode ? "┌" : "+";
    const char *ur = unicode ? "┐" : "+";
    const char *ll = unicode ? "└" : "+";
    const char *lr = unicode ? "┘" : "+";
    const char *h  = unicode ? "─" : "-";
    const char *v  = unicode ? "│" : "|";

    if (top < 0 || left < 0 || bottom >= STATLINE || right >= NUMCOLS ||
        top >= bottom || left >= right)
        return;

    if (erase)
        jrogue_set_normal_attr();
    else
        jrogue_implode_set_box_attr();

    jrogue_implode_puts(top, left, erase ? " " : ul);
    jrogue_implode_puts(top, right, erase ? " " : ur);
    jrogue_implode_puts(bottom, left, erase ? " " : ll);
    jrogue_implode_puts(bottom, right, erase ? " " : lr);

    for (x = left + 1; x < right; x++) {
        if (erase) {
            jrogue_implode_putch(top, x, ' ');
            jrogue_implode_putch(bottom, x, ' ');
        }
        else {
            jrogue_implode_puts(top, x, h);
            jrogue_implode_puts(bottom, x, h);
        }
    }
    for (y = top + 1; y < bottom; y++) {
        if (erase) {
            jrogue_implode_putch(y, left, ' ');
            jrogue_implode_putch(y, right, ' ');
        }
        else {
            jrogue_implode_puts(y, left, v);
            jrogue_implode_puts(y, right, v);
        }
    }
    jrogue_set_normal_attr();
}

static void
jrogue_implode_clear_strip(int y, int x1, int x2)
{
    int x;

    if (y < 0 || y >= STATLINE)
        return;
    if (x1 < 0)
        x1 = 0;
    if (x2 >= NUMCOLS)
        x2 = NUMCOLS - 1;
    if (x1 > x2)
        return;
    jrogue_set_normal_attr();
    for (x = x1; x <= x2; x++)
        jrogue_implode_putch(y, x, ' ');
}

static void
jrogue_implode_screen(void)
{
    int step, y;
    int top = 0, left = 0;
    int bottom = STATLINE - 1;
    int right = NUMCOLS - 1;
    int col_inc = NUMCOLS / IMPLODE_STEPS / 2;

    if (col_inc < 1)
        col_inc = 1;
    if (!level_screen_drawn && level <= 1 && max_level <= 1)
        return;

    jrogue_show_cursor(FALSE);
    for (step = 0; step < IMPLODE_STEPS && top < bottom && left < right;
         step++, top++, bottom--, left += col_inc, right -= col_inc) {
        jrogue_implode_draw_box(top, left, bottom, right, FALSE);
        refresh();
        msleep(IMPLODE_DELAY_MS);

        for (y = top + 1; y < bottom; y++) {
            jrogue_implode_clear_strip(y, left + 1, left + col_inc - 1);
            jrogue_implode_clear_strip(y, right - col_inc + 1, right - 1);
        }
        jrogue_implode_draw_box(top, left, bottom, right, TRUE);
        refresh();
    }
    jrogue_set_normal_attr();
    jrogue_restore_cursor();
}

void
new_level(void)
{
    THING *tp;
    PLACE *pp;
    int *sp;
    int i;
    coord trap_pos;

    player.t_flags &= ~ISHELD;	/* unhold when you go down just in case */
    f_restor();
    if (level > max_level)
	max_level = level;
    jrogue_implode_screen();
    /*
     * Clean things off from last level
     */
    for (pp = places; pp < &places[MAXCOLS*MAXLINES]; pp++)
    {
	pp->p_ch = ' ';
	pp->p_flags = F_REAL;
	pp->p_monst = NULL;
    }
    clear();
    jrogue_clear_screen_chars();
    /*
     * Free up the monsters on the last level
     */
    for (tp = mlist; tp != NULL; tp = next(tp))
	free_list(tp->t_pack);
    free_list(mlist);
    /*
     * Throw away stuff left on the previous level (if anything)
     */
    free_list(lvl_obj);
    do_rooms();				/* Draw rooms */
    do_passages();			/* Draw passages */
    no_food++;
    put_things();			/* Place objects (if any) */
    /*
     * Place the staircase down before traps, as in the DOS 1.48 code.
     */
    find_floor(NULL, &stairs, FALSE, FALSE);
    chat(stairs.y, stairs.x) = STAIRS;
    seenstairs = FALSE;
    /*
     * Place the traps
     */
    if (rnd(10) < level)
    {
	ntraps = rnd(level / 4) + 1;
	if (ntraps > MAXTRAPS)
	    ntraps = MAXTRAPS;
	i = ntraps;
	while (i--)
	{
	    find_floor(NULL, &trap_pos, FALSE, FALSE);
	    sp = &flat(trap_pos.y, trap_pos.x);
	    *sp &= ~(F_REAL | F_TMASK);
	    *sp |= rnd(NTRAPS);
	}
    }

    for (tp = mlist; tp != NULL; tp = next(tp))
	tp->t_room = roomin(&tp->t_pos);

    find_floor(NULL, &hero, FALSE, TRUE);
    enter_room(&hero);
    oldpos = hero;
    oldrp = roomin(&hero);
    proom = oldrp;
    jrogue_mvaddch_map(hero.y, hero.x, PLAYER);
    level_screen_drawn = TRUE;
    if (on(player, SEEMONST))
	turn_see(FALSE);
}

/*
 * rnd_room:
 *	Pick a room that is really there
 */
int
rnd_room(void)
{
    int rm;

    do
    {
	rm = rnd(MAXROOMS);
    } while (!((rooms[rm].r_flags & ISGONE) == 0 ||
	       (rooms[rm].r_flags & ISMAZE)));
    return rm;
}

/*
 * put_things:
 *	Put potions and scrolls on this level
 */

void
put_things(void)
{
    int i = 0;
    THING *obj;

    /*
     * Once you have found the amulet and are climbing back up, the DOS 1.48
     * version still gives one chance for an object so the player may find food.
     */
    if (saw_amulet && level < max_level)
	i = MAXOBJ - 1;
    else
    {
	/*
	 * If he is really deep in the dungeon and he hasn't found the
	 * amulet yet, put it somewhere on the ground before other objects.
	 */
	if (level >= AMULETLEVEL && !saw_amulet)
	{
	    obj = new_item();
	    if (obj == NULL)
		return;
	    attach(lvl_obj, obj);
	    obj->o_hplus = 0;
	    obj->o_dplus = 0;
	    jrogue_strlcpy(obj->o_damage, "0x0", sizeof obj->o_damage);
	    jrogue_strlcpy(obj->o_hurldmg, "0x0", sizeof obj->o_hurldmg);
	    obj->o_arm = 11;
	    obj->o_type = AMULET;
	    find_floor(NULL, &obj->o_pos, FALSE, FALSE);
	    chat(obj->o_pos.y, obj->o_pos.x) = AMULET;
	}
	/*
	 * check for treasure rooms, and if so, put it in.
	 */
	if (rnd(TREAS_ROOM) == 0)
	    treas_room();
    }
    /*
     * Do MAXOBJ attempts to put things on a level
     */
    for (; i < MAXOBJ; i++)
	if (rnd(100) < 35)
	{
	    obj = new_thing();
	    if (obj == NULL)
		continue;
	    attach(lvl_obj, obj);
	    find_floor(NULL, &obj->o_pos, FALSE, FALSE);
	    chat(obj->o_pos.y, obj->o_pos.x) = obj->o_type;
	}
}

/*
 * treas_room:
 *	Add a treasure room
 */
#define MAXTRIES 10	/* max number of tries to put down a monster */


void
treas_room(void)
{
    int nm;
    THING *tp;
    struct room *rp;
    int spots, num_monst;
    coord mp;

    rp = &rooms[rnd_room()];
    spots = (rp->r_max.y - 2) * (rp->r_max.x - 2) - MINTREAS;
    if (spots > (MAXTREAS - MINTREAS))
	spots = (MAXTREAS - MINTREAS);
    num_monst = nm = rnd(spots) + MINTREAS;
    while (nm--)
    {
	if (!find_floor(rp, &mp, 2 * MAXTRIES, FALSE))
	    continue;
	tp = new_thing();
	if (tp == NULL)
	    continue;
	tp->o_pos = mp;
	attach(lvl_obj, tp);
	chat(mp.y, mp.x) = tp->o_type;
    }

    /*
     * fill up room with monsters from the next level down
     */

    if ((nm = rnd(spots) + MINTREAS) < num_monst + 2)
	nm = num_monst + 2;
    spots = (rp->r_max.y - 2) * (rp->r_max.x - 2);
    if (nm > spots)
	nm = spots;
    level++;
    while (nm--)
    {
	spots = 0;
	if (find_floor(rp, &mp, MAXTRIES, TRUE))
	{
	    tp = new_item();
	    if (tp == NULL)
		continue;
	    new_monster(tp, randmonster(FALSE), &mp);
	    tp->t_flags |= ISMEAN;	/* no sloughers in THIS room */
	    give_pack(tp);
	}
    }
    level--;
}
