/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	4.24 (Berkeley) 02/05/99
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
 * doctor:
 *	A healing daemon that restors hit points after rest
 */
void
doctor(int arg)
{
    (void)arg;
    int lv, ohp;

    lv = pstats.s_lvl;
    ohp = pstats.s_hpt;
    quiet++;
    if (lv < 8)
    {
	if (quiet + (lv << 1) > 20)
	    pstats.s_hpt++;
    }
    else
	if (quiet >= 3)
	    pstats.s_hpt += rnd(lv - 7) + 1;
    if (ISRING(LEFT, R_REGEN))
	pstats.s_hpt++;
    if (ISRING(RIGHT, R_REGEN))
	pstats.s_hpt++;
    if (ohp != pstats.s_hpt)
    {
	if (pstats.s_hpt > max_hp)
	    pstats.s_hpt = max_hp;
	quiet = 0;
    }
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(int arg)
{
    (void)arg;
    start_daemon(rollwand, 0, BEFORE);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */

void
rollwand(int arg)
{
    (void)arg;

    if (++between >= 3 + rnd(3))
    {
	if (roll(1, 6) == 4)
	{
	    wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse(int arg)
{
    (void)arg;
    player.t_flags &= ~ISHUH;
    msg(JSTR("混乱が少しおさまってきた。", "you feel less confused now"));
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee(int arg)
{
    (void)arg;
    THING *th;

    for (th = mlist; th != NULL; th = next(th))
	if (on(*th, ISINVIS) && see_monst(th))
	    jrogue_mvaddch_map(th->t_pos.y, th->t_pos.x, th->t_oldch);
    player.t_flags &= ~CANSEE;
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight(int arg)
{
    (void)arg;
    if (on(player, ISBLIND))
    {
	extinguish(sight);
	player.t_flags &= ~ISBLIND;
	if (!(proom->r_flags & ISGONE))
	    enter_room(&hero);
	msg(JSTR("暗闇のとばりが上がってゆく。", "the veil of darkness lifts"));
    }
}

/*
 * nohaste:
 *	End the hasting
 */
void
nohaste(int arg)
{
    (void)arg;
    player.t_flags &= ~ISHASTE;
    msg(JSTR("体の動きが遅くなってきた。", "you feel yourself slowing down"));
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach(int arg)
{
    (void)arg;
    int oldfood;
    int orig_hungry = hungry_state;

    if (food_left <= 0)
    {
	if (food_left-- < -STARVETIME)
	    death('s');
	/*
	 * the hero is fainting
	 */
	if (no_command || rnd(5) != 0)
	    return;
	no_command += rnd(8) + 4;
	hungry_state = 3;
	if (!terse)
	    addmsg("%s", JSTR("食べていないので体が弱りきっている。", "you feel very weak. "));
	msg("%s", JSTR("食糧がなくて気を失った。", "you faint from lack of food"));
    }
    else
    {
	oldfood = food_left;
	food_left -= ring_eat(LEFT) + ring_eat(RIGHT) + 1;

	if (food_left < MORETIME && oldfood >= MORETIME)
	{
	    hungry_state = 2;
	    msg("%s", JSTR("そろそろ力がなくなってきた。", "you are starting to feel weak"));
	}
	else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    hungry_state = 1;
	    if (terse)
		msg("%s", JSTR("空腹になってきた。", "getting hungry"));
	    else
		msg("%s", JSTR("そろそろ空腹になってきた。", "you are starting to get hungry"));
	}
    }
    if (hungry_state != orig_hungry) { 
        player.t_flags &= ~ISRUN; 
        running = FALSE; 
        to_death = FALSE; 
        count = 0; 
    } 
}

