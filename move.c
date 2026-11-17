/*
 * hero movement commands
 *
 * @(#)move.c	4.49 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"

/*
 * Remember the direction at each square during one continuous run.
 * As in Rogue 5.4.5 JEC, revisiting the same square with the same
 * heading ends a run so that an automatically turning passage loop
 * cannot keep the player moving indefinitely.
 * Each byte holds one bit for each of the eight movement directions.
 */
static unsigned char run_visited[NUMLINES][NUMCOLS];

static unsigned char
run_direction_bit(int ch)
{
    switch (ch)
    {
        case 'h': return 1U << 0;
        case 'j': return 1U << 1;
        case 'k': return 1U << 2;
        case 'l': return 1U << 3;
        case 'y': return 1U << 4;
        case 'u': return 1U << 5;
        case 'b': return 1U << 6;
        case 'n': return 1U << 7;
    }
    return 0;
}

/*
 * do_run:
 *	Start the hero running
 */

void
do_run(int ch)
{
    /* A new run must not inherit any visited states from the last run. */
    memset(run_visited, 0, sizeof run_visited);
    running = TRUE;
    after = FALSE;
    runch = ch;
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */

void
do_move(int dy, int dx)
{
    int ch, fl;
    coord nh;

    firstmove = FALSE;
    if (no_move)
    {
	no_move--;
	msg(JSTR("とらばさみのわなでまだ身動きがとれない。", "you are still stuck in the bear trap"));
	return;
    }
    if (running)
    {
        unsigned char bit = run_direction_bit(runch);
        /* Stop before revisiting the same position and direction. */
        if (bit != 0 && hero.y > 0 && hero.y < NUMLINES - 1
            && hero.x >= 0 && hero.x < NUMCOLS)
        {
            if (run_visited[hero.y][hero.x] & bit)
            {
                running = FALSE;
                after = FALSE;
                return;
            }
            run_visited[hero.y][hero.x] |= bit;
        }
    }
    /*
     * Do a confused move (maybe)
     */
    if (on(player, ISHUH) && rnd(5) != 0)
    {
	nh = rndmove(&player);
	if (ce(nh, hero))
	{
	    after = FALSE;
	    running = FALSE;
	    to_death = FALSE;
	    return;
	}
    }
    else
    {
over:
	nh.y = hero.y + dy;
	nh.x = hero.x + dx;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;
    if (!diag_ok(&hero, &nh))
    {
	after = FALSE;
	running = FALSE;
	return;
    }
    if (running && ce(hero, nh))
	after = running = FALSE;
    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);
    /* Do not keep running when stepping from a doorway into a room. */
    if (chat(hero.y, hero.x) == DOOR && ch == FLOOR)
	running = FALSE;
    if (!(fl & F_REAL) && ch == FLOOR)
    {
	chat(nh.y, nh.x) = ch = TRAP;
	flat(nh.y, nh.x) |= F_REAL;
    }
    else if (on(player, ISHELD) && ch != 'F')
    {
	msg(JSTR("怪物につかまえられている。", "you are being held"));
	return;
    }
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
hit_bound:
	    if (running && (proom->r_flags & ISGONE)
		&& !on(player, ISBLIND))
	    {
		int	b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (hero.y != 1 && turn_ok(hero.y - 1, hero.x));
			b2 = (hero.y != NUMLINES - 2 && turn_ok(hero.y + 1, hero.x));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'k';
			    dy = -1;
			}
			else
			{
			    runch = 'j';
			    dy = 1;
			}
			dx = 0;
			turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (hero.x != 0 && turn_ok(hero.y, hero.x - 1));
			b2 = (hero.x != NUMCOLS - 1 && turn_ok(hero.y, hero.x + 1));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'h';
			    dx = -1;
			}
			else
			{
			    runch = 'l';
			    dx = 1;
			}
			dy = 0;
			turnref();
			goto over;
		}
	    }
	    running = FALSE;
	    after = FALSE;
	    break;
	case DOOR:
	    running = FALSE;
	    if (flat(hero.y, hero.x) & F_PASS)
		enter_room(&nh);
	    goto move_stuff;
	case TRAP:
	    ch = be_trapped(&nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return;
	    goto move_stuff;
	case PASSAGE:
	    goto move_stuff;
	case FLOOR:
	    if (!(fl & F_REAL))
		be_trapped(&hero);
	    goto move_stuff;
	case STAIRS:
	    seenstairs = TRUE;
	    /* FALLTHROUGH */
	default:
	    running = FALSE;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight(&nh, cur_weapon, FALSE);
	    else
	    {
		if (ch != STAIRS)
		    take = ch;
move_stuff:
		jrogue_mvaddch_map(hero.y, hero.x, floor_at());
		/*
		 * Match the DOS 1.48 maze-room bookkeeping more closely:
		 * leaving a doorway into a passage, or stepping out of a
		 * maze-room passage, runs the room-exit code.  Entering a
		 * maze room from a non-maze square sets proom to that maze
		 * room without drawing it as an ordinary lit room.
		 */
		if ((fl & F_PASS) &&
		    (chat(oldpos.y, oldpos.x) == DOOR ||
		     (flat(oldpos.y, oldpos.x) & F_MAZE)))
		    leave_room(&nh);
		if ((fl & F_MAZE) &&
		    (flat(oldpos.y, oldpos.x) & F_MAZE) == 0)
		    enter_room(&nh);
		hero = nh;
	    }
    }
}

/*
 * turn_ok:
 *	Decide whether it is legal to turn onto the given space
 */
int
turn_ok(int y, int x)
{
    PLACE *pp;

    pp = INDEX(y, x);
    return (pp->p_ch == DOOR
	|| (pp->p_flags & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
}

/*
 * turnref:
 *	Decide whether to refresh at a passage turning or not
 */

void
turnref(void)
{
    PLACE *pp;

    pp = INDEX(hero.y, hero.x);
    if (!(pp->p_flags & F_SEEN))
    {
	if (jump)
	{
	    leaveok(stdscr, TRUE);
	    jrogue_restore_cursor();
	    refresh();
	    leaveok(stdscr, FALSE);
	}
	pp->p_flags |= F_SEEN;
    }
}

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */

void
door_open(const struct room *rp)
{
    int y, x;

    if (!(rp->r_flags & ISGONE) && !on(player, ISBLIND))
	for (y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
	    for (x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
		if (isupper(winat(y, x)))
		    wake_monster(y, x);
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
int
be_trapped(const coord *tc)
{
    PLACE *pp;
    THING *arrow;
    int tr;

    running = FALSE;
    count = FALSE;
    pp = INDEX(tc->y, tc->x);
    pp->p_ch = TRAP;
    tr = pp->p_flags & F_TMASK;
    pp->p_flags |= F_SEEN;
    was_trapped = TRUE;
    switch (tr)
    {
	case T_DOOR:
	    descend(JSTR("わなの中に落ちた！", "you fell into a trap!"));
	when T_BEAR:
	    no_move += BEARTIME;
	    msg(JSTR("とらばさみのわなにつかまった。", "you are caught in a bear trap"));
	when T_SLEEP:
	    no_command += SLEEPTIME;
	    player.t_flags &= ~ISRUN;
	    msg(JSTR("不思議な白いもやにつつまれ、眠り込んでしまった。", "a strange white mist envelops you and you fall asleep"));
	when T_ARROW:
	    if (swing(pstats.s_lvl - 1, pstats.s_arm, 1))
	    {
		pstats.s_hpt -= roll(1, 6);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("矢に撃たれて死んでしまった。", "an arrow killed you"));
		    death('a');
		}
		else
		    msg(JSTR("うわっ！矢に撃たれた。", "oh no! An arrow shot you"));
	    }
	    else
	    {
		arrow = new_item();
		if (arrow != NULL)
		{
		    init_weapon(arrow, ARROW);
		    arrow->o_count = 1;
		    arrow->o_pos = hero;
		    fall(arrow, FALSE);
		}
		msg(JSTR("矢が体をかすめていった。", "an arrow shoots past you"));
	    }
	when T_TELEP:
	    /*
	     * since the hero's leaving, look() won't put a TRAP
	     * down for us, so we have to do it ourself
	     */
	    teleport();
	    jrogue_mvaddch_map(tc->y, tc->x, TRAP);
	    was_trapped++;
	when T_DART:
	    if (!swing(pstats.s_lvl+1, pstats.s_arm, 1))
		msg(JSTR("小さな投げ矢が耳をかすめ、どこかに消えてしまった。", "a dart whizzes by your ear and vanishes"));
	    else
	    {
		pstats.s_hpt -= roll(1, 4);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("毒の投げ矢が当たり死んでしまった。", "a poisoned dart killed you"));
		    death('d');
		}
		if (!ISWEARING(R_SUSTSTR) && !save(VS_POISON))
		    chg_str(-1);
		msg(JSTR("小さな投げ矢が肩に刺さった。", "a dart just hit you in the shoulder"));
	    }
    }
    flush_type();
    return tr;
}


/*
 * descend:
 *	Fall to the next level through a trapdoor or collapsing floor.
 */
void
descend(const char *mesg)
{
    level++;
    if (*mesg == '\0')
	msg(" ");
    new_level();
    msg("");
    msg("%s", mesg);
    if (!save(VS_LUCK))
    {
	msg(JSTR("落ちて、けがをした", "you are damaged by the fall"));
	if ((pstats.s_hpt -= roll(1, 8)) <= 0)
	    death('f');
    }
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
coord
rndmove(const THING *who)
{
    THING *obj;
    int x, y;
    int ch;
    coord ret;  /* what we will be returning */

    y = ret.y = who->t_pos.y + rnd(3) - 1;
    x = ret.x = who->t_pos.x + rnd(3) - 1;
    /*
     * Now check to see if that's a legal move.  If not, don't move.
     * (I.e., bump into the wall or whatever)
     */
    if (y == who->t_pos.y && x == who->t_pos.x)
	return ret;
    if (!diag_ok(&who->t_pos, &ret))
	goto bad;
    else
    {
	ch = winat(y, x);
	if (!step_ok(ch))
	    goto bad;
	if (ch == SCROLL)
	{
	    for (obj = lvl_obj; obj != NULL; obj = next(obj))
		if (y == obj->o_pos.y && x == obj->o_pos.x)
		    break;
	    if (obj != NULL && obj->o_which == S_SCARE)
		goto bad;
	}
    }
    return ret;

bad:
    ret = who->t_pos;
    return ret;
}

/*
 * rust_armor:
 *	Rust the given armor, if it is a legal kind to rust, and we
 *	aren't wearing a magic ring.
 */

void
rust_armor(THING *arm)
{
    if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
	arm->o_arm >= 9)
	    return;

    if (ISWEARING(R_SUSTARM))
    {
	if (!to_death)
	    msg(JSTR("さびはすぐに消え去った。", "the rust vanishes instantly"));
    }
    else
    {
	arm->o_arm++;
	msg(JSTR("よろいがもろくなった。なんてこった！", "your armor weakens, oh my!"));
    }
}
