/*
 * Function(s) for dealing with potions
 *
 * @(#)potions.c	4.46 (Berkeley) 06/07/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <stdio.h>
#include <ctype.h>
#include "rogue.h"

typedef struct PACT
{
    const int pa_flags;
    void (*pa_daemon)(int);
    const int pa_time;
    const char *pa_message;
} PACT;

static PACT p_actions[] =
{
	{ ISHUH,	unconfuse,	HUHDURATION,	/* P_CONFUSE */
		JSTR_STATIC("ちょっと待った、何が起こったんだ？え？なに？だれ？", "wait, what's going on? Huh? What? Who?") },
	{ 0,		NULL,	0,	NULL },		/* P_PARALYZE */
	{ 0,		NULL,	0,	NULL },		/* P_POISON */
	{ 0,		NULL,	0,	NULL },		/* P_STRENGTH */
	{ CANSEE,	unsee,	SEEDURATION,	prbuf },	/* P_SEEINVIS */
	{ 0,		NULL,	0,	NULL },		/* P_HEALING */
	{ 0,		NULL,	0,	NULL },		/* P_MFIND */
	{ 0,		NULL,	0,	NULL },		/* P_TFIND  */
	{ 0,		NULL,	0,	NULL },		/* P_RAISE */
	{ 0,		NULL,	0,	NULL },		/* P_XHEAL */
	{ 0,		NULL,	0,	NULL },		/* P_HASTE */
	{ 0,		NULL,	0,	NULL },		/* P_RESTORE */
	{ ISBLIND,	sight,	SEEDURATION,		/* P_BLIND */
		JSTR_STATIC("深い暗闇のとばりがあたりをおおってゆく。", "a cloak of darkness falls around you") },
	{ 0,		NULL,	0,	NULL },		/* P_NOP */
};
void
jrogue_apply_potions_language(void)
{
    p_actions[0].pa_message = JSTR("ちょっと待った、何が起こったんだ？え？なに？だれ？", "wait, what's going on? Huh? What? Who?");
    p_actions[4].pa_message = prbuf;
    p_actions[12].pa_message = JSTR("深い暗闇のとばりがあたりをおおってゆく。", "a cloak of darkness falls around you");
}


/*
 * quaff:
 *	Quaff a potion from the pack
 */

void
quaff(void)
{
    THING *obj, *tp, *mp;
    int discardit = FALSE;
    int show;

    obj = get_item(JSTR("飲む水薬", "quaff"), POTION);
    /*
     * Make certain that it is somethings that we want to drink
     */
    if (obj == NULL)
	return;
    if (obj->o_type != POTION)
    {
	if (!terse)
	    msg(JSTR("おえっ。なんでそんなものを飲もうとするの？", "yuk! Why would you want to drink that?"));
	else
	    msg(JSTR("それは飲めない。", "that's undrinkable"));
	return;
    }
    if (obj == cur_weapon)
	cur_weapon = NULL;

    /*
     * Calculate the effect it has on the poor guy.
     */
    discardit = (obj->o_count == 1);
    leave_pack(obj, FALSE, FALSE);
    switch (obj->o_which)
    {
	case P_CONFUSE:
	    do_pot(P_CONFUSE, TRUE);
	when P_POISON:
	    pot_info[P_POISON].oi_know = TRUE;
	    if (ISWEARING(R_SUSTSTR))
		msg(JSTR("具合が悪くなったが、すぐに治った。", "you feel momentarily sick."));
	    else
	    {
		chg_str(-(rnd(3) + 1));
		msg(JSTR("すごく具合が悪くなった。", "you feel very sick."));
	    }
	when P_HEALING:
	    pot_info[P_HEALING].oi_know = TRUE;
	    if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
		pstats.s_hpt = ++max_hp;
	    sight(0);
	    msg(JSTR("気分がよくなってきた。", "you begin to feel better"));
	when P_STRENGTH:
	    pot_info[P_STRENGTH].oi_know = TRUE;
	    chg_str(1);
	    msg(JSTR("強くなった気がするぞ。このはちきれそうな筋肉！", "you feel stronger. What bulging muscles!"));
	when P_MFIND:
	    player.t_flags |= SEEMONST;
	    fuse(turn_see_fuse, TRUE, HUHDURATION, AFTER);
	    if (turn_see(FALSE))
	    {
		pot_info[P_MFIND].oi_know = TRUE;
		msg("");
	    }
	    else
		msg(terse
		    ? JSTR("奇妙な気分がした。", "you have a strange feeling.")
		    : JSTR("しばらく奇妙な気分におそわれた。",
		           "you have a strange feeling for a moment."));
	when P_TFIND:
	    /*
	     * Potion of magic detection.  Show the potions and scrolls
	     */
	    show = FALSE;
	    if (lvl_obj != NULL)
	    {
		wclear(hw);
		jrogue_wset_normal_attr(hw);
		for (tp = lvl_obj; tp != NULL; tp = next(tp))
		{
		    if (is_magic(tp))
		    {
			show = TRUE;
			wmove(hw, tp->o_pos.y, tp->o_pos.x);
			waddch2(hw, goodch(tp));
			jrogue_wset_normal_attr(hw);
			pot_info[P_TFIND].oi_know = TRUE;
		    }
		}
		for (mp = mlist; mp != NULL; mp = next(mp))
		{
		    for (tp = mp->t_pack; tp != NULL; tp = next(tp))
		    {
			if (is_magic(tp))
			{
			    show = TRUE;
			    wmove(hw, mp->t_pos.y, mp->t_pos.x);
			    waddch2(hw, MAGIC);
			    jrogue_wset_normal_attr(hw);
			}
		    }
		}
	    }
	    if (show)
	    {
		pot_info[P_TFIND].oi_know = TRUE;
		show_win(JSTR("この階で魔法の存在を感じた。--続く--",
			     "You sense the presence of magic on this level.--More--"));
	    }
	    else
		msg(terse
		    ? JSTR("しばらく奇妙な気分がした。",
		           "you have a strange feeling for a moment.")
		    : JSTR("しばらく奇妙な気分におそわれたが、すぐに消えていった。",
		           "you have a strange feeling for a moment, then it passes."));
	when P_PARALYZE:
	    pot_info[P_PARALYZE].oi_know = TRUE;
	    no_command = HOLDTIME;
	    player.t_flags &= ~ISRUN;
	    msg(JSTR("体が動かない。", "you can't move"));
	when P_SEEINVIS:
	    snprintf(prbuf, PRBUF_SIZE, JSTR("この水薬は、%sジュースのような味がする。", "this potion tastes like %s juice"), fruit);
	    show = on(player, CANSEE);
	    do_pot(P_SEEINVIS, FALSE);
	    if (!show)
		invis_on();
	    sight(0);
	when P_RAISE:
	    pot_info[P_RAISE].oi_know = TRUE;
	    msg(JSTR("突然に腕前がすごく上がったようだ。", "you suddenly feel much more skillful"));
	    raise_level();
	when P_XHEAL:
	    pot_info[P_XHEAL].oi_know = TRUE;
	    if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_hp)
	    {
		if (pstats.s_hpt > max_hp + pstats.s_lvl + 1)
		    ++max_hp;
		pstats.s_hpt = ++max_hp;
	    }
	    sight(0);
	    msg(JSTR("とても気分がよくなってきた。", "you begin to feel much better"));
	when P_HASTE:
	    pot_info[P_HASTE].oi_know = TRUE;
	    after = FALSE;
	    if (add_haste(TRUE))
		msg(JSTR("動きが素早くなったようだ。", "you feel yourself moving much faster"));
	when P_RESTORE:
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[RIGHT]->o_arm);
	    if (pstats.s_str < max_stats.s_str)
		pstats.s_str = max_stats.s_str;
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[RIGHT]->o_arm);
	    msg(terse
		? JSTR("身体中があたたまる。", "you feel warm all over")
		: JSTR("ああ、これはうまい。身体中があたたまる。",
		       "hey, this tastes great.  It makes you feel warm all over"));
	when P_BLIND:
	    do_pot(P_BLIND, TRUE);
	when P_NOP:
	    msg(JSTR("この水薬は、ひどくつまらない味がする。", "this potion tastes extremely dull"));
#ifdef MASTER
	otherwise:
	    msg(JSTR("なんてひどい味の水薬だ！", "what an odd tasting potion!"));
	    return;
#endif
    }
    status();
    /*
     * Throw the item away
     */

    call_it(&pot_info[obj->o_which]);

    if (discardit)
	discard(obj);
    return;
}


/*
 * th_effect:
 *	Compute the effect of a thrown potion hitting a monster.
 */
void
th_effect(const THING *obj, THING *tp)
{
    const char *mname;

    if (obj == NULL || tp == NULL || obj->o_type != POTION)
	return;

    mname = set_mname(tp);
    switch (obj->o_which)
    {
	case P_CONFUSE:
	case P_BLIND:
	    tp->t_flags |= ISHUH;
	    msg(JSTR("%sは頭が混乱した。", "the %s appears confused"), mname);
	    break;
	case P_PARALYZE:
	    tp->t_flags &= ~ISRUN;
	    tp->t_flags |= ISHELD;
	    break;
	case P_HEALING:
	case P_XHEAL:
	    if ((tp->t_stats.s_hpt += rnd(8)) > tp->t_stats.s_maxhp)
		tp->t_stats.s_hpt = ++tp->t_stats.s_maxhp;
	    break;
	case P_RAISE:
	    tp->t_stats.s_hpt += 8;
	    tp->t_stats.s_maxhp += 8;
	    tp->t_stats.s_lvl++;
	    break;
	case P_HASTE:
	    tp->t_flags |= ISHASTE;
	    break;
    }
    msg(JSTR("水薬のびんはくだけ散った。", "the flask shatters."));
}

/*
 * is_magic:
 *	Returns true if an object radiates magic
 */
int
is_magic(const THING *obj)
{
    switch (obj->o_type)
    {
	case ARMOR:
	    return (obj->o_arm != a_class[obj->o_which]);
	case WEAPON:
	    return (obj->o_hplus != 0 || obj->o_dplus != 0);
	case POTION:
	case SCROLL:
	case STICK:
	case RING:
	case AMULET:
	    return TRUE;
    }
    return FALSE;
}

/*
 * invis_on:
 *	Turn on the ability to see invisible
 */

void
invis_on(void)
{
    THING *mp;

    player.t_flags |= CANSEE;
    for (mp = mlist; mp != NULL; mp = next(mp))
	if (on(*mp, ISINVIS) && see_monst(mp))
	    jrogue_mvaddch_map(mp->t_pos.y, mp->t_pos.x, mp->t_disguise);
}

/*
 * turn_see:
 *	Put on or off seeing monsters on this level
 */
void
turn_see_fuse(int turn_off)
{
    (void) turn_see(turn_off);
}

int
turn_see(int turn_off)
{
    THING *mp;
    int can_see, add_new;

    add_new = FALSE;
    for (mp = mlist; mp != NULL; mp = next(mp))
    {
	move(mp->t_pos.y, mp->t_pos.x);
	can_see = see_monst(mp);
	if (turn_off)
	{
	    if (!can_see)
		jrogue_addch_map(mp->t_oldch);
	}
	else
	{
	    if (!can_see)
		standout2();
	    if (can_see)
		jrogue_addch_map(mp->t_type);
	    else
		addch2(mp->t_type);
	    if (!can_see)
	    {
		standend2();
		add_new++;
	    }
	}
    }
    if (turn_off)
	player.t_flags &= ~SEEMONST;
    else
	player.t_flags |= SEEMONST;
    return add_new;
}

/*
 * seen_stairs:
 *	Return TRUE if the player has seen the stairs
 */
int
seen_stairs(void)
{
    THING	*tp;

    move(stairs.y, stairs.x);
    if (jrogue_mvinch_char(stairs.y, stairs.x) == STAIRS)	/* it's on the map */
	return TRUE;
    if (ce(hero, stairs))			/* It's under him */
	return TRUE;

    /*
     * if a monster is on the stairs, this gets hairy
     */
    if ((tp = moat(stairs.y, stairs.x)) != NULL)
    {
	if (see_monst(tp) && on(*tp, ISRUN))	/* if it's visible and awake */
	    return TRUE;			/* it must have moved there */

	if (on(player, SEEMONST)		/* if she can detect monster */
	    && tp->t_oldch == STAIRS)		/* and there once were stairs */
		return TRUE;			/* it must have moved there */
    }
    return FALSE;
}

/*
 * raise_level:
 *	The guy just magically went up a level.
 */

void
raise_level(void)
{
    pstats.s_exp = e_levels[pstats.s_lvl-1] + 1L;
    check_level();
}

/*
 * do_pot:
 *	Do a potion with standard setup.  This means it uses a fuse and
 *	turns on a flag
 */

void
do_pot(int type, int knowit)
{
    const PACT *pp;
    int t;

    pp = &p_actions[type];
    if (!pot_info[type].oi_know)
	pot_info[type].oi_know = knowit;
    t = spread(pp->pa_time);
    if (!on(player, pp->pa_flags))
    {
	player.t_flags |= pp->pa_flags;
	fuse(pp->pa_daemon, 0, t, AFTER);
	look(FALSE);
    }
    /*
     * DOS 1.48 Rogue does not extend the duration when the same
     * confusion, blindness, or see-invisible potion is quaffed while
     * the effect is already active.
     */
    if (pp->pa_message != NULL)
	msg("%s", pp->pa_message);
}
