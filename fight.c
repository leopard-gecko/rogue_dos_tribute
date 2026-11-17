/*
 * All the fighting gets done here
 *
 * @(#)fight.c	4.67 (Berkeley) 09/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"

#define	EQSTR(a, b)	(strcmp(a, b) == 0)

static const char *h_names[] = {		/* strings for hitting */
	JSTR_STATIC("激しい一撃を食らわせた。", " scored an excellent hit on "),
	JSTR_STATIC("攻撃し命中した。", " hit "),
	JSTR_STATIC("傷を負わせた。", " have injured "),
	JSTR_STATIC("殴りかかり命中した。", " swing and hit "),
	JSTR_STATIC("激しい一撃を食らわせた。", " scored an excellent hit on "),
	JSTR_STATIC("攻撃し命中した。", " hit "),
	JSTR_STATIC("傷を負わせた。", " has injured "),
	JSTR_STATIC("殴りかかり命中した。", " swings and hits ")
};

static const char *m_names[] = {		/* strings for missing */
	JSTR_STATIC("攻撃したが外れた。", " miss"),
	JSTR_STATIC("殴りかかったが外れた。", " swing and miss"),
	JSTR_STATIC("攻撃したがぎりぎりでそれた。", " barely miss"),
	JSTR_STATIC("攻撃したが当たらなかった。", " don't hit"),
	JSTR_STATIC("攻撃したが外れた。", " misses"),
	JSTR_STATIC("殴りかかったが外れた。", " swings and misses"),
	JSTR_STATIC("攻撃したがぎりぎりでそれた。", " barely misses"),
	JSTR_STATIC("攻撃したが当たらなかった。", " doesn't hit"),
};
void
jrogue_apply_fight_language(void)
{
    h_names[0] = JSTR("激しい一撃を食らわせた。", " scored an excellent hit on ");
    h_names[1] = JSTR("攻撃し命中した。", " hit ");
    h_names[2] = JSTR("傷を負わせた。", " have injured ");
    h_names[3] = JSTR("殴りかかり命中した。", " swing and hit ");
    h_names[4] = JSTR("激しい一撃を食らわせた。", " scored an excellent hit on ");
    h_names[5] = JSTR("攻撃し命中した。", " hit ");
    h_names[6] = JSTR("傷を負わせた。", " has injured ");
    h_names[7] = JSTR("殴りかかり命中した。", " swings and hits ");
    m_names[0] = JSTR("攻撃したが外れた。", " miss");
    m_names[1] = JSTR("殴りかかったが外れた。", " swing and miss");
    m_names[2] = JSTR("攻撃したがぎりぎりでそれた。", " barely miss");
    m_names[3] = JSTR("攻撃したが当たらなかった。", " don't hit");
    m_names[4] = JSTR("攻撃したが外れた。", " misses");
    m_names[5] = JSTR("殴りかかったが外れた。", " swings and misses");
    m_names[6] = JSTR("攻撃したがぎりぎりでそれた。", " barely misses");
    m_names[7] = JSTR("攻撃したが当たらなかった。", " doesn't hit");
}


/*
 * adjustments to hit probabilities due to strength
 */
static int str_plus[] = {
    -7, -6, -5, -4, -3, -2, -1, 0,
     0,  0,  0,  0,  0,  0,  0, 0,
     0,  1,  1,  2,  2,  3,  3, 3,
     3,  3,  3,  3,  3,  3,  3, 4,
};

/*
 * adjustments to damage done due to strength
 */
static int add_dam[] = {
    -7, -6, -5, -4, -3, -2, -1, 0,
     0,  0,  0,  0,  0,  0,  0, 0,
     1,  2,  3,  3,  4,  4,  5, 5,
     5,  5,  5,  5,  5,  5,  5, 6
};

/*
 * fight:
 *	The player attacks the monster.
 */
int
fight(const coord *mp, THING *weap, int thrown)
{
    THING *tp;
    int did_hit = TRUE;
    const char *mname;

    /*
     * Find the monster we want to fight
     */
    if ((tp = moat(mp->y, mp->x)) == NULL)
	{
#ifdef MASTER
		debug("Fight what @ %d,%d", mp->y, mp->x);
#endif
		return FALSE;
	}
    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.
     */
    count = 0;
    quiet = 0;
    runto(mp);
    /*
     * Let him know it was really a xeroc (if it was one).
     */
    if (tp->t_type == 'X' && tp->t_disguise != 'X' && !on(player, ISBLIND))
    {
	tp->t_disguise = 'X';
	if (thrown)
	    return FALSE;
	msg(JSTR("待て！そいつはゼロックだ！", "wait! That's a Xeroc!"));
    }
    mname = set_mname(tp);
    did_hit = FALSE;
    has_hit = (terse && !to_death);
    if (roll_em(&player, tp, weap, thrown) || (weap != NULL && weap->o_type == POTION))
    {
	did_hit = FALSE;
	if (thrown)
	    thunk(weap, mname, terse);
	else
	    hit(NULL, mname, terse);
	if (weap != NULL && weap->o_type == POTION)
	{
	    th_effect(weap, tp);
	    if (!thrown)
	    {
		if (weap->o_count > 1)
		    weap->o_count--;
		else
		{
		    detach(pack, weap);
		    discard(weap);
		}
		cur_weapon = NULL;
	    }
	}
	if (on(player, CANHUH))
	{
	    did_hit = TRUE;
	    tp->t_flags |= ISHUH;
	    player.t_flags &= ~CANHUH;
	    endmsg();
	    has_hit = FALSE;
	    msg(JSTR("手の%sの輝きが消えた。", "your hands stop glowing %s"), pick_color(JSTR("赤色", "red")));
	}
	if (tp->t_stats.s_hpt <= 0)
	    killed(tp, TRUE);
	else if (did_hit && !on(player, ISBLIND))
	    msg(JSTR("%sは頭が混乱した。", "the %s appears confused"), mname);
	did_hit = TRUE;
    }
    else
    {
	if (thrown)
	    bounce(weap, mname, terse);
	else
	    miss(NULL, mname, terse);
	if (tp->t_type == 'S' && rnd(100) > 25)
	    slime_split(tp);
    }
    return did_hit;
}

/*
 * attack:
 *	The monster attacks the player
 */
int
attack(THING *mp)
{
    const char *mname;
    int oldhp;

    /*
     * Since this is an attack, stop running and any healing that was
     * going on at the time.
     */
    running = FALSE;
    count = 0;
    quiet = 0;
    if (to_death && !on(*mp, ISTARGET))
    {
	to_death = FALSE;
	kamikaze = FALSE;
    }
    if (mp->t_type == 'X' && mp->t_disguise != 'X' && !on(player, ISBLIND))
	mp->t_disguise = 'X';
    mname = set_mname(mp);
    oldhp = pstats.s_hpt;
    if (roll_em(mp, &player, NULL, FALSE))
    {
	if (has_hit)
	    addmsg(JSTR("", ".  "));
	hit(mname, NULL, FALSE);
	has_hit = FALSE;
	if (pstats.s_hpt <= 0)
	    death(mp->t_type);	/* Bye bye life ... */
	else if (!kamikaze)
	{
	    oldhp -= pstats.s_hpt;
	    if (oldhp > max_hit)
		max_hit = oldhp;
	    if (pstats.s_hpt <= max_hit)
		to_death = FALSE;
	}
	if (!on(*mp, ISCANC))
	    switch (mp->t_type)
	    {
		case 'A':
		    /*
		     * If an aquator hits, you can lose armor class.
		     */
		    rust_armor(cur_armor);
		when 'I':
		    /*
		     * When an ice monster hits, it makes frost wear off faster.
		     */
		    if (no_command > 1)
			no_command--;
		when 'R':
		    /*
		     * Rattlesnakes have poisonous bites
		     */
		    if (!save(VS_POISON))
		    {
			if (!ISWEARING(R_SUSTSTR))
			{
			    chg_str(-1);
			    if (!terse)
				msg(JSTR("足を咬まれて、力が弱くなったようだ。", "you feel a bite in your leg%s"),
			    terse ? "" : " and now feel weaker");
			    else
				msg(JSTR("咬まれて、力が弱くなった。", "a bite has weakened you"));
			}
			else if (!to_death)
			{
			    if (!terse)
				msg(JSTR("咬まれて、ほんの一瞬だけ力が弱くなった。", "a bite momentarily weakens you"));
			    else
				msg(JSTR("咬まれたが、何ともない。", "bite has no effect"));
			}
		    }
		when 'W':
		case 'V':
		    /*
		     * Wraiths might drain energy levels, and Vampires
		     * can steal max_hp
		     */
		    if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
		    {
			int fewer;

			if (mp->t_type == 'W')
			{
			    if (pstats.s_exp == 0)
				death('W');		/* All levels gone */
			    if (--pstats.s_lvl == 0)
			    {
				pstats.s_exp = 0;
				pstats.s_lvl = 1;
			    }
			    else
				pstats.s_exp = e_levels[pstats.s_lvl-1]+1;
			    fewer = roll(1, 10);
			}
			else
			    fewer = roll(1, 5);
			pstats.s_hpt -= fewer;
			max_hp -= fewer;
			if (pstats.s_hpt <= 0)
			    pstats.s_hpt = 1;
			if (max_hp <= 0)
			    death(mp->t_type);
			msg(JSTR("急に弱くなったようだ。", "you suddenly feel weaker"));
		    }
		when 'F':
		    /*
		     * Venus Flytrap stops the poor guy from moving
		     */
		    player.t_flags |= ISHELD;
		    snprintf(mp->t_stats.s_dmg, sizeof mp->t_stats.s_dmg, "%dx1", ++vf_hit);
		when 'L':
		{
		    /*
		     * Leperachaun steals some gold
		     */
		    int lastpurse;

		    lastpurse = purse;
		    purse -= GOLDCALC;
		    if (!save(VS_MAGIC))
			purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		    if (purse < 0)
			purse = 0;
		    remove_mon(&mp->t_pos, mp, FALSE);
                    mp=NULL;
		    if (purse != lastpurse)
			msg(JSTR("財布が軽くなった気がするぞ。", "your purse feels lighter"));
		}
		when 'N':
		{
		    THING *obj, *steal;
		    int nobj;

		    /*
		     * Nymph's steal a magic item, look through the pack
		     * and pick out one we like.
		     */
		    steal = NULL;
		    for (nobj = 0, obj = pack; obj != NULL; obj = next(obj))
			if (obj != cur_armor && obj != cur_weapon
			    && obj != cur_ring[LEFT] && obj != cur_ring[RIGHT]
			    && is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
		    if (steal != NULL)
		    {
			remove_mon(&mp->t_pos, moat(mp->t_pos.y, mp->t_pos.x), FALSE);
                        mp=NULL;
			steal = leave_pack(steal, TRUE, FALSE);
			if (steal == NULL)
			    break;
			msg(JSTR("彼女は%sを盗んでいった！", "she stole %s!"), inv_name(steal, TRUE));
			discard(steal);
		    }
		}
		otherwise:
		    break;
	    }
    }
    else if (mp->t_type != 'I')
    {
	if (has_hit)
	{
	    addmsg(JSTR("", ".  "));
	    has_hit = FALSE;
	}
	if (mp->t_type == 'F')
	{
	    pstats.s_hpt -= vf_hit;
	    if (pstats.s_hpt <= 0)
		death(mp->t_type);	/* Bye bye life ... */
	}
	miss(mname, NULL, FALSE);
    }
    count = 0;
    status();
    if (mp == NULL)
        return(-1);
    else
        return(0);
}

/*
 * set_mname:
 *	return the monster name for the given monster
 */
const char *
set_mname(const THING *tp)
{
    const char *mname;
    static char tbuf[MAXSTR] = { 't', 'h', 'e', ' ' };

    if (!see_monst(tp) && !on(player, SEEMONST))
	return (terse ? JSTR("何か", "it") : JSTR("何者か", "something"));
    else
	mname = monsters[tp->t_type - 'A'].m_name;
    jrogue_strlcpy(tbuf, mname, sizeof tbuf);
    return tbuf;
}

/*
 * swing:
 *	Returns true if the swing hits
 */
int
swing(int at_lvl, int op_arm, int wplus)
{
    int res = rnd(20);
    int need = (20 - at_lvl) - op_arm;

    return (res + wplus >= need);
}

/*
 * roll_em:
 *	Roll several attacks
 */
int
roll_em(const THING *thatt, THING *thdef, THING *weap, int hurl)
{
    const struct stats *att;
    struct stats *def;
    const char *cp;
    int ndice, nsides, def_arm;
    int did_hit = FALSE;
    int hplus;
    int dplus;
    int damage;

    att = &thatt->t_stats;
    def = &thdef->t_stats;
    if (weap == NULL)
    {
	cp = att->s_dmg;
	dplus = 0;
	hplus = 0;
    }
    else
    {
	hplus = (weap == NULL ? 0 : weap->o_hplus);
	dplus = (weap == NULL ? 0 : weap->o_dplus);
	if (weap->o_enemy != 0 && thdef->t_type == weap->o_enemy)
	{
	    hplus += 4;
	    dplus += 4;
	}
	if (weap == cur_weapon)
	{
	    if (ISRING(LEFT, R_ADDDAM))
		dplus += cur_ring[LEFT]->o_arm;
	    else if (ISRING(LEFT, R_ADDHIT))
		hplus += cur_ring[LEFT]->o_arm;
	    if (ISRING(RIGHT, R_ADDDAM))
		dplus += cur_ring[RIGHT]->o_arm;
	    else if (ISRING(RIGHT, R_ADDHIT))
		hplus += cur_ring[RIGHT]->o_arm;
	}
	cp = weap->o_damage;
	if (hurl)
	{
	    if ((weap->o_flags&ISMISL) && cur_weapon != NULL &&
	      cur_weapon->o_which == weap->o_launch)
	    {
		cp = weap->o_hurldmg;
		hplus += cur_weapon->o_hplus;
		dplus += cur_weapon->o_dplus;
	    }
	    else if (weap->o_launch < 0)
		cp = weap->o_hurldmg;
	}
	/*
	 * The DOS 1.48 code drains one charge whenever a staff of striking is used
	 * as the weapon in combat.  If it is already empty, it becomes
	 * a harmless 0x0 weapon.  A zap also loses the normal zap charge
	 * later in do_zap(), matching the original double-drain behavior.
	 */
	if (weap->o_type == STICK && weap->o_which == WS_HIT && --weap->o_charges < 0)
	{
	    jrogue_strlcpy(weap->o_damage, "0x0", sizeof weap->o_damage);
	    weap->o_hplus = 0;
	    weap->o_dplus = 0;
	    weap->o_charges = 0;
	    cp = weap->o_damage;
	    hplus = 0;
	    dplus = 0;
	}
    }
    /*
     * If the creature being attacked is not running (alseep or held)
     * then the attacker gets a plus four bonus to hit.
     */
    if (!on(*thdef, ISRUN))
	hplus += 4;
    def_arm = def->s_arm;
    if (def == &pstats)
    {
	if (cur_armor != NULL)
	    def_arm = cur_armor->o_arm;
	if (ISRING(LEFT, R_PROTECT))
	    def_arm -= cur_ring[LEFT]->o_arm;
	if (ISRING(RIGHT, R_PROTECT))
	    def_arm -= cur_ring[RIGHT]->o_arm;
    }
    while(cp != NULL && *cp != '\0')
    {
	ndice = atoi(cp);
	if ((cp = strchr(cp, 'x')) == NULL)
	    break;
	nsides = atoi(++cp);
	if (swing(att->s_lvl, def_arm, hplus + str_plus[att->s_str]))
	{
	    int proll;

	    proll = roll(ndice, nsides);
#ifdef MASTER
	    if (ndice + nsides > 0 && proll <= 0)
		debug("Damage for %dx%d came out %d, dplus = %d, add_dam = %d, def_arm = %d", ndice, nsides, proll, dplus, add_dam[att->s_str], def_arm);
#endif
	    damage = dplus + proll + add_dam[att->s_str];
	    if (thdef == &player && max_level == 1)
		damage = (damage + 1) / 2;
	    def->s_hpt -= max(0, damage);
	    did_hit = TRUE;
	}
	if ((cp = strchr(cp, '/')) == NULL)
	    break;
	cp++;
    }
    return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */

char *
prname(const char *mname, int upper)
{
    static char tbuf[MAXSTR];

    *tbuf = '\0';
    if (jrogue_lang_is_japanese)
    {
	if (mname == 0)
	    jrogue_strlcpy(tbuf, "あなた", sizeof tbuf); 
	else
	    jrogue_strlcpy(tbuf, mname, sizeof tbuf);
    }
    else
    {
	if (mname == 0)
	    jrogue_strlcpy(tbuf, "you", sizeof tbuf);
	else
	    jrogue_strlcpy(tbuf, mname, sizeof tbuf);
	if (upper)
	    *tbuf = (char) toupper(*tbuf);
    }
    return tbuf;
}

/*
 * thunk:
 *	A missile hits a monster
 */
void
thunk(const THING *weap, const char *mname, int noend)
{
    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	if (weap->o_type == WEAPON)
	    addmsg("%sは", weap_info[weap->o_which].oi_name);
	addmsg("%s", mname);
	addmsg("に命中した。");
    }
    else
    {
	if (weap->o_type == WEAPON)
	    addmsg("the %s hits ", weap_info[weap->o_which].oi_name);
	else
	    addmsg("you hit ");
	addmsg("%s", mname);
    }
    if (!noend)
	endmsg();
}

/*
 * hit:
 *	Print a message to indicate a successful hit
 */
void
hit(const char *er, const char *ee, int noend)
{
    int i;
    const char *s;

    if (to_death)
	return;

    /*
     * DOS 1.48 skips the random phrase selection in terse/expert mode.
     * JEC keeps the 80-column original's one draw for every displayed
     * melee hit, even when the terse phrase is selected.  A display
     * preference must not change subsequent combat/gameplay RNG results.
     * The normal (!terse) English and Japanese paths are unchanged.
     */
    i = rnd(4);
    if (er != NULL)
	i += 4;

    if (jrogue_lang_is_japanese)
    {
	addmsg("%s", prname(er, TRUE));
	if (terse)
	    s = "の攻撃は命中した。";
	else
	{
	    addmsg("は%sに", prname(ee, FALSE));
	    s = h_names[i];
	}
	addmsg("%s", s);
    }
    else
    {
	addmsg("%s", prname(er, TRUE));
	if (terse)
	    s = " hit";
	else
	{
	    s = h_names[i];
	}
	addmsg("%s", s);
	if (!terse)
	    addmsg("%s", prname(ee, FALSE));
    }
    if (!noend)
	endmsg();
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
void
miss(const char *er, const char *ee, int noend)
{
    int i;

    if (to_death)
	return;

    /* Consume the same draw as normal display, regardless of terse. */
    i = rnd(4);
    if (terse)
	i = 0;
    if (er != NULL)
	i += 4;

    if (jrogue_lang_is_japanese)
    {
	addmsg("%sは", prname(er, TRUE));
	if (!terse)
	    addmsg("%sに", prname(ee, FALSE));
	addmsg("%s", m_names[i]);
    }
    else
    {
	addmsg("%s", prname(er, TRUE));
	addmsg("%s", m_names[i]);
	if (!terse)
	    addmsg(" %s", prname(ee, FALSE));
    }
    if (!noend)
	endmsg();
}

/*
 * bounce:
 *	A missile misses a monster
 */
void
bounce(const THING *weap, const char *mname, int noend)
{
    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	if (weap->o_type == WEAPON)
	    addmsg("%sはそれて、", weap_info[weap->o_which].oi_name);
	addmsg("%s", mname);
	addmsg("には当たらなかった。");
    }
    else
    {
	if (weap->o_type == WEAPON)
	    addmsg("the %s misses ", weap_info[weap->o_which].oi_name);
	else
	    addmsg("you missed ");
	addmsg("%s", mname);
    }
    if (!noend)
	endmsg();
}


/*
 * remove_mon:
 *	Remove a monster from the screen
 */
void
remove_mon(const coord *mp, THING *tp, int waskill)
{
    THING *obj, *nexti;

    for (obj = tp->t_pack; obj != NULL; obj = nexti)
    {
	nexti = next(obj);
	obj->o_pos = tp->t_pos;
	detach(tp->t_pack, obj);
	if (waskill)
	    fall(obj, FALSE);
	else
	    discard(obj);
    }
    moat(mp->y, mp->x) = NULL;
    jrogue_mvaddch_map(mp->y, mp->x, tp->t_oldch);
    detach(mlist, tp);
    if (on(*tp, ISTARGET))
    {
	kamikaze = FALSE;
	to_death = FALSE;
    }
    discard(tp);
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(THING *tp, int pr)
{
    const char *mname;

    pstats.s_exp += tp->t_stats.s_exp;

    /*
     * If the monster was a venus flytrap, un-hold him
     */
    switch (tp->t_type)
    {
	case 'F':
	    player.t_flags &= ~ISHELD;
	    f_restor();
	when 'L':
	{
	    THING *gold;

	    gold = new_item();
	    if (gold == NULL)
		break;
	    gold->o_type = GOLD;
	    gold->o_goldval = GOLDCALC;
	    if (save(VS_MAGIC))
		gold->o_goldval += GOLDCALC + GOLDCALC
				 + GOLDCALC + GOLDCALC;
	    attach(tp->t_pack, gold);
	}
    }
    /*
     * Get rid of the monster.
     */
    mname = set_mname(tp);
    remove_mon(&tp->t_pos, tp, TRUE);
    if (pr)
    {
if (jrogue_lang_is_japanese)
	{
	    if (has_hit)
	    {
		addmsg("");
		has_hit = FALSE;
	    }
	    else
	    {
		if (!terse)
		    addmsg("あなたは");
	    }
	    msg("%sを倒した。", mname);
	}
	else
	{
	    if (has_hit)
	    {
		addmsg(".  Defeated ");
		has_hit = FALSE;
	    }
	    else
	    {
		if (!terse)
		    addmsg("you have ");
		addmsg("defeated ");
	    }
	    msg("%s", mname);
	}
    }
    /*
     * Do adjustments if he went up a level
     */
    check_level();
}
