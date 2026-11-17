/*
 * Read and execute the user commands
 *
 * @(#)command.c	4.73 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"

int
precom(int ch)
{
    switch (ch)
    {
	case 'b': case 'h': case 'j': case 'k':
	case 'l': case 'n': case 'u': case 'y':
	    ch += (CTRL('A') - 'a');
/*	    ch = toupper(ch);	*/
    }
    return(ch);
}

static int
dos_is_dir(int ch)
{
    return (ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l'
	 || ch == 'y' || ch == 'u' || ch == 'b' || ch == 'n');
}

static int
dos_ctrl_dir_to_run(int ch)
{
    switch (ch)
    {
	case DOSCMD_CTL_LEFT: return 'H';
	case DOSCMD_CTL_DOWN: return 'J';
	case DOSCMD_CTL_UP: return 'K';
	case DOSCMD_CTL_RIGHT: return 'L';
	case DOSCMD_CTL_UPLEFT: return 'Y';
	case DOSCMD_CTL_UPRIGHT: return 'U';
	case DOSCMD_CTL_DOWNLEFT: return 'B';
	case DOSCMD_CTL_DOWNRIGHT: return 'N';
    }
    return 0;
}

void
jrogue_set_macro(const char *src)
{
    char buf[41];
    char *dst;
    size_t left;

    if (src == NULL)
	src = "";
    dst = buf;
    left = sizeof buf - 1;
    while (*src != '\0' && left > 0)
    {
	/* Prevent a macro from recursively running itself. */
	if ((unsigned char)*src != 'x' && (unsigned char)*src != CTRL('F'))
	{
	    *dst++ = *src;
	    left--;
	}
	src++;
    }
    *dst = '\0';
    jrogue_strlcpy(dos_macro, buf, 41);
    dos_typeahead = "";
}

static void
dos_define_macro(void)
{
    msg(JSTR("マクロは%sです。新しいマクロを入力してください：",
	     "Macro was %s, enter new macro: "),
	dos_macro[0] != '\0' ? dos_macro : JSTR("（なし）", "(none)"));
    prbuf[0] = '\0';
    jrogue_show_cursor(TRUE);
    if (get_str(prbuf, stdscr) == NORM)
	jrogue_set_macro(prbuf);
    msg("");
    jrogue_restore_cursor();
}

static int
dos_dodos(WINDOW *win, const char *com)
{
    while (*com != '\0' && isspace((unsigned char)*com))
	com++;
    if (*com == '\0')
	return TRUE;
    if (strcmp(com, "rogue") == 0 || strcmp(com, "ROGUE") == 0)
	return FALSE;
    if (isalpha((unsigned char)com[0]) && com[1] == ':' && com[2] == '\0')
    {
	wprintw(win, "\nInvalid drive specification\n");
	return TRUE;
    }
    wprintw(win, "\nBad command or file name\n");
    return TRUE;
}

static void
dos_fake_dos(void)
{
    int row;
    WINDOW *win;

    /*
     * Keep the game screen in stdscr intact.  The DOS prompt is a temporary
     * overlay on hw; otherwise returning from fake DOS leaves the typed
     * command in stdscr and the dungeon cannot be repainted correctly.
     */
    win = hw != NULL ? hw : stdscr;
    wclear(win);
    wmove(win, 0, 0);
    jrogue_show_cursor(TRUE);
    row = 0;
    for (;;)
    {
	if (row >= _LINES - 2)
	{
	    wclear(win);
	    row = 0;
	}
	wmove(win, row, 0);
	wprintw(win, "\nC>");
	wclrtoeol(win);
	wrefresh(win);
	prbuf[0] = '\0';
	if (get_str(prbuf, win) == QUIT)
	    break;
	if (!dos_dodos(win, prbuf))
	    break;
	row = getcury(win) + 1;
    }
    jrogue_restore_cursor();
    clearok(curscr, TRUE);
    touchwin(stdscr);
    wrefresh(stdscr);
    msg("");
}
/*
 * command:
 *	Process the user commands
 */
void
command(void)
{
    int ch;
    int ntimes = 1;			/* Number of player moves */
    int *fp;
    THING *mp;
    static int countch, direction, newcount = FALSE;
    static int prerun = 0;
    static int dos_last_count = 0;
    static int dos_last_take = TRUE;
    int dos_do_take = TRUE;
    int dos_fast = FALSE;

    if (on(player, ISHASTE))
	ntimes = rnd(2) + 2;
    /*
     * Let the daemons start up
     */
    do_fuses(BEFORE);
    do_daemons(BEFORE);
    while (ntimes--)
    {
	if (color_mode != color_mode2)
	    refresh2();
	again = FALSE;
	if (has_hit)
	{
	    endmsg();
	    has_hit = FALSE;
	}
	/*
	 * these are illegal things for the player to be, so if any are
	 * set, someone's been poking in memeory
	 */
	if (on(player, ISSLOW|ISGREED|ISINVIS|ISREGEN|ISTARGET))
	    exit(1);

	look(TRUE);
	if (!running)
	    door_stop = FALSE;
	status();
	lastscore = purse;
	move(hero.y, hero.x);
	if (!((running || count) && jump))
	{
	    jrogue_restore_cursor();
	    refresh();			/* Draw screen */
	}
	if (running && !jump)
	    msleep(24);
	take = 0;
	after = TRUE;
	/*
	 * Read command or continue run
	 */
#ifdef MASTER
	if (wizard)
	    noscore = TRUE;
#endif
	if (!no_command)
	{
	    if (running || to_death)
	    {
		ch = runch;
		if (dos_keys)
		    dos_do_take = dos_last_take;
	    }
	    else if (count)
	    {
		ch = countch;
		if (dos_keys)
		    dos_do_take = dos_last_take;
	    }
	    else
	    {
		ch = readchar();
		if (!dos_keys && !count && prerun)
		    ch = precom(ch);
		move_on = FALSE;
		if (mpos != 0)		/* Erase message if its there */
		{
		    if (ch != '.')
			msg("");
		}
	    }
	}
	else
	    ch = '.';
	if (no_command)
	{
	    if (--no_command == 0)
	    {
		player.t_flags |= ISRUN;
		msg(JSTR("また動けるようになった。", "you can move again"));
	    }
	}
	else
	{
	    /*
	     * check for prefixes
	     */
	    newcount = FALSE;
	    if (dos_keys)
	    {
		dos_fast = FALSE;
		if (!(running || count || to_death))
		{
		    dos_do_take = TRUE;
		    for (;;)
		    {
			if (ch == CTRL('H'))
			    ch = 'h';
			if (ch == '+')
			{
			    ch = 't';
			    break;
			}
			if (ch == '-')
			{
			    ch = 'z';
			    break;
			}
			if (isdigit(ch))
			{
			    int ncount;

			    count = 0;
			    newcount = TRUE;
			    while (isdigit(ch))
			    {
				ncount = count * 10 + (ch - '0');
				if (ncount > 0 && ncount < 10000)
				    count = ncount;
				ch = readchar();
				if (ch == CTRL('H'))
				    ch = 'h';
			    }
			    continue;
			}
			if (ch == 'f')
			{
			    dos_fast = !dos_fast;
			    ch = readchar();
			    continue;
			}
			if (ch == 'g')
			{
			    dos_do_take = FALSE;
			    ch = readchar();
			    continue;
			}
			if (ch == 'a')
			{
			    if (last_comm == '\0')
			    {
				msg(JSTR("まだコマンドを入力していない。", "you haven't typed a command yet"));
				after = FALSE;
				ch = ESCAPE;
			    }
			    else
			    {
				ch = last_comm;
				count = dos_last_count;
				dos_do_take = dos_last_take;
				newcount = (count != 0);
				again = TRUE;
			    }
			    break;
			}
			if (ch == ' ')
			{
			    ch = readchar();
			    continue;
			}
			break;
		    }
		    if (dos_fast && dos_is_dir(ch))
		    {
			if (!on(player, ISBLIND))
			{
			    door_stop = TRUE;
			    firstmove = TRUE;
			}
			ch = toupper(ch);
		    }
		    if (!again && ch != ESCAPE && ch != INVALID_INPUT)
		    {
			dos_last_count = count;
			dos_last_take = dos_do_take;
		    }
		}
		if (newcount)
		{
		    countch = ch;
		    switch (ch)
		    {
			case '.': case 'b': case 'h': case 'j': case 'k':
			case 'l': case 'n': case 'q': case 'r': case 's':
			case 't': case 'u': case 'y': case 'z':
			case 'B': case 'H': case 'J': case 'K': case 'L':
			case 'N': case 'U': case 'Y':
#ifdef MASTER
			case 'C':
#endif
			    break;
			default:
			    count = 0;
		    }
		}
	    }
	    else
	    {
		if (!(running || count || to_death))
		{
		    for (;;)
		    {
			if (ch == '-')
			{
			    prerun = 1 - prerun;
			    ch = readchar();
			    if (!count && prerun)
				ch = precom(ch);
			    continue;
			}
			if (isdigit(ch))
			{
			    int ncount;

			    count = 0;
			    newcount = TRUE;
			    while (isdigit(ch))
			    {
				ncount = count * 10 + (ch - '0');
				if (ncount > 0 && ncount < 10000)
				    count = ncount;
				ch = readchar();
				if (!count && prerun)
				    ch = precom(ch);
			    }
			    continue;
			}
			break;
		    }
		}
		if (newcount)
		{
		    countch = ch;
		    /*
		     * turn off count for commands which don't make sense
		     * to repeat
		     */
		    switch (ch)
		    {
			case CTRL('B'): case CTRL('H'): case CTRL('J'):
			case CTRL('K'): case CTRL('L'): case CTRL('N'):
			case CTRL('U'): case CTRL('Y'):
			case '.': case 'a': case 'b': case 'h': case 'j':
			case 'k': case 'l': case 'm': case 'n': case 'q':
			case 'r': case 's': case 't': case 'u': case 'y':
			case 'z': case 'B': case 'C': case 'H': case 'I':
			case 'J': case 'K': case 'L': case 'N': case 'U':
			case 'Y':
#ifdef MASTER
			case CTRL('D'): case CTRL('A'):
#endif
			    break;
			default:
			    count = 0;
		    }
		}
	    }
	    if (dos_keys && !again && ch != ESCAPE && ch != INVALID_INPUT && !(running || to_death))
	    {
		dos_last_count = count;
		dos_last_take = dos_do_take;
	    }
	    /*
	     * execute a command
	     */
	    if (count && !running)
		count--;
	    if (dos_keys)
	    {
		if (!again && ch != ESCAPE && ch != INVALID_INPUT
		&& !(running || to_death) && (!count || newcount))
		{
		    l_last_comm = last_comm;
		    l_last_dir = last_dir;
		    l_last_pick = last_pick;
		    last_comm = ch;
		    last_dir = '\0';
		    last_pick = NULL;
		}
		/* Keep DOS-style keys from colliding with classic commands. */
		switch (ch)
		{
		    case 'F':	    ch = DOSCMD_DEFINE_MACRO; break;
		    case CTRL('F'): ch = DOSCMD_RUN_MACRO; break;
		    case CTRL('T'): ch = DOSCMD_TOGGLE_TERSE; break;
		    case CTRL('L'): ch = DOSCMD_SCREEN_CHECK; break;
		}
	    }
	    else if (ch != 'a' && !again && ch != ESCAPE && ch != INVALID_INPUT && !(running || count || to_death))
	    {
		l_last_comm = last_comm;
		l_last_dir = last_dir;
		l_last_pick = last_pick;
		last_comm = ch;
		last_dir = '\0';
		last_pick = NULL;
	    }
over:
	    switch (ch)
	    {
		case ',': {
		    THING *obj = NULL;
		    int found = 0;

		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
			break;
		    }
		    for (obj = lvl_obj; obj != NULL; obj = next(obj))
		    {
			if (obj->o_pos.y == hero.y && obj->o_pos.x == hero.x)
			{
			    found=1;
			    break;
			}
		    }

		    if (found) {
			pick_up(obj->o_type);
		    }
		    else {
			/* Compose complete sentences in both languages and modes. */
			msg(terse
			    ? JSTR("拾えるものはない。", "nothing to pick up")
			    : JSTR("ここには拾えるものは何もない。",
			           "there is nothing here to pick up"));
		    }
		}
		when '!':
		    if (dos_keys)
		    {
			after = FALSE;
			dos_fake_dos();
		    }
		    else
			shell();
		when 'h': do_move(0, -1);
		when 'j': do_move(1, 0);
		when 'k': do_move(-1, 0);
		when 'l': do_move(0, 1);
		when 'y': do_move(-1, -1);
		when 'u': do_move(-1, 1);
		when 'b': do_move(1, -1);
		when 'n': do_move(1, 1);
		when 'H': do_run('h');
		when 'J': do_run('j');
		when 'K': do_run('k');
		when 'L': do_run('l');
		when 'Y': do_run('y');
		when 'U': do_run('u');
		when 'B': do_run('b');
		when 'N': do_run('n');
		/* Normal running must not enter the Ctrl+direction handler. */
		break;
		case DOSCMD_CTL_LEFT: case DOSCMD_CTL_DOWN: case DOSCMD_CTL_UP:
		case DOSCMD_CTL_RIGHT: case DOSCMD_CTL_UPLEFT: case DOSCMD_CTL_UPRIGHT:
		case DOSCMD_CTL_DOWNLEFT: case DOSCMD_CTL_DOWNRIGHT:
		{
		    if (!dos_keys)
		    {
			after = FALSE;
			illcom(ch);
			break;
		    }
		    if (!on(player, ISBLIND))
		    {
			door_stop = TRUE;
			firstmove = TRUE;
		    }
		    if (count && !newcount)
			ch = direction;
		    else
		    {
			ch = dos_ctrl_dir_to_run(ch);
			direction = ch;
		    }
		    goto over;
		}
		when CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
		case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
		{
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
			break;
		    }
		    if (!on(player, ISBLIND))
		    {
			door_stop = TRUE;
			firstmove = TRUE;
		    }
		    if (count && !newcount)
			ch = direction;
		    else
		    {
			ch += ('A' - CTRL('A'));
			direction = ch;
		    }
		    goto over;
		}
		case 'F':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
			break;
		    }
		    kamikaze = TRUE;
		    /* FALLTHROUGH */
		case 'f':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
			break;
		    }
		    if (!get_dir())
		    {
			after = FALSE;
			break;
		    }
		    delta.y += hero.y;
		    delta.x += hero.x;
		    if ( ((mp = moat(delta.y, delta.x)) == NULL)
			|| ((!see_monst(mp)) && !on(player, SEEMONST)))
		    {
			msg(terse
			    ? JSTR("怪物がいない。", "no monster there")
			    : JSTR("その方向には怪物がいない。", "I see no monster there"));
			after = FALSE;
		    }
		    else if (diag_ok(&hero, &delta))
		    {
			to_death = TRUE;
			max_hit = 0;
			mp->t_flags |= ISTARGET;
			runch = ch = dir_ch;
			goto over;
		    }
		when 't':
		    if (!get_dir())
			after = FALSE;
		    else
			missile(delta.y, delta.x);
		when 'a':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else if (last_comm == '\0')
		    {
			msg(JSTR("まだコマンドを入力していない。", "you haven't typed a command yet"));
			after = FALSE;
		    }
		    else
		    {
			ch = last_comm;
			again = TRUE;
			goto over;
		    }
		when 'q': quaff();
		when 'Q':
		    after = FALSE;
		    q_comm = TRUE;
		    quit(0);
		    q_comm = FALSE;
		when 'i': after = FALSE; inventory(pack, 0);
		when 'I':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
		    {
			after = FALSE;
			picky_inven();
		    }
		when 'd': drop();
		when 'r': read_scroll();
		when 'e': eat();
		when 'w': wield();
		when 'W': wear();
		when 'T': take_off();
		when 'P': ring_on();
		when 'R': ring_off();
		when 'o':
		    option();
		    after = FALSE;
		when 'c':
		    call();
		    after = FALSE;
		when '>': after = FALSE; d_level();
		when '<': after = FALSE; u_level();
		when '?': after = FALSE; help();
		when '/': after = FALSE; identify();
		when 's': search();
		when 'z':
		    if (get_dir())
			do_zap();
		    else
			after = FALSE;
		when 'D': after = FALSE; discovered();
		when CTRL('P'):
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
		    {
			after = FALSE;
			msg("%s", huh);
		    }
		when CTRL('R'):
		    after = FALSE;
		    if (dos_keys)
			msg("%s", huh);
		    else
		    {
			clearok(curscr,TRUE);
			wrefresh(curscr);
		    }
		when DOSCMD_TOGGLE_TERSE:
		    after = FALSE;
		    terse = !terse;
		    msg(terse ? JSTR("では、手短に話そう。", "Ok, I'll be brief") :
			JSTR("よし、また大げさに話せる。", "Goodie, I can use big words again!"));
		when DOSCMD_SCREEN_CHECK:
		    after = FALSE;
		    msg(JSTR("画面はちゃんとしているように見える。", "the screen looks fine to me (jll was here)"));
		when 'v':
		    after = FALSE;
		    msg("%s", version);
		when 'S': 
		    after = FALSE;
		    save_game();
		when '.': ;			/* Rest command */
		when ' ': after = FALSE;	/* "Legal" illegal command */
		when '^':
		    after = FALSE;
		    if (get_dir()) {
			delta.y += hero.y;
			delta.x += hero.x;
			fp = &flat(delta.y, delta.x);
			if (chat(delta.y, delta.x) != TRAP)
			    msg(JSTR("その方向にわなはない。", "no trap there."));
			else {
			    msg(JSTR("%sを見つけた。", "you found %s"),
				tr_name[*fp & F_TMASK]);
			    *fp |= F_SEEN;
			}
		    }
#ifdef MASTER
		when '+':
		    after = FALSE;
		    if (!dos_keys && wizard)
		    {
			wizard = FALSE;
			turn_see(TRUE);
			msg(JSTR("もはや、魔法使いではない。", "not wizard any more"));
		    }
		    else if (!dos_keys)
		    {
			wizard = passwd();
			if (wizard) 
			{
			    noscore = TRUE;
			    turn_see(FALSE);
			    msg(JSTR("あなたは洞窟（#%d）の中で、突然ケン・アーノルドのように賢くなった。", "you are suddenly as smart as Ken Arnold in dungeon #%d"), dnum);
			}
			else
			    msg(JSTR("残念でした。", "sorry"));
		    }
		    else
			illcom(ch);
#endif
		when ESCAPE:	/* Escape */
		    door_stop = FALSE;
		    count = 0;
		    after = FALSE;
		    again = FALSE;
		when 'm':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
		    {
			move_on = TRUE;
			if (!get_dir())
			    after = FALSE;
			else
			{
			    ch = dir_ch;
			    countch = dir_ch;
			    goto over;
			}
		    }
		when ')':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
			current(cur_weapon, JSTR("使っている", "wielding"), NULL);
		when ']':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
			current(cur_armor, JSTR("着ている", "wearing"), NULL);
		when '=':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
		    {
			current(cur_ring[LEFT], JSTR("はめている", "wearing"),
					terse ? JSTR("（左手）", "(L)") : JSTR("（左の手）", "on left hand"));
			current(cur_ring[RIGHT], JSTR("はめている", "wearing"),
					terse ? JSTR("（右手）", "(R)") : JSTR("（右の手）", "on right hand"));
		    }
		when '@':
		    if (dos_keys)
		    {
			after = FALSE;
			illcom(ch);
		    }
		    else
		    {
			stat_msg = TRUE;
			status();
			stat_msg = FALSE;
			after = FALSE;
		    }
		when 'M':
		    after = FALSE;
		    if (dos_keys)
			illcom(ch);
		    else
			dos_define_macro();
		when 'x':
		    after = FALSE;
		    if (dos_keys)
			illcom(ch);
		    else
			dos_typeahead = dos_macro;
		when DOSCMD_DEFINE_MACRO:
		    after = FALSE;
		    dos_define_macro();
		when DOSCMD_RUN_MACRO:
		    after = FALSE;
		    dos_typeahead = dos_macro;
		otherwise:
		    after = FALSE;
#ifdef MASTER
		    if (wizard)
		    {
			if (dos_keys)
			{
			    switch (ch)
			    {
				case 'C': create_obj(); break;
				default: illcom(ch); break;
			    }
			}
			else
			{
			    switch (ch)
			    {
				case '|': msg("@ %d,%d", hero.y, hero.x); break;
				when 'C': create_obj();
				when '$': msg(JSTR("荷物の数：%d", "inpack = %d"), inpack);
				when CTRL('G'): inventory(lvl_obj, 0);
				when CTRL('W'): whatis(FALSE, 0);
				when CTRL('D'): level++; new_level();
				when CTRL('A'): level--; new_level();
				when CTRL('F'): show_map();
				when CTRL('T'): teleport();
				when CTRL('E'): msg(JSTR("食べものの残り：%d", "food left: %d"), food_left);
				when CTRL('Q'): add_pass();
				when CTRL('X'): turn_see(on(player, SEEMONST));
				when '~':
				{
				    THING *item;

				    if ((item = get_item(JSTR("チャージするつえ/魔法の棒", "charge"), STICK)) != NULL)
					item->o_charges = 10000;
				}
				when CTRL('I'):
				{
				    int i;
				    THING *obj;

				    for (i = 0; i < 9; i++)
					raise_level();
				    /*
				     * Give him a sword (+1,+1)
				     */
				    obj = new_item();
				    if (obj == NULL)
					break;
				    init_weapon(obj, TWOSWORD);
				    obj->o_hplus = 1;
				    obj->o_dplus = 1;
				    add_pack(obj, TRUE);
				    cur_weapon = obj;
				    /*
				     * And his suit of armor
				     */
				    obj = new_item();
				    if (obj == NULL)
					break;
				    obj->o_type = ARMOR;
				    obj->o_which = PLATE_MAIL;
				    obj->o_arm = -5;
				    obj->o_flags |= ISKNOW;
				    obj->o_count = 1;
				    obj->o_group = 0;
				    cur_armor = obj;
				    add_pack(obj, TRUE);
				}
				when '*' :
				    pr_list();
				otherwise:
				    illcom(ch);
			    }
			}
			jrogue_restore_cursor();
		    }
		    else
#endif
			illcom(ch);
	    }
	    /*
	     * turn off flags if no longer needed
	     */
	    if (!running)
		door_stop = FALSE;
	}
	/*
	 * If he ran into something to take, let him pick it up.
	 */
	if (take != 0 && (!dos_keys || dos_do_take))
	    pick_up(take);
	take = 0;
	if (!running)
	    door_stop = FALSE;
	if (!after)
	    ntimes++;
    }
    do_fuses(AFTER);
    do_daemons(AFTER);
    if (ISRING(LEFT, R_SEARCH))
	search();
    else if (ISRING(LEFT, R_TELEPORT) && rnd(50) == 0)
	teleport();
    if (ISRING(RIGHT, R_SEARCH))
	search();
    else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
	teleport();
}

/*
 * illcom:
 *	What to do with an illegal command
 */
void
illcom(int ch)
{
    save_msg = FALSE;
    count = 0;
    if (ch == INVALID_INPUT)
	msg(JSTR("入力エラーです。", "input error"));
    else
	msg(JSTR("'%s' コマンドが違います。", "illegal command '%s'"), unctrl(ch));
    save_msg = TRUE;
}

/*
 * search:
 *	player gropes about him to find hidden things.
 */
void
search(void)
{
    int y, x;
    int *fp;
    int ey, ex;
    int found;

    if (on(player, ISBLIND))
	return;

    ey = hero.y + 1;
    ex = hero.x + 1;
    found = FALSE;
    for (y = hero.y - 1; y <= ey; y++) 
	for (x = hero.x - 1; x <= ex; x++)
	{
	    if ((y == hero.y && x == hero.x)
		|| y < 0 || y >= STATLINE || x < 0 || x >= NUMCOLS)
		continue;
	    fp = &flat(y, x);
	    if (!(*fp & F_REAL))
		switch (chat(y, x))
		{
		    case '|':
		    case '-':
			if (rnd(5) != 0)
			    break;
			chat(y, x) = DOOR;
foundone:
			found = TRUE;
			*fp |= F_REAL;
			count = FALSE;
			running = FALSE;
			break;
		    case FLOOR:
			if (rnd(2) != 0)
			    break;
			chat(y, x) = TRAP;
			*fp |= F_SEEN;
			msg(JSTR("%sを見つけた。", "you found %s"),
			    tr_name[*fp & F_TMASK]);
			goto foundone;
		}
	}
    if (found)
	look(FALSE);
}

static void
show_help_list(const struct h_list *list, int use_glyphs,
    int interleaved_columns)
{
    const struct h_list *strp;
    int total, rows, columns, per_page, page_start;
    int idx, cnt, row, col, ch;

    total = 0;
    for (strp = list; strp->h_desc != NULL; strp++)
	if (strp->h_print)
	    total++;
    if (total <= 0)
	return;

    rows = _LINES - 1;
    if (rows < 1)
	rows = 1;

    /*
     * DOS 1.48 used one column for screen symbols on 40-column screens.
     * JEC's internal screen is 80 columns wide, regardless of terse.
     * Keep the existing row/column order for both kinds of help.
     */
    columns = 2;
    per_page = rows * columns;
    page_start = 0;

    for (;;)
    {
	wclear(hw);
	jrogue_wset_normal_attr(hw);
	cnt = 0;
	idx = 0;
	for (strp = list; strp->h_desc != NULL; strp++)
	    if (strp->h_print)
	    {
		if (idx++ < page_start)
		    continue;
		if (cnt >= per_page)
		    break;
		if (interleaved_columns)
		{
		    row = cnt / columns;
		    col = (columns == 2 && (cnt % 2) != 0) ? _COLS / 2 : 0;
		}
		else
		{
		    row = cnt % rows;
		    col = cnt >= rows ? _COLS / 2 : 0;
		}
		wmove(hw, row, col);
		if (strp->h_ch)
		{
		    const char *glyph = use_glyphs ? jrogue_color_glyph_for(0, 0, strp->h_ch, FALSE) : NULL;
		    if (use_glyphs)
			jrogue_wset_map_attr(hw, strp->h_ch);
		    if (glyph != NULL)
			waddstr(hw, glyph);
		    else
			waddstr(hw, unctrl(strp->h_ch));
		    if (use_glyphs)
			jrogue_wset_normal_attr(hw);
		}
		waddstr(hw, strp->h_desc);
		cnt++;
	    }
	wmove(hw, _LINES - 1, 0);
	if (page_start + per_page < total)
	{
	    waddstr(hw, JSTR("--スペースで続き、ESCで戻る--",
		"--Press space for more, Esc to continue--"));
	    wrefresh(hw);
	    jrogue_show_cursor(TRUE);
	    do
		ch = wreadchar(hw);
	    while (ch != ' ' && ch != ESCAPE);
	    jrogue_restore_cursor();
	    if (ch == ESCAPE)
		break;
	    page_start += per_page;
	}
	else
	{
	    waddstr(hw, JSTR("--スペースを押してください--",
		"--Press space to continue--"));
	    wrefresh(hw);
	    jrogue_show_cursor(TRUE);
	    wait_for(hw, ' ');
	    jrogue_restore_cursor();
	    break;
	}
    }

    clearok(stdscr, TRUE);
    msg("");
    touchwin(stdscr);
    wrefresh(stdscr);
}

/*
 * help:
 *	Give the command list.
 */
void
help(void)
{
    const struct h_list dos_helpstr[] = {
	{'\0', JSTR("F1	コマンド一覧", "F1	list of commands"), TRUE},
	{'\0', JSTR("F2	画面記号の一覧", "F2	list of symbols"), TRUE},
	{'\0', JSTR("F3	直前のコマンドを繰り返す", "F3	repeat command"), TRUE},
	{'\0', JSTR("F4	最後のメッセージを再表示", "F4	repeat message"), TRUE},
	{'\0', JSTR("F5	持ちものに呼び名をつける", "F5	rename something"), TRUE},
	{'\0', JSTR("F6	発見済み一覧", "F6	recall what's been discovered"), TRUE},
	{'\0', JSTR("F7	持ちものを見る", "F7	inventory of your possessions"), TRUE},
	{'\0', JSTR("F8 <方角>	わなの種類を調べる", "F8 <dir>	identify trap type"), TRUE},
	{'\0', JSTR("F9	The Any Key（マクロ）", "F9	The Any Key (macro)"), TRUE},
	{'\0', JSTR("F10	fake DOS", "F10	Supervisor Key (fake DOS)"), TRUE},
	{'?', JSTR("	コマンド一覧を表示", "	list commands"), TRUE},
	{'/', JSTR("	画面記号の一覧を表示", "	list screen symbols"), TRUE},
	{'h', JSTR("	左に進む", "	left"), TRUE},
	{'j', JSTR("	下に進む", "	down"), TRUE},
	{'k', JSTR("	上に進む", "	up"), TRUE},
	{'l', JSTR("	右に進む", "	right"), TRUE},
	{'y', JSTR("	左上に進む", "	up & left"), TRUE},
	{'u', JSTR("	右上に進む", "	up & right"), TRUE},
	{'b', JSTR("	左下に進む", "	down & left"), TRUE},
	{'n', JSTR("	右下に進む", "	down & right"), TRUE},
	{'\0', JSTR("<SHIFT><方角>	走る", "<SHIFT><dir>	run that way"), TRUE},
	{'\0', JSTR("<CTRL><方角>	何かを見つけるまで走る", "<CTRL><dir>	run till adjacent"), TRUE},
	{'f', JSTR("<方角>	何かを見つけるまで進む", "<dir>	find something"), TRUE},
	{'g', JSTR("<方角>	拾わずに進む", "<dir>	move without picking up"), TRUE},
	{'+', JSTR("<方角>	物を投げる", "<dir>	throw something"), TRUE},
	{'-', JSTR("<方角>	つえ/魔法の棒を振る", "<dir>	zap a wand in a direction"), TRUE},
	{'t', JSTR("<方角>	物を投げる", "<dir>	throw something"), FALSE},
	{'z', JSTR("<方角>	つえ/魔法の棒を振る", "<dir>	zap a wand in a direction"), FALSE},
	{'^', JSTR("<方角>	わなの種類を調べる", "<dir>	identify trap type"), TRUE},
	{'s', JSTR("	わなと隠しとびらを探す", "	search for trap/secret door"), TRUE},
	{'>', JSTR("	階段を降りる", "	go down a staircase"), TRUE},
	{'<', JSTR("	階段を上る", "	go up a staircase"), TRUE},
	{'.', JSTR("	その場で休む", "	rest for a turn"), TRUE},
	{'i', JSTR("	持ちものを見る", "	inventory"), TRUE},
	{'q', JSTR("	水薬を飲む", "	quaff potion"), TRUE},
	{'r', JSTR("	巻き物を読む", "	read scroll"), TRUE},
	{'e', JSTR("	食糧を食べる", "	eat food"), TRUE},
	{'w', JSTR("	武器を手に持つ", "	wield a weapon"), TRUE},
	{'W', JSTR("	よろいを着る", "	wear armor"), TRUE},
	{'T', JSTR("	よろいを脱ぐ", "	take armor off"), TRUE},
	{'P', JSTR("	指輪をはめる", "	put on ring"), TRUE},
	{'R', JSTR("	指輪をはずす", "	remove ring"), TRUE},
	{'d', JSTR("	持ちものを落とす", "	drop object"), TRUE},
	{'c', JSTR("	持ちものに呼び名をつける", "	call object"), TRUE},
	{'D', JSTR("	今まで何を発見したかを表示", "	recall what's been discovered"), TRUE},
	{'o', JSTR("	オプションの表示・設定", "	examine/set options"), TRUE},
	{'a', JSTR("	直前のコマンドを繰り返す", "	repeat last command"), TRUE},
	{'F', JSTR("	マクロを定義する", "	define macro"), TRUE},
	{CTRL('F'), JSTR("	マクロを実行する", "	run macro"), TRUE},
	{CTRL('R'), JSTR("	最後のメッセージを再表示する", "	repeat last message"), TRUE},
	{CTRL('T'), JSTR("	簡潔な表示を切り替える", "	toggle terse message format"), TRUE},
	{CTRL('L'), JSTR("	画面確認メッセージを表示", "	screen check message"), TRUE},
	{ESCAPE, JSTR("	コマンドを中止する", "	cancel command"), TRUE},
	{'S', JSTR("	ゲームをセーブして終了する", "	save game"), TRUE},
	{'Q', JSTR("	ゲームを終了する", "	quit"), TRUE},
	{'!', JSTR("	fake DOS", "	Supervisor Key (fake DOS)"), TRUE},
	{'v', JSTR("	バージョン情報を表示する", "	print version number"), TRUE},
	{0, NULL, FALSE}
    };

    if (dos_keys)
	show_help_list(dos_helpstr, FALSE, FALSE);
    else
	show_help_list(helpstr, FALSE, FALSE);
}

/*
 * identify:
 *	List the symbols used on the screen.
 */
void
identify(void)
{
    const struct h_list ident_list[] = {
	/* DOS 1.48 help-object order; solid rock is a modern extra. */
	{FLOOR,		JSTR("\t部屋の床", "\troom floor"),			TRUE},
	{PLAYER,	JSTR("\tプレイヤー", "\tyou"),			TRUE},
	{FOOD,		JSTR("\t食糧", "\tfood"),			TRUE},
	{AMULET,	JSTR("\tイェンダーの魔除け", "\tthe Amulet of Yendor"),	TRUE},
	{SCROLL,	JSTR("\t巻き物", "\tscroll"),			TRUE},
	{WEAPON,	JSTR("\t武器", "\tweapon"),			TRUE},
	{ARMOR,		JSTR("\tよろい", "\tarmor"),			TRUE},
	{GOLD,		JSTR("\t金塊", "\tgold"),				TRUE},
	{STICK,		JSTR("\tつえまたは魔法の棒", "\tstaff or wand"),		TRUE},
	{POTION,	JSTR("\t水薬", "\tpotion"),			TRUE},
	{RING,		JSTR("\t指輪", "\tring"),			TRUE},
	{PASSAGE,	JSTR("\t部屋と部屋をむすぶ通路", "\tpassage"),		TRUE},
	{' ',		JSTR("\t硬い岩", "\tsolid rock"),			TRUE},
	{DOOR,		JSTR("\t部屋のドア", "\tdoor"),				TRUE},
	{TRAP,		JSTR("\t隠されたわな", "\ttrap"),			TRUE},
	{'-',		JSTR("\t部屋の壁", "\twall of a room"),			TRUE},
	{'|',		JSTR("\t部屋の壁", "\twall of a room"),			TRUE},
	{STAIRS,	JSTR("\t階段", "\ta staircase"),			TRUE},
	{'\0',		JSTR("$,+\t安全な魔法と危険な魔法", "$,+\tsafe and perilous magic"), TRUE},
	{'\0',		JSTR("A-Z\t26種類の怪物", "A-Z\t26 different monsters"),	TRUE},
	{'\0'}
    };

    show_help_list(ident_list, TRUE, TRUE);
}

/*
 * d_level:
 *	He wants to go down a level
 */
void
d_level(void)
{
    if (chat(hero.y, hero.x) != STAIRS)
	msg(JSTR("降りる階段がない。", "I see no way down"));
    else
    {
	level++;
	seenstairs = FALSE;
	new_level();
    }
}

/*
 * u_level:
 *	He wants to go up a level
 */
void
u_level(void)
{
    if (chat(hero.y, hero.x) == STAIRS)
	if (amulet)
	{
	    level--;
	    if (level == 0)
		total_winner();
	    new_level();
	    msg(JSTR("はらわたをねじられるような感じがした。", "you feel a wrenching sensation in your gut"));
	}
	else
	    msg(JSTR("階段は不思議な力でふさがれている。", "your way is magically blocked"));
    else
	msg(JSTR("上る階段がない。", "I see no way up"));
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call(void)
{
    THING *obj;
    struct obj_info *op = NULL;
    char **guess;
    const char *elsewise = NULL;
    int *know;

    obj = get_item(JSTR("呼び名をつける持ちもの", "call"), CALLABLE);
    /*
     * Make certain that it is somethings that we want to wear
     */
    if (obj == NULL)
	return;
    switch (obj->o_type)
    {
	case RING:
	    op = &ring_info[obj->o_which];
	    elsewise = r_stones[obj->o_which];
	    goto norm;
	when POTION:
	    op = &pot_info[obj->o_which];
	    elsewise = p_colors[obj->o_which];
	    goto norm;
	when SCROLL:
	    op = &scr_info[obj->o_which];
	    elsewise = s_names[obj->o_which];
	    goto norm;
	when STICK:
	    op = &ws_info[obj->o_which];
	    elsewise = ws_made[obj->o_which];
norm:
	    know = &op->oi_know;
	    guess = &op->oi_guess;
	    if (*guess != NULL)
		elsewise = *guess;
	otherwise:
	    msg(JSTR("それには呼び名をつけることができない。", "you can't call that anything"));
	    return;
    }
    if (know != NULL && *know)
    {
	msg(JSTR("それはすでに鑑別されている。", "that has already been identified"));
	return;
    }
    if (elsewise != NULL)
	msg(JSTR("以前は「%s」と呼ばれていた", "Was called \"%s\""), elsewise);
    if (terse)
	msg(JSTR("呼び名は？", "call it: "));
    else
	msg(JSTR("どんな呼び名をつける？", "what do you want to call it?"));
    jrogue_show_cursor(TRUE);
    if (elsewise == NULL)
	prbuf[0] = '\0';
    else
	jrogue_strlcpy(prbuf, elsewise, PRBUF_SIZE);
    jrogue_set_normal_attr();
	if (get_str(prbuf, stdscr) == NORM)
    {
	if (*guess != NULL)
	    free(*guess);
	*guess = malloc(strlen(prbuf) + 1);
	if (*guess != NULL)
		jrogue_strlcpy(*guess, prbuf, MAXSTR);
    }
    msg("");
    jrogue_restore_cursor();
}

/*
 * current:
 *	Print the current weapon/armor
 */
void
current(const THING *cur, const char *how, const char *where)
{
    after = FALSE;
    if (cur != NULL)
    {

	inv_describe = FALSE;
	addmsg("%c) %s", pack_char(cur), inv_name(cur, TRUE));
	if (!terse)
	    addmsg(JSTR("（%s）", " (%s)"), how);
	inv_describe = TRUE;
	if (where)
	    addmsg(JSTR("%s", " %s"), where);
	endmsg();
    }
    else
    {
	addmsg(JSTR("身につけていない。", "not wearing anything"));
	if (where)
	    addmsg(JSTR("%s", " %s"), where);
	endmsg();
    }
}
