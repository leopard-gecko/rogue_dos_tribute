/*
 * File with various monster functions in it
 *
 * @(#)monsters.c	4.46 (Berkeley) 02/05/99
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
#include <ctype.h>

/*
 * List of monsters in rough order of vorpalness
 */
static const int lvl_mons[] =  {
    'K',   0, 'B', 'H', 'I', 'S', 'O', 'R',   0, 'L', 'C', 'A',   0,
    'N', 'Y', 'T', 'W', 'F', 'P',   0, 'G', 'M', 'X', 'V', 'J', 'D'
};

static const int wand_mons[] = {
    'K', 'E', 'B', 'H', 'I', 'S', 'O', 'R', 'Z',   0, 'C', 'A', 'Q',
      0, 'Y', 'T', 'W',   0, 'P', 'U', 'G', 'M',   0, 'V', 'J',   0
};

static int
xeroc_disguise(void)
{
    static const int things[] = {
	GOLD, POTION, SCROLL, STAIRS, WEAPON, ARMOR, RING, STICK
    };

    if (level >= AMULETLEVEL && rnd(9) == 0)
	return AMULET;
    return things[rnd(sizeof things / sizeof things[0])];
}

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
int
randmonster(int wander)
{
    int d;
    const int *mons;

    mons = (wander ? wand_mons : lvl_mons);
    do
    {
	d = level + (rnd(5) + rnd(6) - 5);
	if (d < 1)
	    d = rnd(5) + 1;
	if (d > 26)
	    d = rnd(5) + 22;
	d--;
    } while (mons[d] == 0);
    return mons[d];
}

/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */

void
new_monster(THING *tp, int type, const coord *cp)
{
    struct monster *mp;
    int lev_add;

    if ((lev_add = level - AMULETLEVEL) < 0)
	lev_add = 0;
    attach(mlist, tp);
    tp->t_type = type;
    tp->t_disguise = type;
    tp->t_pos = *cp;
    move(cp->y, cp->x);
    tp->t_oldch = jrogue_mvinch_char(cp->y, cp->x);
    tp->t_room = roomin(cp);
    moat(cp->y, cp->x) = tp;
    mp = &monsters[tp->t_type-'A'];
    tp->t_stats.s_lvl = mp->m_stats.s_lvl + lev_add;
    tp->t_stats.s_maxhp = tp->t_stats.s_hpt = roll(tp->t_stats.s_lvl, 8);
    tp->t_stats.s_arm = mp->m_stats.s_arm - lev_add;
    jrogue_strlcpy(tp->t_stats.s_dmg, mp->m_stats.s_dmg, sizeof tp->t_stats.s_dmg);
    tp->t_stats.s_str = mp->m_stats.s_str;
    tp->t_stats.s_exp = mp->m_stats.s_exp + lev_add * 10 + exp_add(tp);
    tp->t_flags = mp->m_flags;
    tp->t_turn = TRUE;
    tp->t_pack = NULL;
    if (ISWEARING(R_AGGR))
	runto(cp);
    if (type == 'X')
	tp->t_disguise = xeroc_disguise();
}


/*
 * f_restor:
 *	Restore the initial flytrap damage string after the player is released.
 */
void
f_restor(void)
{
    THING *tp;

    vf_hit = 0;
    jrogue_strlcpy(monsters['F' - 'A'].m_stats.s_dmg, "0x0",
		   sizeof monsters['F' - 'A'].m_stats.s_dmg);
    for (tp = mlist; tp != NULL; tp = next(tp))
	if (tp->t_type == 'F')
	    jrogue_strlcpy(tp->t_stats.s_dmg, "0x0",
			   sizeof tp->t_stats.s_dmg);
}

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
int
exp_add(const THING *tp)
{
    int mod;

    if (tp->t_stats.s_lvl == 1)
	mod = tp->t_stats.s_maxhp / 8;
    else
	mod = tp->t_stats.s_maxhp / 6;
    if (tp->t_stats.s_lvl > 9)
	mod *= 20;
    else if (tp->t_stats.s_lvl > 6)
	mod *= 4;
    return mod;
}

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player
 */

void
wanderer(void)
{
    THING *tp;
    coord cp;
    int cnt = 0;

    tp = new_item();
    if (tp == NULL)
	return;
    do
    {
        /* Avoid endless loop when all rooms are filled with monsters
	 * and the player room is not accessible to the monsters.
	 */
	if (cnt++ >= 500)
	{
	    discard(tp);
	    return;
	}
	find_floor(NULL, &cp, FALSE, TRUE);
    } while (roomin(&cp) == proom && moat(cp.y, cp.x) == NULL);
    new_monster(tp, randmonster(TRUE), &cp);
    if (on(player, SEEMONST))
    {
	standout2();
	addch2(tp->t_type);
	standend2();
    }
    runto(&tp->t_pos);
#ifdef MASTER
    if (wizard)
	msg(JSTR("%sがさまよい始めた", "started a wandering %s"), monsters[tp->t_type-'A'].m_name);
#endif
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
const THING *
wake_monster(int y, int x)
{
    THING *tp;
    struct room *rp;
	int ch;

	if ((tp = moat(y, x)) == NULL) {
#ifdef MASTER
		msg("can't find monster in wake_monster");
#endif
		return NULL;
	}

    ch = tp->t_type;
    /*
     * Every time he sees mean monster, it might start chasing him
     */
    if (!on(*tp, ISRUN) && rnd(3) != 0 && on(*tp, ISMEAN) && !on(*tp, ISHELD)
	&& !ISWEARING(R_STEALTH))
    {
	tp->t_dest = &hero;
	tp->t_flags |= ISRUN;
    }
    if (ch == 'M' && !on(player, ISBLIND)
	&& !on(*tp, ISFOUND) && !on(*tp, ISCANC) && on(*tp, ISRUN))
    {
        rp = proom;
	if ((rp != NULL && !(rp->r_flags & ISDARK))
	    || dist(y, x, hero.y, hero.x) < LAMPDIST)
	{
	    tp->t_flags |= ISFOUND;
	    if (!save(VS_MAGIC))
	    {
		if (on(player, ISHUH))
		    lengthen(unconfuse, rnd(20) + spread(HUHDURATION));
		else
		    fuse(unconfuse, 0, rnd(20) + spread(HUHDURATION), AFTER);
		player.t_flags |= ISHUH;
		msg(JSTR("メデューサににらまれ、頭が混乱してしまった。", "the medusa's gaze has confused you"));
	    }
	}
    }
    /*
     * Let greedy ones guard gold
     */
    if (on(*tp, ISGREED) && !on(*tp, ISRUN))
    {
	tp->t_flags |= ISRUN;
	if (proom->r_goldval)
	    tp->t_dest = &proom->r_gold;
	else
	    tp->t_dest = &hero;
    }
    return tp;
}

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one
 */

void
give_pack(THING *tp)
{
    if (rnd(100) < monsters[tp->t_type-'A'].m_carry)
    {
	THING *obj;

	obj = new_thing();
	if (obj != NULL)
	    attach(tp->t_pack, obj);
    }
}


/*
 * pick_mons:
 *	Choose a sort of monster for the enemy of a vorpally enchanted weapon.
 */
int
pick_mons(void)
{
    int i;

    for (i = 25; i >= 0 && rnd(10) != 0; i--)
        ;
    if (i < 0 || lvl_mons[i] == 0)
        return 'M';
    return lvl_mons[i];
}

/*
 * save_throw:
 *	See if a creature save against something
 */
int
save_throw(int which, const THING *tp)
{
    int need;

    need = 14 + which - tp->t_stats.s_lvl / 2;
    return (roll(1, 20) >= need);
}

/*
 * save:
 *	See if he saves against various nasty things
 */
int
save(int which)
{
    if (which == VS_MAGIC)
    {
	if (ISRING(LEFT, R_PROTECT))
	    which -= cur_ring[LEFT]->o_arm;
	if (ISRING(RIGHT, R_PROTECT))
	    which -= cur_ring[RIGHT]->o_arm;
    }
    return save_throw(which, &player);
}
