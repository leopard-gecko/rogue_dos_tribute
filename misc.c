/*
 * All sorts of miscellaneous routines
 *
 * @(#)misc.c	4.66 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"


static void
blind_room_display(const struct room *rp)
{
    int y, x;

    if (rp == NULL || (rp->r_flags & (ISGONE|ISMAZE)))
	return;

    /*
     * Keep the 5.4.4 potion flow intact: the blindness potion still enters
     * do_pot(), sets ISBLIND, and calls look(FALSE).  The redraw rule belongs
     * here because look() is the routine that interprets what the player can
     * currently see.  Once blind, previously visible room contents must not
     * remain on the physical screen.
     */
    for (y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
	for (x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
	    if (y != hero.y || x != hero.x)
		mvaddch2(y, x, ' ');
}

/*
 * look:
 *	A quick glance all around the player
 */
#undef DEBUG


void
look(int wakeup)
{
    int x, y;
    chtype ch;
    THING *tp;
    PLACE *pp;
    struct room *rp;
    int ey, ex;
    int passcount;
    int pfl, *fp, pch;
    int sy, sx, sumhero = 0, diffhero = 0;
# ifdef DEBUG
    static int done = FALSE;

    if (done)
	return;
    done = TRUE;
# endif /* DEBUG */
    passcount = 0;
    rp = proom;
    if (on(player, ISBLIND))
	blind_room_display(rp);
    if (!ce(oldpos, hero))
    {
	erase_lamp(&oldpos, oldrp);
	oldpos = hero;
	oldrp = rp;
    }
    ey = hero.y + 1;
    ex = hero.x + 1;
    sx = hero.x - 1;
    sy = hero.y - 1;
    if (door_stop && !firstmove && running)
    {
	sumhero = hero.y + hero.x;
	diffhero = hero.y - hero.x;
    }
    pp = INDEX(hero.y, hero.x);
    pch = pp->p_ch;
    pfl = pp->p_flags;

    for (y = sy; y <= ey; y++)
	if (y > 0 && y < NUMLINES - 1) for (x = sx; x <= ex; x++)
	{
	    if (x < 0 || x >= NUMCOLS)
		continue;
	    if (!on(player, ISBLIND))
	    {
		if (y == hero.y && x == hero.x)
		    continue;
	    }

	    pp = INDEX(y, x);
	    ch = pp->p_ch;
	    if (ch == ' ')		/* nothing need be done with a ' ' */
		    continue;
	    fp = &pp->p_flags;
	    if (pch != DOOR && ch != DOOR)
		if ((pfl & F_PASS) != (*fp & F_PASS))
		    continue;
	    if (((*fp & F_PASS) || ch == DOOR) && 
		 ((pfl & F_PASS) || pch == DOOR))
	    {
		if (hero.x != x && hero.y != y &&
		    !step_ok(chat(y, hero.x)) && !step_ok(chat(hero.y, x)))
			continue;
	    }

	    if ((tp = pp->p_monst) == NULL)
		;
	    else
		if (on(player, SEEMONST) && on(*tp, ISINVIS))
		{
		    if (door_stop && !firstmove)
			running = FALSE;
		    continue;
		}
		else
		{
		    if (wakeup)
			wake_monster(y, x);
		    if (see_monst(tp))
		    {
			ch = tp->t_disguise;
		    }
		}
	    if (on(player, ISBLIND) && (y != hero.y || x != hero.x))
		continue;

	    move(y, x);


	    if (tp != NULL || ch != (chtype)jrogue_mvinch_char(y, x))
		jrogue_addch_map(ch);

	    if (door_stop && !firstmove && running)
	    {
		switch (runch)
		{
		    case 'h':
			if (x == ex)
			    continue;
		    when 'j':
			if (y == sy)
			    continue;
		    when 'k':
			if (y == ey)
			    continue;
		    when 'l':
			if (x == sx)
			    continue;
		    when 'y':
			if ((y + x) - sumhero >= 1)
			    continue;
		    when 'u':
			if ((y - x) - diffhero >= 1)
			    continue;
		    when 'n':
			if ((y + x) - sumhero <= -1)
			    continue;
		    when 'b':
			if ((y - x) - diffhero <= -1)
			    continue;
		}
		switch (ch)
		{
		    case DOOR:
			if (x == hero.x || y == hero.y)
			    running = FALSE;
			break;
		    case PASSAGE:
			if (x == hero.x || y == hero.y)
			    passcount++;
			break;
		    case FLOOR:
		    case '|':
		    case '-':
		    case ' ':
			break;
		    default:
			running = FALSE;
			break;
		}
	    }
	}
    if (door_stop && !firstmove && passcount > 1)
	running = FALSE;
    if (!running || !jump)
    {
	move(hero.y, hero.x);
	if (was_trapped > TRUE)
	    standout2();
	jrogue_addch_map(PLAYER);
	if (was_trapped > TRUE)
	    standend2();
	was_trapped = FALSE;
    }
# ifdef DEBUG
    done = FALSE;
# endif /* DEBUG */
}

/*
 * erase_lamp:
 *	Erase the area shown by a lamp in a dark room.
 */

static int
dark_lamp_restore_ch(int y, int x, int oldch)
{
    int ch;

    if (y < 0 || y >= STATLINE || x < 0 || x >= NUMCOLS)
	return ' ';

    /*
     * Match the original DOS 1.48 dark-room display logic: leaving the
     * lamp-lit area blanks only visible FLOOR cells.  Other glyphs that
     * remain on the physical screen are left alone.
     */
    if (oldch == FLOOR)
	return ' ';

    ch = chat(y, x);
    switch (ch)
    {
	case '|':
	case '-':
	case DOOR:
	case PASSAGE:
	case STAIRS:
	    return ch;
	default:
	    return oldch;
    }
}

void
erase_lamp(const coord *pos, const struct room *rp)
{
    int y, x, ey, sy, ex;
    int oldch, newch;

    if (rp == NULL || !((rp->r_flags & (ISGONE|ISDARK)) == ISDARK
	&& !on(player,ISBLIND)))
	    return;

    ey = pos->y + 1;
    ex = pos->x + 1;
    sy = pos->y - 1;
    for (x = pos->x - 1; x <= ex; x++)
	for (y = sy; y <= ey; y++)
	{
	    if (y < 0 || y >= STATLINE || x < 0 || x >= NUMCOLS)
		continue;
	    if (y == hero.y && x == hero.x)
		continue;

	    oldch = jrogue_mvinch_char(y, x);
	    if (oldch == ' ' || oldch == PLAYER)
		continue;

	    newch = dark_lamp_restore_ch(y, x, oldch);
	    if (oldch == newch)
		continue;

	    move(y, x);
	    if (newch == ' ')
		addch2(' ');
	    else
		jrogue_addch_map(newch);
	}
}

/*
 * show_floor:
 *	Should we show the floor in her room at this time?
 */
int
show_floor(void)
{
    return TRUE;
}

/*
 * goodch:
 *	Decide how good a magic object is and return the DOS-style
 *	character used by the magic-detection temporary display.
 */
int
goodch(const THING *obj)
{
    int ch = MAGIC;

    if (obj->o_flags & ISCURSED)
	ch = BMAGIC;
    switch (obj->o_type)
    {
	case ARMOR:
	    if (obj->o_arm > a_class[obj->o_which])
		ch = BMAGIC;
	    break;
	case WEAPON:
	    if (obj->o_hplus < 0 || obj->o_dplus < 0)
		ch = BMAGIC;
	    break;
	case SCROLL:
	    switch (obj->o_which)
	    {
		case S_SLEEP:
		case S_CREATE:
		case S_AGGR:
		    ch = BMAGIC;
		    break;
	    }
	    break;
	case POTION:
	    switch (obj->o_which)
	    {
		case P_CONFUSE:
		case P_PARALYZE:
		case P_POISON:
		case P_BLIND:
		    ch = BMAGIC;
		    break;
	    }
	    break;
	case STICK:
	    switch (obj->o_which)
	    {
		case WS_HASTE_M:
		case WS_TELTO:
		    ch = BMAGIC;
		    break;
	    }
	    break;
	case RING:
	    switch (obj->o_which)
	    {
		case R_PROTECT:
		case R_ADDSTR:
		case R_ADDDAM:
		case R_ADDHIT:
		    if (obj->o_arm < 0)
			ch = BMAGIC;
		    break;
		case R_AGGR:
		case R_TELEPORT:
		    ch = BMAGIC;
		    break;
	    }
	    break;
    }
    return ch;
}

/*
 * find_obj:
 *	Find the unclaimed object at y, x
 */
THING *
find_obj(int y, int x)
{
    THING *obj;

    for (obj = lvl_obj; obj != NULL; obj = next(obj))
    {
	if (obj->o_pos.y == y && obj->o_pos.x == x)
		return obj;
    }
#ifdef MASTER
    snprintf(prbuf, PRBUF_SIZE, "Non-object %d,%d", y, x);
    msg("%s", prbuf);
    return NULL;
#else
    /* NOTREACHED */
    return NULL;
#endif
}

/*
 * eat:
 *	She wants to eat something, so let her try
 */

void
eat(void)
{
    THING *obj;

    if ((obj = get_item(JSTR("食べるもの", "eat"), FOOD)) == NULL)
	return;
    if (obj->o_type != FOOD)
    {
	if (!terse)
	    msg(JSTR("うげっ、そんなものを食べたら病気になってしまう。", "ugh, you would get ill if you ate that"));
	else
	    msg(JSTR("それは食べられない。", "that's inedible!"));
	return;
    }
    if (food_left < 0)
	food_left = 0;
    if (food_left > STOMACHSIZE - 20)
	no_command += 2 + rnd(5);
    if ((food_left += HUNGERTIME - 200 + rnd(400)) > STOMACHSIZE)
	food_left = STOMACHSIZE;
    hungry_state = 0;
    if (obj == cur_weapon)
	cur_weapon = NULL;
    if (obj->o_which == 1)
	msg(JSTR("ああ、美味しい%sだった。", "my, that was a yummy %s"), fruit);
    else
	if (rnd(100) > 70)
	{
	    pstats.s_exp++;
	    msg(JSTR("%s、ひどい味だ。", "%s, this food tastes awful"), JSTR("げげっ", "yuk"));
	    check_level();
	}
	else
	    msg(JSTR("%s、おいしかった。", "%s, that tasted good"), JSTR("うーむ", "yum"));
    if (no_command)
	msg(JSTR("食べ過ぎて眠ってしまった。", "You feel bloated and fall asleep"));
    leave_pack(obj, FALSE, FALSE);
}

/*
 * check_level:
 *	Check to see if the guy has gone up a level.
 */

void
check_level(void)
{
    int i, add, olevel;

    for (i = 0; e_levels[i] != 0; i++)
	if (e_levels[i] > pstats.s_exp)
	    break;
    i++;
    olevel = pstats.s_lvl;
    pstats.s_lvl = i;
    if (i > olevel)
    {
	add = roll(i - olevel, 10);
	max_hp += add;
	pstats.s_hpt += add;
	msg(JSTR("そして \"%s\" の位に達した。", "and achieve the rank of \"%s\""),
	    JSTR(rank_names_ja[i - 1], rank_names_en[i - 1]));
    }
}

/*
 * chg_str:
 *	used to modify the playes strength.  It keeps track of the
 *	highest it has been, just in case
 */

void
chg_str(int amt)
{
    int comp;

    if (amt == 0)
	return;
    add_str(&pstats.s_str, amt);
    comp = pstats.s_str;
    if (ISRING(LEFT, R_ADDSTR))
	add_str(&comp, -cur_ring[LEFT]->o_arm);
    if (ISRING(RIGHT, R_ADDSTR))
	add_str(&comp, -cur_ring[RIGHT]->o_arm);
    if (comp > max_stats.s_str)
	max_stats.s_str = comp;
}

/*
 * add_str:
 *	Perform the actual add, checking upper and lower bound limits
 */
void
add_str(int *sp, int amt)
{
    if ((*sp += amt) < 3)
	*sp = 3;
    else if (*sp > 31)
	*sp = 31;
}

/*
 * add_haste:
 *	Add a haste to the player
 */
int
add_haste(int potion)
{
    if (on(player, ISHASTE))
    {
	no_command += rnd(8);
	player.t_flags &= ~(ISRUN|ISHASTE);
	extinguish(nohaste);
	msg(JSTR("消耗して気絶した。", "you faint from exhaustion"));
	return FALSE;
    }
    else
    {
	player.t_flags |= ISHASTE;
	if (potion)
	    fuse(nohaste, 0, rnd(4)+10, AFTER);
	return TRUE;
    }
}

/*
 * aggravate:
 *	Aggravate all the monsters on this level
 */

void
aggravate(void)
{
    THING *mp;

    for (mp = mlist; mp != NULL; mp = next(mp))
	runto(&mp->t_pos);
}

/*
 * vowelstr:
 *      For printfs: if string starts with a vowel, return "n" for an
 *	"an".
 */
char *
vowelstr(const char *str)
{
    (void)str;
//    switch (*str)
//    {
//	case 'a': case 'A':
//	case 'e': case 'E':
//	case 'i': case 'I':
//	case 'o': case 'O':
//	case 'u': case 'U':
//	    return "n";
//	default:
	    return "";
//    }
}

/* 
 * is_current:
 *	See if the object is one of the currently used items
 */
int
is_current(const THING *obj)
{
    if (obj == NULL)
	return FALSE;
    if (obj == cur_armor || obj == cur_weapon || obj == cur_ring[LEFT]
	|| obj == cur_ring[RIGHT])
    {
	if (!terse)
	    addmsg(JSTR("それは", "That's already "));
	msg(JSTR("もう使っている。", "in use"));
	return TRUE;
    }
    return FALSE;
}

/*
 * get_dir:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
int
get_dir(void)
{
    const char *prompt;
    int gotit;
    static coord last_delt= {0,0};

    if (again && last_dir != '\0')
    {
	delta.y = last_delt.y;
	delta.x = last_delt.x;
	dir_ch = last_dir;
    }
    else
    {
	if (!terse)
	    msg(prompt = JSTR("どちらの方角？", "which direction? "));
	else
	    msg(prompt = JSTR("方角は？", "direction: "));
	jrogue_show_cursor(TRUE);
	do
	{
	    gotit = TRUE;
	    switch (dir_ch = readchar())
	    {
		case 'h': case'H':
		case DOSCMD_CTL_LEFT:
		    delta.y =  0; delta.x = -1; dir_ch = 'h'; break;
		case 'j': case'J':
		case DOSCMD_CTL_DOWN:
		    delta.y =  1; delta.x =  0; dir_ch = 'j'; break;
		case 'k': case'K':
		case DOSCMD_CTL_UP:
		    delta.y = -1; delta.x =  0; dir_ch = 'k'; break;
		case 'l': case'L':
		case DOSCMD_CTL_RIGHT:
		    delta.y =  0; delta.x =  1; dir_ch = 'l'; break;
		case 'y': case'Y':
		case DOSCMD_CTL_UPLEFT:
		    delta.y = -1; delta.x = -1; dir_ch = 'y'; break;
		case 'u': case'U':
		case DOSCMD_CTL_UPRIGHT:
		    delta.y = -1; delta.x =  1; dir_ch = 'u'; break;
		case 'b': case'B':
		case DOSCMD_CTL_DOWNLEFT:
		    delta.y =  1; delta.x = -1; dir_ch = 'b'; break;
		case 'n': case'N':
		case DOSCMD_CTL_DOWNRIGHT:
		    delta.y =  1; delta.x =  1; dir_ch = 'n'; break;
		when ESCAPE:
		    last_dir = '\0';
		    reset_last();
		    msg(""); 
	jrogue_restore_cursor();
		    return FALSE;
		otherwise:
		    mpos = 0;
		    msg("%s", prompt);
		    gotit = FALSE;
	    }
	} until (gotit);
	if (isupper(dir_ch))
	    dir_ch = tolower(dir_ch);
	last_dir = dir_ch;
	last_delt.y = delta.y;
	last_delt.x = delta.x;
    }
    if (on(player, ISHUH) && rnd(5) == 0)
	do
	{
	    delta.y = rnd(3) - 1;
	    delta.x = rnd(3) - 1;
	} while (delta.y == 0 && delta.x == 0);
    mpos = 0;
    msg("");
	jrogue_restore_cursor();
    return TRUE;
}

/*
 * sign:
 *	Return the sign of the number
 */
int
sign(int nm)
{
    if (nm < 0)
	return -1;
    else
	return (nm > 0);
}

/*
 * spread:
 *	Give a spread around a given number (+/- 20%)
 */
int
spread(int nm)
{
    return nm - nm / 20 + rnd(nm / 10);
}

/*
 * call_it:
 *	Call an object something after use.
 */

void
call_it(struct obj_info *info)
{
    if (info->oi_know)
    {
	if (info->oi_guess)
	{
	    free(info->oi_guess);
	    info->oi_guess = NULL;
	}
    }
    else if (!info->oi_guess)
    {
	msg(terse ? JSTR("呼び名は？", "call it: ") : JSTR("どんな呼び名をつける？", "what do you want to call it? "));
	jrogue_show_cursor(TRUE);
	jrogue_set_normal_attr();
	if (get_str(prbuf, stdscr) == NORM)
	{
	    if (info->oi_guess != NULL)
		free(info->oi_guess);
	    info->oi_guess = malloc(strlen(prbuf) + 1);
		if (info->oi_guess != NULL)
			jrogue_strlcpy(info->oi_guess, prbuf, MAXSTR);
	}
	msg("");
	jrogue_restore_cursor();
    }
}

/*
 * rnd_thing:
 *	Pick a random thing appropriate for this level
 */
int
rnd_thing(void)
{
    int i;
    int thing_list[] = {
	POTION, SCROLL, RING, STICK, FOOD, WEAPON, ARMOR, STAIRS, GOLD, AMULET
    };

    if (level >= AMULETLEVEL)
        i = rnd(sizeof thing_list / sizeof (int));
    else
        i = rnd(sizeof thing_list / sizeof (int) - 1);
    return thing_list[i];
}

/*
 str str:
 *	Choose the first or second string depending on whether it the
 *	player is tripping
 */
const char *
choose_str(const char *ts, const char *ns)
{
    (void) ts;
    return ns;
}
