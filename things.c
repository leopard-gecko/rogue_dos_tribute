/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)things.c	4.53 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <curses.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
static char *inv_name_en(const THING *obj, int drop);
static char *inv_name_ja(const THING *obj, int drop);
static void nameit_en(const THING *obj, const char *type, const char *which, const struct obj_info *op, const char *(*prfunc)(const THING *));
static void nameit_ja(const THING *obj, const char *type, const char *which, const struct obj_info *op, const char *(*prfunc)(const THING *));
static const char *ring_num_prefix_en(const THING *obj);

#define PRBUF_ROOM(p) \
    ((size_t)(((p) >= prbuf && (p) < prbuf + PRBUF_SIZE) ? \
    (prbuf + PRBUF_SIZE - (p)) : 0))

static char *
inv_name_en(const THING *obj, int drop)
{
    char *pb;
    struct obj_info *op;
    const char *sp;
    int which;

    pb = prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
	case POTION:
	    nameit_en(obj, "potion", p_colors[which], &pot_info[which], nullstr);
	when RING:
	    op = &ring_info[which];
	    if (op->oi_know)
	    {
		if (terse)
		    snprintf(pb, PRBUF_ROOM(pb), "A%s ring of %s",
			ring_num_prefix_en(obj), op->oi_name);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "A%s ring of %s(%s)",
			ring_num_prefix_en(obj), op->oi_name, r_stones[which]);
	    }
	    else if (op->oi_guess)
	    {
		if (terse)
		    snprintf(pb, PRBUF_ROOM(pb), "A ring called %s",
			op->oi_guess);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "A ring called %s(%s)",
			op->oi_guess, r_stones[which]);
	    }
	    else
		snprintf(pb, PRBUF_ROOM(pb), "A%s %s ring",
		    vowelstr(r_stones[which]), r_stones[which]);
	when STICK:
	    op = &ws_info[which];
	    snprintf(pb, PRBUF_ROOM(pb), "A%s %s ",
		vowelstr(ws_type[which]), ws_type[which]);
	    pb = &prbuf[strlen(prbuf)];
	    if (op->oi_know)
	    {
		if (terse)
		    snprintf(pb, PRBUF_ROOM(pb), "of %s%s",
			op->oi_name, charge_str(obj));
		else
		    snprintf(pb, PRBUF_ROOM(pb), "of %s%s(%s)",
			op->oi_name, charge_str(obj), ws_made[which]);
	    }
	    else if (op->oi_guess)
	    {
		if (terse)
		    snprintf(pb, PRBUF_ROOM(pb), "called %s", op->oi_guess);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "called %s(%s)",
			op->oi_guess, ws_made[which]);
	    }
	    else
		snprintf(&prbuf[2], PRBUF_SIZE - 2, "%s %s",
		    ws_made[which], ws_type[which]);
	when SCROLL:
	    if (obj->o_count == 1)
	    {
		jrogue_strlcpy(pb, "A scroll ", PRBUF_ROOM(pb));
		pb = &prbuf[9];
	    }
	    else
	    {
		snprintf(pb, PRBUF_ROOM(pb), "%d scrolls ", obj->o_count);
		pb = &prbuf[strlen(prbuf)];
	    }
	    op = &scr_info[which];
	    if (op->oi_know)
		snprintf(pb, PRBUF_ROOM(pb), "of %s", op->oi_name);
	    else if (op->oi_guess)
		snprintf(pb, PRBUF_ROOM(pb), "called %s", op->oi_guess);
	    else if (terse)
		snprintf(pb, PRBUF_ROOM(pb), "titled '%.17s'", s_names[which]);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "titled '%s'", s_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    snprintf(pb, PRBUF_ROOM(pb), "A%s %s", vowelstr(fruit), fruit);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d %ss", obj->o_count, fruit);
	    else
		if (obj->o_count == 1)
		    jrogue_strlcpy(pb, "Some food", PRBUF_ROOM(pb));
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d rations of food", obj->o_count);
	when WEAPON:
	    sp = weap_info[which].oi_name;
	    if (obj->o_count > 1)
		snprintf(pb, PRBUF_ROOM(pb), "%d ", obj->o_count);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "A%s ", vowelstr(sp));
	    pb = &prbuf[strlen(prbuf)];
	    if (obj->o_flags & ISKNOW)
		snprintf(pb, PRBUF_ROOM(pb), "%s %s", num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
	    if (obj->o_count > 1)
		jrogue_strlcat(pb, "s", PRBUF_ROOM(pb));
	    if (obj->o_enemy != 0 && (obj->o_flags & ISREVEAL))
	    {
		pb = &prbuf[strlen(prbuf)];
		snprintf(pb, PRBUF_ROOM(pb), " of %s slaying", monsters[obj->o_enemy - 'A'].m_name);
	    }
	when ARMOR:
	    sp = arm_info[which].oi_name;
	    if (obj->o_flags & ISKNOW)
	    {
		if (terse)
		    snprintf(pb, PRBUF_ROOM(pb), "%s %s",
			num(a_class[which] - obj->o_arm, 0, ARMOR), sp);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%s %s [armor class %d]",
			num(a_class[which] - obj->o_arm, 0, ARMOR), sp,
			11 - obj->o_arm);
	    }
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
	when AMULET:
	    jrogue_strlcpy(pb, "The Amulet of Yendor", PRBUF_ROOM(pb));
	when GOLD:
	    snprintf(prbuf, PRBUF_SIZE, "%d Gold pieces", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    snprintf(pb, PRBUF_ROOM(pb), "Something bizarre %s", unctrl(obj->o_type));
#endif
    }
    if (inv_describe)
    {
	if (obj == cur_armor)
	    jrogue_strlcat(pb, " (being worn)", PRBUF_ROOM(pb));
	if (obj == cur_weapon)
	    jrogue_strlcat(pb, " (weapon in hand)", PRBUF_ROOM(pb));
	if (obj == cur_ring[LEFT])
	    jrogue_strlcat(pb, " (on left hand)", PRBUF_ROOM(pb));
	else if (obj == cur_ring[RIGHT])
	    jrogue_strlcat(pb, " (on right hand)", PRBUF_ROOM(pb));
    }
    if (drop && isupper((int)prbuf[0]))
	prbuf[0] = (char) tolower(prbuf[0]);
    else if (!drop && islower((int)*prbuf))
	*prbuf = (char) toupper(*prbuf);
    prbuf[PRBUF_SIZE-1] = '\0';
    return prbuf;
}


static char *
inv_name_ja(const THING *obj, int drop)
{
    char *pb;
    struct obj_info *op;
    const char *sp;
    int which;

    pb = prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
        case POTION:
	    nameit_ja(obj, "水薬", p_colors[which], &pot_info[which], nullstr);
	when RING:
	    nameit_ja(obj, "指輪", r_stones[which], &ring_info[which], ring_num);
	when STICK:
	    nameit_ja(obj, ws_type[which], ws_made[which], &ws_info[which], charge_str);
	when SCROLL:
	    if (obj->o_count > 1)
	    {
		snprintf(pb, PRBUF_ROOM(pb), "%dつの", obj->o_count);
		pb = &prbuf[strlen(prbuf)];
	    }
	    op = &scr_info[which];
	    if (op->oi_know)
		snprintf(pb, PRBUF_ROOM(pb), "%s巻き物", op->oi_name);
	    else if (op->oi_guess)
		snprintf(pb, PRBUF_ROOM(pb), "『%s』と呼ぶ巻き物", op->oi_guess);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "「%s」という巻き物", s_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    snprintf(pb, PRBUF_ROOM(pb), "%s", fruit);
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d個の%s", obj->o_count, fruit);
	    else
		if (obj->o_count == 1)
		    jrogue_strlcpy(pb, "1袋の食糧", PRBUF_ROOM(pb));
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d袋の食糧", obj->o_count);
	when WEAPON:
	    sp = weap_info[which].oi_name;
	    if (obj->o_count > 1)
	    		snprintf(pb, PRBUF_ROOM(pb), "%d個の", obj->o_count);
	    else
		pb[0] = '\0';
	    pb = &prbuf[strlen(prbuf)];
	    if (obj->o_flags & ISKNOW)
		snprintf(pb, PRBUF_ROOM(pb), "（%s）%s", num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
	    if (obj->o_count > 1)
		jrogue_strlcat(pb, "", PRBUF_ROOM(pb));
	    if (obj->o_enemy != 0 && (obj->o_flags & ISREVEAL))
	    {
		pb = &prbuf[strlen(prbuf)];
		snprintf(pb, PRBUF_ROOM(pb), "（%s殺し）", monsters[obj->o_enemy - 'A'].m_name);
	    }
	when ARMOR:
	    sp = arm_info[which].oi_name;
	    pb[0] = '\0';
	    if (obj->o_flags & ISKNOW)
	    {
	    pb = &prbuf[strlen(prbuf)];
		snprintf(pb, PRBUF_ROOM(pb), "（%s）%s［",
		    num(a_class[which] - obj->o_arm, 0, ARMOR), sp);
		if (!terse)
		    jrogue_strlcat(pb, "守備 ", PRBUF_ROOM(pb));
		pb = &prbuf[strlen(prbuf)];
		snprintf(pb, PRBUF_ROOM(pb), "%d］", 11 - obj->o_arm);
	    }
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);

	when AMULET:
	    jrogue_strlcpy(pb, "イェンダーの魔除け", PRBUF_ROOM(pb));
	when GOLD:
	    snprintf(prbuf, PRBUF_SIZE, "%d個の金塊", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    snprintf(pb, PRBUF_ROOM(pb), "何か変なもの「%s」", unctrl(obj->o_type));
#endif
    }
    if (inv_describe)
    {
	if (obj == cur_armor)
	    jrogue_strlcat(pb, "（着ている）", PRBUF_ROOM(pb));
	if (obj == cur_weapon)
	    jrogue_strlcat(pb, "（使っている）", PRBUF_ROOM(pb));
	if (obj == cur_ring[LEFT])
	    jrogue_strlcat(pb, "（左手）", PRBUF_ROOM(pb));
	else if (obj == cur_ring[RIGHT])
	    jrogue_strlcat(pb, "（右手）", PRBUF_ROOM(pb));
    }
    if (drop && isupper((int)prbuf[0]))
	prbuf[0] = (char) tolower(prbuf[0]);
    else if (!drop && islower((int)*prbuf))
	*prbuf = (char) toupper(*prbuf);
    prbuf[PRBUF_SIZE-1] = '\0';
    return prbuf;
}


char *
inv_name(const THING *obj, int drop)
{
    return jrogue_lang_is_japanese ? inv_name_ja(obj, drop) : inv_name_en(obj, drop);
}

/*
 * drop:
 *	Put something down
 */

void
drop(void)
{
    int ch;
    THING *obj;

    ch = chat(hero.y, hero.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	after = FALSE;
	msg(JSTR("そこには、すでに何かが置かれている。", "there is something there already"));
	return;
    }
    if ((obj = get_item(JSTR("落とす持ちもの", "drop"), 0)) == NULL)
	return;
    if (!dropcheck(obj))
	return;
    obj = leave_pack(obj, TRUE, !ISMULT(obj->o_type));
    if (obj == NULL)
	return;
    /*
     * Link it into the level object list
     */
    attach(lvl_obj, obj);
    chat(hero.y, hero.x) = obj->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    obj->o_pos = hero;
    if (obj->o_type == AMULET)
	amulet = FALSE;
    msg(JSTR("%sを落とした。", "dropped %s"), inv_name(obj, TRUE));
}

/*
 * dropcheck:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
int
dropcheck(const THING *obj)
{
    if (obj == NULL)
	return TRUE;
    if (obj != cur_armor && obj != cur_weapon
	&& obj != cur_ring[LEFT] && obj != cur_ring[RIGHT])
	    return TRUE;
    if (obj->o_flags & ISCURSED)
    {
	msg(JSTR("それはできない。その持ちものは呪われているようだ。", "you can't.  It appears to be cursed"));
	return FALSE;
    }
    if (obj == cur_weapon)
	cur_weapon = NULL;
    else if (obj == cur_armor)
    {
	waste_time();
	cur_armor = NULL;
    }
    else
    {
	cur_ring[obj == cur_ring[LEFT] ? LEFT : RIGHT] = NULL;
	switch (obj->o_which)
	{
	    case R_ADDSTR:
		chg_str(-obj->o_arm);
		break;
	    case R_SEEINVIS:
		unsee(0);
		extinguish(unsee);
		break;
	}
    }
    return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING *
new_thing(void)
{
    THING *cur;
    int r;

    cur = new_item();
    if (cur == NULL)
	return NULL;
    cur->o_hplus = 0;
    cur->o_dplus = 0;
    jrogue_strlcpy(cur->o_damage, "0x0", sizeof cur->o_damage);
    jrogue_strlcpy(cur->o_hurldmg, "0x0", sizeof cur->o_hurldmg);
    cur->o_arm = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_enemy = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
    {
	case 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(pot_info, MAXPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(scr_info, MAXSCROLLS);
	when 2:
	    cur->o_type = FOOD;
	    no_food = 0;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    init_weapon(cur, rnd(MAXWEAPONS));
	    if ((r = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (r < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    cur->o_which = pick_one(arm_info, MAXARMORS);
	    cur->o_arm = a_class[cur->o_which];
	    if ((r = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_arm += rnd(3) + 1;
	    }
	    else if (r < 28)
		cur->o_arm -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(ring_info, MAXRINGS);
	    switch (cur->o_which)
	    {
		case R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_arm = rnd(3)) == 0)
		    {
			cur->o_arm = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(ws_info, MAXSTICKS);
	    fix_stick(cur);
#ifdef MASTER
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(stdscr, ' ');
#endif
    }
    return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible objects
 */
int
pick_one(const struct obj_info *info, int nitems)
{
    const struct obj_info *end;
    const struct obj_info *start;
    int i;

    start = info;
    for (end = &info[nitems], i = rnd(100); info < end; info++)
	if (i < info->oi_prob)
	    break;
    if (info == end)
    {
#ifdef MASTER
	if (wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (info = start; info < end; info++)
		msg("%s: %d%%", info->oi_name, info->oi_prob);
	}
#endif
	info = start;
    }
    return (int)(info - start);
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int line_cnt = 0;


void
discovered(void)
{
    print_disc(POTION);
    add_line("", NULL);
    print_disc(SCROLL);
    add_line("", NULL);
    print_disc(RING);
    add_line("", NULL);
    print_disc(STICK);
    end_line();
    msg("");
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX4(a,b,c,d)	(a > b ? (a > c ? (a > d ? a : d) : (c > d ? c : d)) : (b > c ? (b > d ? b : d) : (c > d ? c : d)))


void
print_disc(int type)
{
    struct obj_info *info = NULL;
    int i, maxnum = 0, num_found;
    THING obj;
    int order[MAX4(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

    switch (type)
    {
	case SCROLL:
	    maxnum = MAXSCROLLS;
	    info = scr_info;
	    break;
	case POTION:
	    maxnum = MAXPOTIONS;
	    info = pot_info;
	    break;
	case RING:
	    maxnum = MAXRINGS;
	    info = ring_info;
	    break;
	case STICK:
	    maxnum = MAXSTICKS;
	    info = ws_info;
	    break;
    }
    set_order(order, maxnum);
    obj.o_count = 1;
    obj.o_flags = 0;
    obj.o_enemy = 0;
    num_found = 0;
    for (i = 0; i < maxnum; i++)
	if (info[order[i]].oi_know || info[order[i]].oi_guess)
	{
	    obj.o_type = type;
	    obj.o_which = order[i];
	    add_line("%s", inv_name(&obj, FALSE));
	    num_found++;
	}
    if (num_found == 0)
	add_line(nothing(type), NULL);
}

/*
 * set_order:
 *	Set up order for list
 */

void
set_order(int *order, int numthings)
{
    int i, r, t;

    for (i = 0; i< numthings; i++)
	order[i] = i;

    for (i = numthings; i > 0; i--)
    {
	r = rnd(i);
	t = order[i - 1];
	order[i - 1] = order[r];
	order[r] = t;
    }
}

/*
 * add_line:
 *	Add a line to the full-screen list of discoveries or inventory.
 */
/* VARARGS1 */
int
add_line(const char *fmt, const char *arg)
{
    const char *prompt = JSTR("--スペースを押してください--", "--Press space to continue--");

    if (line_cnt == 0)
    {
	wclear(hw);
	jrogue_wset_normal_attr(hw);
    }

    if (line_cnt >= LINES - 1 || fmt == NULL)
    {
	wmove(hw, LINES - 1, 0);
	waddstr(hw, prompt);
	wrefresh(hw);
	jrogue_show_cursor(TRUE);
	wait_for(hw, ' ');
	jrogue_restore_cursor();
	clearok(curscr, TRUE);
	wclear(hw);
	touchwin(stdscr);
	line_cnt = 0;

	if (fmt == NULL)
	    return ~ESCAPE;
    }

    if (!(line_cnt == 0 && *fmt == '\0'))
	mvwprintw(hw, line_cnt++, 0, fmt, arg);

    return ~ESCAPE;
}

/*
 * end_line:
 *	End the list of lines
 */

void
end_line(void)
{
    add_line(NULL, NULL);
    line_cnt = 0;
}

/*
 * nothing:
 *	Set up prbuf so that message for "nothing found" is there
 */
const char *
nothing(int type)
{
    char *sp;
    const char *tystr = NULL;

    if (terse)
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（知っている", "Nothing"));
    else
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（これまでに明らかにされた", "Haven't discovered anything"));
    if (type != '*')
    {
	sp = &prbuf[strlen(prbuf)];
	switch (type)
	{
	    case POTION: tystr = JSTR("水薬", "potion");
	    when SCROLL: tystr = JSTR("巻き物", "scroll");
	    when RING: tystr = JSTR("指輪", "ring");
	    when STICK: tystr = JSTR("つえまたは魔法の棒", "stick");
	}
	snprintf(sp, PRBUF_ROOM(sp), JSTR("%sはない）", " about any %ss"), tystr);
    }
    return prbuf;
}

/*
 * nameit:
 *	Give the proper name to a potion, stick, or ring
 */

static const char *
ring_num_prefix_en(const THING *obj)
{
    static char buf[64];

    if (!(obj->o_flags & ISKNOW))
	return "";
    switch (obj->o_which)
    {
	case R_PROTECT:
	case R_ADDSTR:
	case R_ADDDAM:
	case R_ADDHIT:
	    snprintf(buf, sizeof buf, " %s", num(obj->o_arm, 0, RING));
	    return buf;
    }
    return "";
}

static void
nameit_en(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *))
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    snprintf(prbuf, PRBUF_SIZE, "A %s ", type);
	else
	    snprintf(prbuf, PRBUF_SIZE, "%d %ss ", obj->o_count, type);
	pb = &prbuf[strlen(prbuf)];
	if (op->oi_know)
	{
	    if (terse)
		snprintf(pb, PRBUF_ROOM(pb), "of %s%s", op->oi_name, (*prfunc)(obj));
	    else
		snprintf(pb, PRBUF_ROOM(pb), "of %s%s(%s)", op->oi_name, (*prfunc)(obj), which);
	}
	else if (op->oi_guess)
	{
	    if (terse)
		snprintf(pb, PRBUF_ROOM(pb), "called %s%s", op->oi_guess, (*prfunc)(obj));
	    else
		snprintf(pb, PRBUF_ROOM(pb), "called %s%s(%s)", op->oi_guess, (*prfunc)(obj), which);
	}
    }
    else if (obj->o_count == 1)
	snprintf(prbuf, PRBUF_SIZE, "A%s %s %s", vowelstr(which), which, type);
    else
	snprintf(prbuf, PRBUF_SIZE, "%d %s %ss", obj->o_count, which, type);
}

static void
nameit_ja(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *))
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    prbuf[0] = '\0';
	else
	    snprintf(prbuf, PRBUF_SIZE, "%d個の", obj->o_count);
	pb = &prbuf[strlen(prbuf)];
	if (op->oi_know)
	{
	    if (terse)
		snprintf(pb, PRBUF_ROOM(pb), "%s%s%s", op->oi_name, type, (*prfunc)(obj));
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s%s%s（%s）", op->oi_name, type, (*prfunc)(obj), which);
	}
	else if (op->oi_guess)
	{
	    if (terse)
		snprintf(pb, PRBUF_ROOM(pb), "『%s』と呼ぶ%s%s", op->oi_guess, type, (*prfunc)(obj));
	    else
		snprintf(pb, PRBUF_ROOM(pb), "『%s』と呼ぶ%s%s（%s）", op->oi_guess, type, (*prfunc)(obj), which);
	}
    }
    else if (obj->o_count == 1)
	snprintf(prbuf, PRBUF_SIZE, "%sの%s", which, type);
    else
	snprintf(prbuf, PRBUF_SIZE, "%d個の%sの%s", obj->o_count, which, type);
}

void
nameit(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *))
{
    if (jrogue_lang_is_japanese)
	nameit_ja(obj, type, which, op, prfunc);
    else
	nameit_en(obj, type, which, op, prfunc);
}

/*
 * nullstr:
 *	Return a pointer to a null-length string
 */
const char *
nullstr(const THING *ignored)
{
    (void)ignored;
    return "";
}

# ifdef	MASTER
/*
 * pr_list:
 *	List possible potions, scrolls, etc. for wizard.
 */

void
pr_list(void)
{
    int ch;

//    if (!terse)
//	addmsg("for ");
    addmsg(JSTR("どのタイプ", "what type"));
    if (!terse)
	addmsg(JSTR("の物をリストにする", " of object do you want a list"));
    msg(JSTR("？", "? "));
    ch = readchar();
	msg("");
    switch (ch)
    {
	case POTION:
	    pr_spec(pot_info, MAXPOTIONS);
	when SCROLL:
	    pr_spec(scr_info, MAXSCROLLS);
	when RING:
	    pr_spec(ring_info, MAXRINGS);
	when STICK:
	    pr_spec(ws_info, MAXSTICKS);
	when ARMOR:
	    pr_spec(arm_info, MAXARMORS);
	when WEAPON:
	    pr_spec(weap_info, MAXWEAPONS);
	otherwise:
	    return;
    }
}

/*
 * pr_spec:
 *	Print specific list of possible items to choose from
 */

void
pr_spec(const struct obj_info *info, int nitems)
{
    const struct obj_info *endp;
    int i, lastprob;

    endp = &info[nitems];
    lastprob = 0;
    for (i = '0'; info < endp; i++)
    {
	if (i == '9' + 1)
	    i = 'a';
	snprintf(prbuf, PRBUF_SIZE, "%c: %%s (%d%%%%)", i, info->oi_prob - lastprob);
	lastprob = info->oi_prob;
	add_line(prbuf, info->oi_name);
	info++;
    }
    end_line();
}
# endif	/* MASTER */
