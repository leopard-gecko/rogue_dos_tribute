/*
 * Create the layout for the new level
 *
 * @(#)rooms.c	4.45 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <ctype.h>
#include <curses.h>
#include "rogue.h"

#define MAXFRNT 100
#define FRONTIER 'F'
#define NOTHING ' '

#define GOLDGRP 1

/*
 * do_rooms:
 *	Create rooms and corridors with a connectivity graph
 */

void
do_rooms(void)
{
    int i, rm;
    struct room *rp;
    THING *tp;
    int left_out;
    coord top;
    coord bsze;				/* maximum room size */
    coord mp;

    bsze.x = NUMCOLS / 3;
    bsze.y = NUMLINES / 3;
    /*
     * Clear things for a new level
     */
    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
    {
	rp->r_goldval = 0;
	rp->r_nexits = 0;
	rp->r_flags = 0;
    }
    /*
     * Put the gone rooms, if any, on the level
     */
    left_out = rnd(4);
    for (i = 0; i < left_out; i++)
    {
	do
	    rp = &rooms[rm = rnd_room()];
	while (rp->r_flags & ISMAZE);
	rp->r_flags |= ISGONE;
	if (rm > 2 && level > 10 && rnd(20) < level - 9)
	    rp->r_flags |= ISMAZE;
    }
    /*
     * dig and populate all the rooms on the level
     */
    for (i = 0, rp = rooms; i < MAXROOMS; rp++, i++)
    {
	/*
	 * Find upper left corner of box that this room goes in
	 */
	top.x = (i % 3) * bsze.x + 1;
	top.y = (i / 3) * bsze.y;
	if (rp->r_flags & ISGONE)
	{
	    /*
	     * In the DOS 1.48 version, maze rooms are made from rooms that
	     * were otherwise left out.
	     */
	    if (rp->r_flags & ISMAZE)
	    {
		rp->r_pos.x = top.x;
		rp->r_pos.y = top.y;
		draw_room(rp);
	    }
	    else
	    {
		/*
		 * Place a gone room.  Make certain that there is a blank line
		 * for passage drawing.
		 */
		do
		{
		    rp->r_pos.x = top.x + rnd(bsze.x - 2) + 1;
		    rp->r_pos.y = top.y + rnd(bsze.y - 2) + 1;
		    rp->r_max.x = -NUMCOLS;
		    rp->r_max.y = -NUMLINES;
		} until (rp->r_pos.y > 0 && rp->r_pos.y < NUMLINES-1);
	    }
	    continue;
	}
	/*
	 * set room type
	 */
	if (rnd(10) < level - 1)
	    rp->r_flags |= ISDARK;		/* dark room */
	/*
	 * Find a place and size for a random room
	 */
	do
	{
	    rp->r_max.x = rnd(bsze.x - 4) + 4;
	    rp->r_max.y = rnd(bsze.y - 4) + 4;
	    rp->r_pos.x = top.x + rnd(bsze.x - rp->r_max.x);
	    rp->r_pos.y = top.y + rnd(bsze.y - rp->r_max.y);
	} until (rp->r_pos.y != 0);
	draw_room(rp);
	/*
	 * Put the gold in
	 */
	if (rnd(2) == 0 && (!saw_amulet || level >= max_level))
	{
	    THING *gold;

	    gold = new_item();
	    if (gold == NULL)
		return;
	    gold->o_goldval = rp->r_goldval = GOLDCALC;
	    find_floor(rp, &rp->r_gold, FALSE, FALSE);
	    gold->o_pos = rp->r_gold;
	    chat(rp->r_gold.y, rp->r_gold.x) = GOLD;
	    gold->o_flags = ISMANY;
	    gold->o_group = GOLDGRP;
	    gold->o_type = GOLD;
	    attach(lvl_obj, gold);
	}
	/*
	 * Put the monster in
	 */
	if (rnd(100) < (rp->r_goldval > 0 ? 80 : 25))
	{
	    tp = new_item();
	    if (tp == NULL)
		return;
	    find_floor(rp, &mp, FALSE, TRUE);
	    new_monster(tp, randmonster(FALSE), &mp);
	    give_pack(tp);
	}
    }
}

/*
 * draw_room:
 *	Draw a box around a room and lay down the floor for normal
 *	rooms; for maze rooms, draw maze.
 */

void
draw_room(struct room *rp)
{
    int y, x;

    if (rp->r_flags & ISMAZE)
	do_maze(rp);
    else
    {
	vert(rp, rp->r_pos.x);				/* Draw left side */
	vert(rp, rp->r_pos.x + rp->r_max.x - 1);	/* Draw right side */
	horiz(rp, rp->r_pos.y);				/* Draw top */
	horiz(rp, rp->r_pos.y + rp->r_max.y - 1);	/* Draw bottom */

	/*
	 * Put the floor down
	 */
	for (y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
	    for (x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
		chat(y, x) = FLOOR;
    }
}

/*
 * vert:
 *	Draw a vertical line
 */

void
vert(const struct room *rp, int startx)
{
    int y;

    for (y = rp->r_pos.y + 1; y <= rp->r_max.y + rp->r_pos.y - 1; y++)
	chat(y, startx) = '|';
}

/*
 * horiz:
 *	Draw a horizontal line
 */

void
horiz(const struct room *rp, int starty)
{
    int x;

    for (x = rp->r_pos.x; x <= rp->r_pos.x + rp->r_max.x - 1; x++)
	chat(starty, x) = '-';
}

/*
 * do_maze:
 *	Draw a maze room using the DOS 1.48 frontier algorithm.
 */

static int frcnt, ny, nx, topy, topx;
static int maze_maxx, maze_maxy;
static int *fr_y, *fr_x;

static int  inrange_maze(int y, int x);
static int  maze_at(int y, int x);
static void splat_maze(int y, int x);
static void add_frnt(int y, int x);
static void new_frontier(int y, int x);
static void con_frnt(void);

void
do_maze(struct room *rp)
{
    int y, x;
    int fy[MAXFRNT], fx[MAXFRNT];
    int psgcnt;
    coord spos;

    fr_y = fy;
    fr_x = fx;
    frcnt = 0;
    maze_maxx = maze_maxy = 0;
    topy = rp->r_pos.y;
    if (topy == 0)
	topy = ++rp->r_pos.y;
    topx = rp->r_pos.x;

    y = topy;
    x = topx;
    splat_maze(y, x);
    new_frontier(y, x);
    while (frcnt)
    {
	con_frnt();
	new_frontier(ny, nx);
    }

    /*
     * The DOS version gives every maze one extra loop by opening a
     * non-passage cell with a suitable pair of neighboring passages.
     */
    rp->r_max.x = maze_maxx - rp->r_pos.x + 1;
    rp->r_max.y = maze_maxy - rp->r_pos.y + 1;
    do
    {
	static coord ld[4] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1} };
	coord *cp;
	int sh;

	rnd_pos(rp, &spos);
	psgcnt = 0;
	for (cp = ld, sh = 1; cp < &ld[4]; sh <<= 1, cp++)
	{
	    y = cp->y + spos.y;
	    x = cp->x + spos.x;
	    if (y > 0 && y < NUMLINES && x >= 0 && x < NUMCOLS &&
		chat(y, x) == PASSAGE)
		psgcnt += sh;
	}
    } while (chat(spos.y, spos.x) == PASSAGE || psgcnt % 5);
    splat_maze(spos.y, spos.x);
}

static void
new_frontier(int y, int x)
{
    add_frnt(y - 2, x);
    add_frnt(y + 2, x);
    add_frnt(y, x - 2);
    add_frnt(y, x + 2);
}

static void
add_frnt(int y, int x)
{
    if (frcnt >= MAXFRNT)
	return;
    if (inrange_maze(y, x) && chat(y, x) == NOTHING)
    {
	chat(y, x) = FRONTIER;
	fr_y[frcnt] = y;
	fr_x[frcnt++] = x;
    }
}

static void
con_frnt(void)
{
    int n, which, ydelt = 0, xdelt = 0;
    int choice[4];
    int cnt = 0, y, x;

    n = rnd(frcnt);
    ny = fr_y[n];
    nx = fr_x[n];
    fr_y[n] = fr_y[frcnt - 1];
    fr_x[n] = fr_x[--frcnt];

    if (maze_at(ny - 2, nx) > 0)
	choice[cnt++] = 0;
    if (maze_at(ny + 2, nx) > 0)
	choice[cnt++] = 1;
    if (maze_at(ny, nx - 2) > 0)
	choice[cnt++] = 2;
    if (maze_at(ny, nx + 2) > 0)
	choice[cnt++] = 3;

    which = choice[rnd(cnt)];
    splat_maze(ny, nx);
    switch (which)
    {
	case 0: ydelt = -1; break;
	case 1: ydelt = 1; break;
	case 2: xdelt = -1; break;
	case 3: xdelt = 1; break;
    }
    y = ny + ydelt;
    x = nx + xdelt;
    if (inrange_maze(y, x))
	splat_maze(y, x);
}

static int
maze_at(int y, int x)
{
    return (inrange_maze(y, x) && chat(y, x) == PASSAGE);
}

static void
splat_maze(int y, int x)
{
    chat(y, x) = PASSAGE;
    flat(y, x) = F_MAZE | F_REAL;
    if (x > maze_maxx)
	maze_maxx = x;
    if (y > maze_maxy)
	maze_maxy = y;
}

static int
inrange_maze(int y, int x)
{
    return (y >= topy && y < topy + NUMLINES / 3 &&
	x >= topx && x < topx + NUMCOLS / 3);
}

/*
 * rnd_pos:
 *	Pick a random spot in a room
 */

void
rnd_pos(const struct room *rp, coord *cp)
{
    cp->x = rp->r_pos.x + rnd(rp->r_max.x - 2) + 1;
    cp->y = rp->r_pos.y + rnd(rp->r_max.y - 2) + 1;
}

/*
 * find_floor:
 *	Find a valid floor spot in this room.  If rp is NULL, then
 *	pick a new room each time around the loop.
 */
int
find_floor(const struct room *rp, coord *cp, int limit, int monst)
{
    PLACE *pp;
    int cnt;
    int compchar = 0;
    int pickroom;

    pickroom = (rp == NULL);

    if (!pickroom)
	compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
    cnt = limit;
    for (;;)
    {
	if (limit && cnt-- == 0)
	    return FALSE;
	if (pickroom)
	{
	    rp = &rooms[rnd_room()];
	    compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
	}
	rnd_pos(rp, cp);
	pp = INDEX(cp->y, cp->x);
	if (monst)
	{
	    if (pp->p_monst == NULL && isfloor(pp->p_ch) &&
		(pp->p_flags & F_REAL))
		return TRUE;
	}
	else if (pp->p_ch == compchar)
	    return TRUE;
    }
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */

void
enter_room(const coord *cp)
{
    struct room *rp;
    THING *tp;
    int y, x;
    chtype ch;

    rp = proom = roomin(cp);
    door_open(rp);
    if (!(rp->r_flags & ISDARK) && !on(player, ISBLIND) &&
	!(rp->r_flags & ISMAZE))
	for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
	{
	    move(y, rp->r_pos.x);
	    for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
	    {
		tp = moat(y, x);
		ch = chat(y, x);
		if (tp == NULL)
		    if ((chtype)jrogue_mvinch_char(y, x) != ch)
			addch2(ch);
		    else
			move(y, x + 1);
		else
		{
		    tp->t_oldch = ch;
		    if (!see_monst(tp))
			if (on(player, SEEMONST))
			{
			    standout2();
			    addch2(tp->t_disguise);
			    standend2();
			}
			else
			    addch2(ch);
		    else
			addch2(tp->t_disguise);
		}
	    }
	}
}

/*
 * leave_room:
 *	Code for when we exit a room
 */

void
leave_room(const coord *cp)
{
    PLACE *pp;
    struct room *rp;
    int y, x;
    int floor;
    int ch;

    rp = proom;

    proom = &passages[flat(cp->y, cp->x) & F_PNUM];

    if (rp->r_flags & ISMAZE)
	floor = PASSAGE;
    else if (rp->r_flags & ISGONE)
	floor = PASSAGE;
    else if (!(rp->r_flags & ISDARK) || on(player, ISBLIND))
	floor = FLOOR;
    else
	floor = ' ';

    for (y = rp->r_pos.y + 1; y < rp->r_max.y + rp->r_pos.y - 1; y++)
	for (x = rp->r_pos.x + 1; x < rp->r_max.x + rp->r_pos.x - 1; x++)
	{
	    move(y, x);
	    switch ( ch = jrogue_mvinch_char(y, x) )
	    {
		case ' ':
		case PASSAGE:
		case TRAP:
		case STAIRS:
		    break;
		case FLOOR:
		    if (floor == ' ' && ch != ' ')
			addch2(' ');
		    break;
		default:
		    /*
		     * to check for monster, we have to strip out
		     * standout bit
		     */
		    if (isupper(toascii(ch)))
		    {
			if (on(player, SEEMONST))
			{
			    standout2();
			    addch2(ch);
			    standend2();
			    break;
			}
                        pp = INDEX(y,x);
			addch2(pp->p_ch == DOOR ? DOOR : floor);
		    }
	    }
	}
    door_open(rp);
}
