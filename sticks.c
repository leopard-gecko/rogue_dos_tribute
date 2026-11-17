/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)sticks.c	4.39 (Berkeley) 02/05/99
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

/*
 * fix_stick:
 *	Set up a new stick
 */

void
fix_stick(THING *cur)
{
    if (strcmp(ws_type[cur->o_which], JSTR("つえ", "staff")) == 0)
	jrogue_strlcpy(cur->o_damage, "2x3", sizeof cur->o_damage);
    else
	jrogue_strlcpy(cur->o_damage, "1x1", sizeof cur->o_damage);
    jrogue_strlcpy(cur->o_hurldmg, "1x1", sizeof cur->o_hurldmg);

    switch (cur->o_which)
    {
	case WS_HIT:
	    cur->o_hplus = 100;
	    cur->o_dplus = 3;
	    jrogue_strlcpy(cur->o_damage, "1x8", sizeof cur->o_damage);
	    cur->o_charges = rnd(5) + 3;
	    break;
	case WS_LIGHT:
	    cur->o_charges = rnd(10) + 10;
	    break;
	otherwise:
	    cur->o_charges = rnd(5) + 3;
    }
}

/*
 * do_zap:
 *	Perform a zap with a wand
 */

void
do_zap(void)
{
    THING *obj, *tp;
    int y, x;
    const char *name;
    int monster, oldch;
    int which_one;
    THING bolt;

    if ((obj = get_item(JSTR("振りかざすつえ/魔法の棒", "zap with"), STICK)) == NULL)
	return;
    which_one = obj->o_which;
    if (obj->o_type != STICK)
    {
	if (obj->o_enemy != 0 && obj->o_charges > 0)
	    which_one = MAXSTICKS;
	else
	{
	    after = FALSE;
	    msg(JSTR("それを振りかざすことはできない。", "you can't zap with that!"));
	    return;
	}
    }
    if (obj->o_charges == 0)
    {
	msg(JSTR("何も起こらなかった。", "nothing happens"));
	return;
    }
    switch (which_one)
    {
	case WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room.
	     * In the DOS 1.48 version, a blind player only feels the glow
	     * and the wand is not identified.
	     */
	    if (on(player, ISBLIND))
	    {
		msg(JSTR("あたたかな光に包まれたような気がした。", "you feel a warm glow around you"));
		break;
	    }
	    ws_info[WS_LIGHT].oi_know = TRUE;
	    if (proom->r_flags & ISGONE)
		msg(JSTR("通路が輝き、しばらくして光は消えていった。", "the corridor glows and then fades"));
	    else
	    {
		proom->r_flags &= ~ISDARK;
		/*
		 * Light the room and put the player back up
		 */
		enter_room(&hero);
		if (!terse)
		    addmsg(JSTR("部屋は%sの光に照らされている。", "the room is lit by a shimmering %s light"), pick_color(JSTR("青色", "blue")));
		else
		    addmsg(JSTR("部屋が照らされた。", "the room is lit"));
		endmsg();
	    }
	when WS_DRAIN:
	    /*
	     * take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room (or next to hero
	     * if he is in a passage)
	     */
	    if (pstats.s_hpt < 2)
	    {
		msg(JSTR("それを使うための体力がない。", "you are too weak to use it"));
		return;
	    }
	    else
		drain();
	when WS_POLYMORPH:
	case WS_TELAWAY:
	case WS_TELTO:
	case WS_CANCEL:
	case MAXSTICKS:
	    y = hero.y;
	    x = hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		monster = tp->t_type;
		if (monster == 'F')
		{
		    player.t_flags &= ~ISHELD;
		    f_restor();
		}
		switch (which_one) {
		    case MAXSTICKS:
			if (monster == obj->o_enemy)
			{
			    msg(JSTR("%sは煙とともに消えてしまった。", "the %s vanishes in a puff of smoke"),
				monsters[monster - 'A'].m_name);
			    killed(tp, FALSE);
			    tp = NULL;
			}
			else
			    msg(JSTR("遠くから、狂ったような含み笑いが聞こえる。", "you hear a maniacal chuckle in the distance."));
			break;
		    case WS_POLYMORPH:
		    {
			THING *pp;
			int newch;

			pp = tp->t_pack;
			detach(mlist, tp);
			if (see_monst(tp))
			    jrogue_mvaddch_map(y, x, chat(y, x));
			oldch = tp->t_oldch;
			delta.y = y;
			delta.x = x;
			newch = rnd(26) + 'A';
			new_monster(tp, newch, &delta);
			if (see_monst(tp))
			    jrogue_mvaddch_map(y, x, newch);
			tp->t_oldch = oldch;
			tp->t_pack = pp;
			if (newch != monster)
			    ws_info[WS_POLYMORPH].oi_know = TRUE;
			break;
		    }
		    case WS_CANCEL:
			tp->t_flags |= ISCANC;
			tp->t_flags &= ~(ISINVIS|CANHUH);
			tp->t_disguise = tp->t_type;
			if (see_monst(tp))
			    jrogue_mvaddch_map(y, x, tp->t_disguise);
			break;
		    case WS_TELAWAY:
		    case WS_TELTO:
                    {
			coord new_pos;

			if (which_one == WS_TELAWAY)
			{
			    do
			    {
				find_floor(NULL, &new_pos, FALSE, TRUE);
			    } while (ce(new_pos, hero));
			}
			else
			{
			    new_pos.y = hero.y + delta.y;
			    new_pos.x = hero.x + delta.x;
			}
			tp->t_dest = &hero;
			tp->t_flags |= ISRUN;
			relocate(tp, &new_pos);
		    }
		}
	    }
	when WS_MISSILE:
	    ws_info[WS_MISSILE].oi_know = TRUE;
	    bolt.o_type = '*';
	    jrogue_strlcpy(bolt.o_hurldmg, "1x8", sizeof bolt.o_hurldmg);
	    bolt.o_hplus = 1000;
	    bolt.o_dplus = 1;
	    bolt.o_flags = ISMISL;
	    if (cur_weapon != NULL)
		bolt.o_launch = cur_weapon->o_which;
	    do_motion(&bolt, delta.y, delta.x);
	    if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != NULL
		&& !save_throw(VS_MAGIC, tp))
		    hit_monster(unc(bolt.o_pos), &bolt);
	    else if (terse)
		msg(JSTR("ミサイルは消えた。", "the missile vanishes"));
	    else
		msg(JSTR("ミサイルは煙とともに消えてしまった。", "the missile vanishes with a puff of smoke"));
	when WS_HIT:
	    delta.y += hero.y;
	    delta.x += hero.x;
	    if ((tp = moat(delta.y, delta.x)) != NULL)
	    {
		if (rnd(20) == 0)
		{
		    jrogue_strlcpy(obj->o_damage, "3x8", sizeof obj->o_damage);
		    obj->o_dplus = 9;
		}
		else
		{
		    jrogue_strlcpy(obj->o_damage, "2x8", sizeof obj->o_damage);
		    obj->o_dplus = 4;
		}
		fight(&delta, obj, FALSE);
	    }
	when WS_HASTE_M:
	case WS_SLOW_M:
	    y = hero.y;
	    x = hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		if (which_one == WS_HASTE_M)
		{
		    if (on(*tp, ISSLOW))
			tp->t_flags &= ~ISSLOW;
		    else
			tp->t_flags |= ISHASTE;
		}
		else
		{
		    if (on(*tp, ISHASTE))
			tp->t_flags &= ~ISHASTE;
		    else
			tp->t_flags |= ISSLOW;
		    tp->t_turn = TRUE;
		}
		delta.y = y;
		delta.x = x;
		runto(&delta);
	    }
	when WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
	    if (which_one == WS_ELECT)
		name = JSTR("いなずま", "bolt");
	    else if (which_one == WS_FIRE)
		name = JSTR("炎", "flame");
	    else
		name = JSTR("冷気", "ice");
	    fire_bolt(&hero, &delta, name);
	    ws_info[which_one].oi_know = TRUE;
	when WS_NOP:
	    break;
#ifdef MASTER
	otherwise:
	    msg(JSTR("なんておかしなつえ/魔法の棒なんだ！", "what a bizarre schtick!"));
#endif
    }
    obj->o_charges--;
}

/*
 * drain:
 *	Do drain hit points from player shtick
 */

void
drain(void)
{
    THING *mp;
    struct room *corp;
    THING **dp;
    int cnt;
    int inpass;
    THING *drainee[40];

    /*
     * First cnt how many things we need to spread the hit points among
     */
    cnt = 0;
    if (chat(hero.y, hero.x) == DOOR)
	corp = &passages[flat(hero.y, hero.x) & F_PNUM];
    else
	corp = NULL;
    inpass = (proom->r_flags & ISGONE);
    dp = drainee;
    for (mp = mlist; mp != NULL; mp = next(mp))
	if (mp->t_room == proom || mp->t_room == corp ||
	    (inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
	    &passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == proom))
		*dp++ = mp;
    if ((cnt = (int)(dp - drainee)) == 0)
    {
	msg(JSTR("チクチクする感覚におそわれた。", "you have a tingling feeling"));
	return;
    }
    *dp = NULL;
    pstats.s_hpt /= 2;
    cnt = pstats.s_hpt / cnt + 1;
    /*
     * Now zot all of the monsters
     */
    for (dp = drainee; *dp; dp++)
    {
	mp = *dp;
	if ((mp->t_stats.s_hpt -= cnt) <= 0)
	    killed(mp, see_monst(mp));
	else
	    runto(&mp->t_pos);
    }
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */

void
fire_bolt(const coord *start, coord *dir, const char *name)
{
    struct bolt_spot {
	coord s_pos;
	chtype s_under;
    } *sp, *ep, spotpos[BOLT_LENGTH * 2];
    THING *tp;
    int dirch = 0, ch;
    int hit_hero, used, changed;
    int beam_pair;
    int is_frost;
    coord pos;
    THING bolt;

    bolt.o_type = WEAPON;
    bolt.o_which = FLAME;
    jrogue_strlcpy(bolt.o_hurldmg, "6x6", sizeof bolt.o_hurldmg);
    bolt.o_hplus = 30;
    bolt.o_dplus = 0;
    weap_info[FLAME].oi_name = name;
    is_frost = (strcmp(name, JSTR("凍気", "frost")) == 0);
    if (strcmp(name, JSTR("炎", "flame")) == 0)
        beam_pair = JROGUE_PAIR_PROJECTILE_RED;
    else if (strcmp(name, JSTR("いなずま", "bolt")) == 0)
        beam_pair = JROGUE_PAIR_PROJECTILE_YELLOW;
    else
        beam_pair = JROGUE_PAIR_PROJECTILE_CYAN;
    switch (dir->y + dir->x)
    {
	case 0: dirch = '/';
	when 1: case -1: dirch = (dir->y == 0 ? '-' : '|');
	when 2: case -2: dirch = '\\';
    }
    pos = *start;
    hit_hero = (start != &hero);
    used = FALSE;
    changed = FALSE;
    for (sp = spotpos; sp <= &spotpos[BOLT_LENGTH * 2 - 1] && !used; sp++)
    {
	pos.y += dir->y;
	pos.x += dir->x;
	sp->s_pos = pos;
	sp->s_under = jrogue_mvinch_char(pos.y, pos.x);
	if (CCHAR(sp->s_under) == (chtype)dirch)
	    sp->s_under = 0;
	ch = winat(pos.y, pos.x);
	switch (ch)
	{
	    case DOOR:
		/*
		 * this code is necessary if the hero is on a door
		 * and he fires at the wall the door is in, it would
		 * otherwise loop infinitely
		 */
		if (ce(hero, pos))
		    goto def;
		/* FALLTHROUGH */
	    case '|':
	    case '-':
	    case ' ':
		if (!changed)
		    hit_hero = !hit_hero;
		changed = FALSE;
		dir->y = -dir->y;
		dir->x = -dir->x;
		sp--;
		msg(JSTR("%sは、はねかえった。", "the %s bounces"), name);
		break;
	    default:
def:
		if (!hit_hero && (tp = moat(pos.y, pos.x)) != NULL)
		{
		    hit_hero = TRUE;
		    changed = !changed;
		    tp->t_oldch = chat(pos.y, pos.x);
		    if (!save_throw(VS_MAGIC, tp) || is_frost)
		    {
			bolt.o_pos = pos;
			used = TRUE;
			if (tp->t_type == 'D' && strcmp(name, JSTR("炎", "flame")) == 0)
			{
			    if (jrogue_lang_is_japanese)
				msg(terse ? "炎ははねかえった。" :
				    "炎はドラゴンにはねかえった。");
			    else
				msg("the flame bounces%s", terse ? "" : " off the dragon");
			}
			else
			{
			    hit_monster(unc(pos), &bolt);
			    sp->s_under = jrogue_mvinch_char(pos.y, pos.x);
			    if (CCHAR(sp->s_under) == (chtype)dirch)
				sp->s_under = 0;
			}
		    }
		    else if (ch != 'X' || tp->t_disguise == 'X')
		    {
			if (start == &hero)
			    runto(&pos);
			if (terse)
			    msg(JSTR("%sは外れた。", "%s misses"), name);
			else
			    msg(JSTR("%sは、%sをかすめて通り過ぎた。", "the %s whizzes past the %s"), name, set_mname(tp));
		    }
		}
		else if (hit_hero && ce(pos, hero))
		{
		    hit_hero = FALSE;
		    changed = !changed;
		    if (!save(VS_MAGIC))
		    {
			if (is_frost)
			{
			    msg(terse
				? JSTR("凍気を浴びて体が凍りついた。", "You are frozen by a blast of frost.")
				: JSTR("氷の怪物の凍気を浴びて体が凍りついた。",
				       "You are frozen by a blast of frost from the Ice Monster."));
			    if (no_command < 20)
				no_command += spread(7);
			}
			else
			{
			    if ((pstats.s_hpt -= roll(6, 6)) <= 0)
			    {
				if (start == &hero)
				    death('b');
				else
				    death(moat(start->y, start->x)->t_type);
			    }
			    if (terse)
				msg(JSTR("%sが当たった。", "the %s hits"), name);
			    else
				msg(JSTR("%sによる攻撃が命中した。", "you are hit by the %s"), name);
			}
			used = TRUE;
		    }
		    else
			msg(JSTR("%sがかすめて通り過ぎた。", "the %s whizzes by you"), name);
		}
			mvaddch_projectile(pos.y, pos.x, dirch, beam_pair);
			msleep(50);
			jrogue_restore_cursor();
			refresh();
	}
    }
    for (ep = spotpos; ep < sp; ep++)
    {
	msleep(50);
	if (ep->s_under != 0)
	    jrogue_mvaddch_map(ep->s_pos.y, ep->s_pos.x, ep->s_under);
	jrogue_restore_cursor();
	refresh();
    }
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
const char *
charge_str(const THING *obj)
{
    static char buf[64];

    if (!(obj->o_flags & ISKNOW))
	buf[0] = '\0';
    else
	snprintf(buf, sizeof buf, JSTR("［残り %d］", " [%d charges]"), obj->o_charges);
    return buf;
}
