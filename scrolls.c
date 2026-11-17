/*
 * Read a scroll and let it happen
 *
 * @(#)scrolls.c	4.44 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * read_scroll:
 *	Read a scroll from the pack and do the appropriate thing
 */

void
read_scroll(void)
{
    THING *obj;
    PLACE *pp;
    int y, x;
    int ch;
    int i;
    int discardit = FALSE;
    int highlight;
    struct room *cur_room;
    THING *orig_obj;
    coord mp;

    obj = get_item(JSTR("読む巻き物", "read"), SCROLL);
    if (obj == NULL)
	return;
    if (obj->o_type != SCROLL)
    {
	/* The DOS 1.48 original uses the same text in both modes. */
	msg(JSTR("それには読めるところはない。", "there is nothing on it to read"));
	return;
    }
    if (!terse)
	msg(JSTR("巻き物を読むと、それは消えてしまった。", "as you read the scroll, it vanishes"));
    else
	msg(JSTR("巻き物は消えてしまった。", "the scroll vanishes"));
    /*
     * Calculate the effect it has on the poor guy.
     */
    if (obj == cur_weapon)
	cur_weapon = NULL;
    /*
     * Get rid of the thing
     */
    discardit = (obj->o_count == 1);
    leave_pack(obj, FALSE, FALSE);
    orig_obj = obj;

    switch (obj->o_which)
    {
	case S_CONFUSE:
	    /*
	     * Scroll of monster confusion.  Give him that power.
	     */
	    player.t_flags |= CANHUH;
	    msg(JSTR("手が%sく輝きはじめた。", "your hands begin to glow %s"), pick_color(JSTR("赤", "red")));
	when S_ARMOR:
	    if (cur_armor != NULL)
	    {
		cur_armor->o_arm--;
		cur_armor->o_flags &= ~ISCURSED;
		msg(terse
		    ? JSTR("着ているよろいがかすかに輝いた。", "your armor glows faintly")
		    : JSTR("着ているよろいが、少しのあいだかすかに輝いた。",
			   "your armor glows faintly for a moment"));
	    }
	when S_HOLD:
	    /*
	     * Hold monster scroll.  Stop all monsters within two spaces
	     * from chasing after the hero.
	     */

	    for (x = hero.x - 3; x <= hero.x + 3; x++)
		if (x >= 0 && x < NUMCOLS)
		    for (y = hero.y - 3; y <= hero.y + 3; y++)
			if (y >= 0 && y <= NUMLINES - 1)
			    if ((obj = moat(y, x)) != NULL)
			    {
				obj->t_flags &= ~ISRUN;
				obj->t_flags |= ISHELD;
			    }
	when S_SLEEP:
	    /*
	     * Scroll which makes you fall asleep
	     */
	    scr_info[S_SLEEP].oi_know = TRUE;
	    no_command += rnd(SLEEPTIME) + 4;
	    player.t_flags &= ~ISRUN;
	    msg(JSTR("眠りにおちてしまった。", "you fall asleep"));
	when S_CREATE:
	    /*
	     * Create a monster:
	     * First look in a circle around him, next try his room
	     * otherwise give up
	     */
	    i = plop_monster(hero.y, hero.x, &mp);
	    /* DOS 1.48 also reports failure if allocating the monster fails. */
	    if (i != 0 && (obj = new_item()) != NULL)
		new_monster(obj, randmonster(FALSE), &mp);
	    else
		msg(terse
		    ? JSTR("かすかな苦悶の叫びが聞こえる。", "you hear a faint cry of anguish")
		    : JSTR("遠くから、かすかな苦悶の叫びが聞こえる。",
			   "you hear a faint cry of anguish in the distance"));
	when S_IDENT:
	    /* Identify, let the rogue figure something out. */
	    scr_info[S_IDENT].oi_know = TRUE;
	    msg(JSTR("これは持ちものの種類がわかる巻き物だった。", "this scroll is an identify scroll"));
	    whatis(TRUE, 0);
	when S_MAP:
	    /*
	     * Scroll of magic mapping.
	     */
	    scr_info[S_MAP].oi_know = TRUE;
	    msg(JSTR("おや、巻き物に地図が現れたぞ。", "oh, now this scroll has a map on it"));
	    /*
	     * take all the things we want to keep hidden out of the window
	     */
	    for (y = 1; y < NUMLINES - 1; y++)
		for (x = 0; x < NUMCOLS; x++)
		{
		    pp = INDEX(y, x);
		    highlight = FALSE;
		    switch (ch = pp->p_ch)
		    {
			case DOOR:
			case STAIRS:
			    break;

			case '-':
			case '|':
			    if (!(pp->p_flags & F_REAL))
			    {
				highlight = TRUE;
				ch = pp->p_ch = DOOR;
				pp->p_flags |= F_REAL;
			    }
			    break;

			case ' ':
			    if (pp->p_flags & F_REAL)
				goto def;
			    highlight = TRUE;
			    pp->p_flags |= F_REAL;
			    ch = pp->p_ch = PASSAGE;
			    /* FALLTHROUGH */

			case PASSAGE:
pass:
			    if (!(pp->p_flags & F_REAL))
			    {
				highlight = TRUE;
				pp->p_ch = PASSAGE;
			    }
			    pp->p_flags |= (F_SEEN|F_REAL);
			    ch = PASSAGE;
			    break;

			case FLOOR:
			    /*
			     * DOS 1.48 magic mapping does not reveal hidden traps.
			     * Hidden passages still use the passage reveal path below.
			     */
			    if (pp->p_flags & F_PASS)
				goto pass;
			    ch = ' ';
			    break;

			default:
def:
			    if (pp->p_flags & F_PASS)
				goto pass;
			    ch = ' ';
			    break;
		    }
		    if (ch != ' ')
		    {
			if ((obj = pp->p_monst) != NULL)
			    obj->t_oldch = ch;
			if (obj == NULL || !on(player, SEEMONST))
			{
			    if (highlight)
			    {
				standout2();
				mvaddch2(y, x, ch);
				standend2();
			    }
			    else
				jrogue_mvaddch_map(y, x, ch);
			}
		    }
		}
	when S_FDET:
	    /*
	     * Potion of gold detection
	     */
	    ch = FALSE;
	    wclear(hw);
	    jrogue_wset_normal_attr(hw);
	    for (obj = lvl_obj; obj != NULL; obj = next(obj))
		if (obj->o_type == FOOD || obj->o_type == AMULET)
		{
		    ch = TRUE;
		    wmove(hw, obj->o_pos.y, obj->o_pos.x);
		    wstandout2(hw);
		    waddch2(hw, obj->o_type == FOOD ? FOOD : AMULET);
		    wstandend2(hw);
		    jrogue_wset_normal_attr(hw);
		}
	    if (ch)
	    {
		scr_info[S_FDET].oi_know = TRUE;
		show_win(JSTR("鼻がむずむずして食糧のにおいを感じる。--続く--",
			     "your nose tingles as you sense food.--More--"));
	    }
	    else
		msg(terse
		    ? JSTR("近くでうなる音が聞こえる。", "you hear a growling noise close by")
		    : JSTR("すぐ近くでうなる音が聞こえる。",
			   "you hear a growling noise very close to you"));
	when S_TELEP:
	    /*
	     * Scroll of teleportation:
	     * Make him dissapear and reappear
	     */
	    {
		cur_room = proom;
		teleport();
		if (cur_room != proom)
		    scr_info[S_TELEP].oi_know = TRUE;
	    }
	when S_ENCH:
	    if (cur_weapon == NULL || cur_weapon->o_type != WEAPON)
		msg(JSTR("奇妙な喪失感をかんじた。", "you feel a strange sense of loss"));
	    else
	    {
		cur_weapon->o_flags &= ~ISCURSED;
		if (rnd(2) == 0)
		    cur_weapon->o_hplus++;
		else
		    cur_weapon->o_dplus++;
		msg(terse
		    ? JSTR("手に持った%sが%sく輝いた。", "your %s glows %s")
		    : JSTR("手に持った%sが、少しのあいだ%sく輝いた。",
			   "your %s glows %s for a moment"),
		    weap_info[cur_weapon->o_which].oi_name, pick_color(JSTR("青", "blue")));
	    }
	when S_SCARE:
	    /*
	     * Reading it is a mistake and produces laughter at her
	     * poor boo boo.
	     */
	    msg(terse
		? JSTR("狂ったような笑い声が聞こえてくる。", "you hear maniacal laughter.")
		: JSTR("遠くから、狂ったような笑い声が聞こえてくる。",
		       "you hear maniacal laughter in the distance."));
	when S_REMOVE:
	    uncurse(cur_armor);
	    uncurse(cur_weapon);
	    uncurse(cur_ring[LEFT]);
	    uncurse(cur_ring[RIGHT]);
	    msg(terse
		? JSTR("誰かが見守ってくれている。", "somebody is watching over you")
		: JSTR("誰かに見守られているような気がした。",
		       "you feel as if somebody is watching over you"));
	when S_AGGR:
	    /*
	     * This scroll aggravates all the monsters on the current
	     * level and sets them running towards the hero
	     */
	    aggravate();
	    msg(terse
		? JSTR("うなり声が聞こえてくる。", "you hear a humming noise")
		: JSTR("かん高いうなり声が聞こえてくる。",
		       "you hear a high pitched humming noise"));
	when S_NOP:
	    msg(JSTR("この巻き物は白紙のようだ。", "this scroll seems to be blank"));
	when S_VORPAL:
	    /* Extra Vorpal Enchant Weapon. */
	    if (cur_weapon == NULL || cur_weapon->o_type != WEAPON)
		msg(terse
		    ? JSTR("狂ったような笑い声が聞こえてくる。", "you hear maniacal laughter.")
		    : JSTR("遠くから、狂ったような笑い声が聞こえてくる。",
			   "you hear maniacal laughter in the distance."));
	    else if (cur_weapon->o_enemy != 0)
	    {
		msg(JSTR("手に持った%sは煙とともに消えてしまった。", "your %s vanishes in a puff of smoke"),
		    weap_info[cur_weapon->o_which].oi_name);
		detach(pack, cur_weapon);
		discard(cur_weapon);
		cur_weapon = NULL;
	    }
	    else
	    {
		cur_weapon->o_enemy = pick_mons();
		cur_weapon->o_hplus++;
		cur_weapon->o_dplus++;
		cur_weapon->o_charges = 1;
		msg(JSTR("手に持った%sが%s閃光を放った。", "your %s gives off a flash%s"),
		    weap_info[cur_weapon->o_which].oi_name,
		    terse ? "" : JSTR("まばゆい白い", " of intense white light"));
	    }
#ifdef MASTER
	otherwise:
	    msg("なんてややこしい巻物だ！");
	    return;
#endif
    }
    obj = orig_obj;
    look(TRUE);	/* put the result of the scroll on the screen */
    status();

    call_it(&scr_info[obj->o_which]);

    if (discardit)
	discard(obj);
}

/*
 * uncurse:
 *	Uncurse an item
 */

void
uncurse(THING *obj)
{
    if (obj != NULL)
	obj->o_flags &= ~ISCURSED;
}
