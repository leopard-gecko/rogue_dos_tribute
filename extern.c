/*
 * global variable initializaton
 *
 * @(#)extern.c	4.82 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/* All global variables are defined here, or in vers.c or init.c */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#endif
#include "rogue.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

int  after;				/* True if we want after daemons */
int  again;				/* Repeating the last command */
int  noscore;				/* Was a wizard sometime */
int  seenstairs;			/* Have seen the stairs (for lsd) */
int  amulet = FALSE;			/* He has the amulet */
int  saw_amulet = FALSE;		/* He has found the amulet */
int  door_stop = FALSE;			/* Stop running when we pass a door */
int  firstmove = FALSE;			/* First move after setting door_stop */
int  got_ltc = FALSE;			/* We have gotten the local tty chars */
int  has_hit = FALSE;			/* Has a "hit" message pending in msg */
int  in_shell = FALSE;			/* True if executing a shell */
int  inv_describe = TRUE;		/* Say which way items are being used */
int  jump = FALSE;			/* Show running as series of jumps */
int  dos_keys = TRUE;		/* Use DOS-style key bindings */
int  kamikaze = FALSE;			/* to_death really to DEATH */
int  lower_msg = FALSE;			/* Messages should start w/lower case */
int  move_on = FALSE;			/* Next move shouldn't pick up items */
int  msg_esc = FALSE;			/* Check for ESC from msg's --More-- */
int  playing = TRUE;			/* True until he quits */
int  q_comm = FALSE;			/* Are we executing a 'Q' command? */
int  running = FALSE;			/* True if player is running */
int  save_msg = TRUE;			/* Remember last msg */
int  stat_msg = FALSE;			/* Should status() print as a msg() */
int  terse = FALSE;				/* True if we should be short */
int  to_death = FALSE;			/* Fighting is to the death! */
int  was_trapped = FALSE;			/* Show trap display effect after teleport */
#ifdef MASTER
int  wizard = FALSE;			/* True if allows wizard commands */
#endif

int  dir_ch;				/* Direction from last get_dir() call */
char file_name[MAXSTR];			/* Save file name */
char huh[MAXSTR];			/* The last message printed */
const char *p_colors[MAXPOTIONS];		/* Colors of the potions */
char prbuf[2*MAXSTR];			/* buffer for sprintfs */
const char *r_stones[MAXRINGS];		/* Stone settings of the rings */
int  runch;				/* Direction player is running */
char *s_names[MAXSCROLLS];		/* Names of the scrolls */
int scroll_syll_count[MAXSCROLLS];	/* Number of syllable IDs per scroll */
int scroll_syll_ids[MAXSCROLLS][MAX_SCROLL_SYLLABLES];
int scroll_syll_breaks[MAXSCROLLS][MAX_SCROLL_SYLLABLES];
int  take;				/* Thing she is taking */
char whoami[MAXSTR];			/* Name of player */
const char *ws_made[MAXSTICKS];		/* What sticks are made of */
const char *ws_type[MAXSTICKS];	/* Is it a wand or a staff */
int  orig_dsusp;			/* Original dsusp char */
char fruit[MAXSTR] =			/* Favorite fruit */
		{ JSTR_STATIC("ネバネバかび", "Slime Mold") };
char home[MAXSTR] = { '\0' };		/* User's home directory */
char inventory_menu[MAXSTR] = "on";		/* DOS-style inventory selection menu */
char dos_macro[41] = "v";		/* command macro */
char *dos_typeahead = "";			/* pending macro input */
int  l_last_comm = '\0';		/* Last last_comm */
int  l_last_dir = '\0';			/* Last last_dir */
int  last_comm = '\0';			/* Last command typed */
int  last_dir = '\0';			/* Last direction given */
const char *tr_name[] = {			/* Names of the traps */
	JSTR_STATIC("落とし戸のわな", "a trapdoor"),
	JSTR_STATIC("矢のわな", "an arrow trap"),
	JSTR_STATIC("眠りガスのわな", "a sleeping gas trap"),
	JSTR_STATIC("とらばさみのわな", "a beartrap"),
	JSTR_STATIC("テレポートのわな", "a teleport trap"),
	JSTR_STATIC("毒投げ矢のわな", "a poison dart trap")
};


int n_objs;				/* # items listed in inventory() call */
int ntraps;				/* Number of traps on this level */
int hungry_state = 0;			/* How hungry is he */
int inpack = 0;				/* Number of things in pack */
int level = 1;				/* What level she is on */
int max_hit;				/* Max damage done to her in to_death */
int max_level;				/* Deepest player has gone */
int mpos = 0;				/* Where cursor is on top line */
int no_food = 0;			/* Number of levels without food */
const int a_class[MAXARMORS] = {		/* Armor class for each armor type */
	8,	/* LEATHER */
	7,	/* RING_MAIL */
	7,	/* STUDDED_LEATHER */
	6,	/* SCALE_MAIL */
	5,	/* CHAIN_MAIL */
	4,	/* SPLINT_MAIL */
	4,	/* BANDED_MAIL */
	3,	/* PLATE_MAIL */
};

int count = 0;				/* Number of times to repeat command */
FILE *scoreboard = NULL;		/* File descriptor for score file */
int food_left;				/* Amount of food in hero's stomach */
int lastscore = -1;			/* Score before this turn */
int no_command = 0;			/* Number of turns asleep */
int no_move = 0;			/* Number of turns held in place */
int purse = 0;				/* How much gold he has */
int quiet = 0;				/* Number of quiet turns */
int vf_hit = 0;				/* Number of time flytrap has hit */
int _COLS;
int _LINES;

unsigned int dnum;				/* Dungeon number */
unsigned int seed;				/* Random number seed */
const int e_levels[] = {
        10L,
        20L,
        40L,
        80L,
       160L,
       320L,
       640L,
      1280L,
      2560L,
      5120L,
     10240L,
     20480L,
     40960L,
     81920L,
    163840L,
    327680L,
    655360L,
   1310720L,
   2621440L,
   5242880L,
         0L
};

const char *rank_names_en[] = {
    "",
    "Guild Novice",
    "Apprentice",
    "Journeyman",
    "Adventurer",
    "Fighter",
    "Warrior",
    "Rogue",
    "Champion",
    "Master Rogue",
    "Warlord",
    "Hero",
    "Guild Master",
    "Dragonlord",
    "Wizard",
    "Rogue Geek",
    "Rogue Addict",
    "Schmendrick",
    "Gunfighter",
    "Time Waster",
    "Bug Chaser"
};

const char *rank_names_ja[] = {
    "",
    "ギルドの新米",
    "見習い",
    "一人前",
    "冒険者",
    "戦士",
    "闘士",
    "無頼漢",
    "選ばれし勇者",
    "無頼の達人",
    "闘将",
    "英雄",
    "ギルドマスター",
    "竜王",
    "魔導士",
    "ローグマニア",
    "ローグ中毒者",
    "愚かな魔術師",
    "ガンマン",
    "時の浪費者",
    "バグ追い人"
};

int	show_cursor = FALSE;	/* Hide cursor during ordinary play */

size_t
jrogue_strlcpy(char *dst, const char *src, size_t dstsize)
{
    size_t srclen, copylen;

    if (src == NULL)
	src = "";
    srclen = strlen(src);
    if (dstsize != 0)
    {
	copylen = (srclen >= dstsize) ? dstsize - 1 : srclen;
	if (copylen != 0)
	    memcpy(dst, src, copylen);
	dst[copylen] = '\0';
    }
    return srclen;
}

size_t
jrogue_strlcat(char *dst, const char *src, size_t dstsize)
{
    size_t dstlen, srclen, copylen;

    if (src == NULL)
	src = "";
    dstlen = strlen(dst);
    srclen = strlen(src);
    if (dstlen >= dstsize)
	return dstsize + srclen;
    copylen = dstsize - dstlen - 1;
    if (copylen > srclen)
	copylen = srclen;
    if (copylen != 0)
	memcpy(dst + dstlen, src, copylen);
    dst[dstlen + copylen] = '\0';
    return dstlen + srclen;
}

void
jrogue_show_cursor(int show)
{
    curs_set(show ? 1 : 0);
}

void
jrogue_restore_cursor(void)
{
    jrogue_show_cursor(show_cursor);
}


void msleep(int sleeptime)
{
#ifdef _WIN32
    jrogue_windows_sleep_ms(sleeptime);
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = sleeptime * 1000;
    select(0, NULL, NULL, NULL, &tv);
#endif
}

coord delta;				/* Change indicated to get_dir() */
coord oldpos;				/* Position before last look() call */
coord stairs;				/* Location of staircase */

PLACE places[MAXLINES*MAXCOLS];		/* level map */

THING *cur_armor;			/* What he is wearing */
THING *cur_ring[2];			/* Which rings are being worn */
THING *cur_weapon;			/* Which weapon he is weilding */
THING *l_last_pick = NULL;		/* Last last_pick */
THING *last_pick = NULL;		/* Last object picked in get_item() */
THING *lvl_obj = NULL;			/* List of objects on this level */
THING *mlist = NULL;			/* List of monsters on the level */
THING player;				/* His stats */
					/* restart of game */

WINDOW *hw = NULL;			/* used as a scratch window */

#define INIT_STATS { 16, 0, 1, 10, 12, "1x4", 12 }

struct stats max_stats = INIT_STATS;	/* The maximum for the player */

struct room *oldrp;			/* Roomin(&oldpos) */
struct room rooms[MAXROOMS];		/* One for each room -- A level */
struct room passages[MAXPASS] =		/* One for each passage */
{
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} }
};

#define ___ 1
#define XX 10
struct monster monsters[26] =
    {
/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg */
{ JSTR_STATIC("水の怪物", "aquator"),        0, ISMEAN,             { XX,   20,  5,  2, ___, "0x0/0x0", 0 } },
{ JSTR_STATIC("こうもり", "bat"),             0, ISFLY,              { XX,    1,  1,  3, ___, "1x2", 0 } },
{ JSTR_STATIC("ケンタウロス", "centaur"),     15, 0,                  { XX,   25,  4,  4, ___, "1x6/1x6", 0 } },
{ JSTR_STATIC("ドラゴン", "dragon"),        100, ISMEAN,             { XX, 6800, 10, -1, ___, "1x8/1x8/3x10", 0 } },
{ JSTR_STATIC("エミュー", "emu"),             0, ISMEAN,             { XX,    2,  1,  7, ___, "1x2", 0 } },
{ JSTR_STATIC("ハエトリグサ", "venus flytrap"), 0, ISMEAN,           { XX,   80,  8,  3, ___, "0x0", 0 } },
{ JSTR_STATIC("グリフィン", "griffin"),       20, ISMEAN|ISFLY|ISREGEN, { XX, 2000, 13, 2, ___, "4x3/3x5/4x3", 0 } },
{ JSTR_STATIC("ホブゴブリン", "hobgoblin"),   0, ISMEAN,             { XX,    3,  1,  5, ___, "1x8", 0 } },
{ JSTR_STATIC("氷の怪物", "ice monster"),     0, ISMEAN,             { XX,   15,  1,  9, ___, "1x2", 0 } },
{ JSTR_STATIC("ジャバウォック", "jabberwock"), 70, 0,                { XX, 4000, 15,  6, ___, "2x12/2x4", 0 } },
{ JSTR_STATIC("はやぶさ", "kestrel"),          0, ISMEAN|ISFLY,       { XX,    1,  1,  7, ___, "1x4", 0 } },
{ JSTR_STATIC("レプラコーン", "leprechaun"),  64, 0,                 { XX,   10,  3,  8, ___, "1x2", 0 } },
{ JSTR_STATIC("メデューサ", "medusa"),        40, ISMEAN,             { XX,  200,  8,  2, ___, "3x4/3x4/2x5", 0 } },
{ JSTR_STATIC("ニンフ", "nymph"),            100, 0,                  { XX,   37,  3,  9, ___, "0x0", 0 } },
{ JSTR_STATIC("オーク", "orc"),              15, ISGREED,            { XX,    5,  1,  6, ___, "1x8", 0 } },
{ JSTR_STATIC("幽霊", "phantom"),             0, ISINVIS,            { XX,  120,  8,  3, ___, "4x4", 0 } },
{ JSTR_STATIC("半シマウマ", "quagga"),       30, ISMEAN,             { XX,   32,  3,  2, ___, "1x2/1x2/1x4", 0 } },
{ JSTR_STATIC("がらがらへび", "rattlesnake"), 0, ISMEAN,             { XX,    9,  2,  3, ___, "1x6", 0 } },
{ JSTR_STATIC("スライム", "slime"),           0, ISMEAN,             { XX,    1,  2,  8, ___, "1x3", 0 } },
{ JSTR_STATIC("トロル", "troll"),            50, ISREGEN|ISMEAN,     { XX,  120,  6,  4, ___, "1x8/1x8/2x6", 0 } },
{ JSTR_STATIC("ウル＝ヴァイル", "ur-vile"),  0, ISMEAN,             { XX,  190,  7, -2, ___, "1x3/1x3/1x3/4x6", 0 } },
{ JSTR_STATIC("ヴァンパイア", "vampire"),    20, ISREGEN|ISMEAN,     { XX,  350,  8,  1, ___, "1x10", 0 } },
{ JSTR_STATIC("死霊", "wraith"),              0, 0,                  { XX,   55,  5,  4, ___, "1x6", 0 } },
{ JSTR_STATIC("ゼロック", "xeroc"),          30, 0,                  { XX,  100,  7,  7, ___, "3x4", 0 } },
{ JSTR_STATIC("イエティ", "yeti"),           30, 0,                  { XX,   50,  4,  6, ___, "1x6/1x6", 0 } },
{ JSTR_STATIC("ゾンビ", "zombie"),            0, ISMEAN,             { XX,    6,  2,  8, ___, "1x8", 0 } }
    };
#undef ___
#undef XX

struct obj_info things[NUMTHINGS] = {
    { 0, 27, 0, NULL, FALSE },  /* potion */
    { 0, 30, 0, NULL, FALSE },  /* scroll */
    { 0, 17, 0, NULL, FALSE },  /* food */
    { 0, 8, 0, NULL, FALSE },  /* weapon */
    { 0, 8, 0, NULL, FALSE },  /* armor */
    { 0, 5, 0, NULL, FALSE },  /* ring */
    { 0, 5, 0, NULL, FALSE }   /* stick */
};

struct obj_info arm_info[MAXARMORS] = {
    { JSTR_STATIC("革のよろい", "leather armor"),		     20,	 20, NULL, FALSE },
    { JSTR_STATIC("鉄環のよろい", "ring mail"),		 15,	 25, NULL, FALSE },
    { JSTR_STATIC("鋲つき革のよろい", "studded leather armor"),	 15,	 20, NULL, FALSE },
    { JSTR_STATIC("鱗のよろい", "scale mail"),		     13,	 30, NULL, FALSE },
    { JSTR_STATIC("鎖のよろい", "chain mail"),		     12,	 75, NULL, FALSE },
    { JSTR_STATIC("鉄片のよろい", "splint mail"),		 10,	 80, NULL, FALSE },
    { JSTR_STATIC("帯金のよろい", "banded mail"),		 10,	 90, NULL, FALSE },
    { JSTR_STATIC("全身のよろい", "plate mail"),		  5,	150, NULL, FALSE },
};
struct obj_info pot_info[MAXPOTIONS] = {
    { JSTR_STATIC("頭が混乱する", "confusion"),              8,   5, NULL, FALSE },
    { JSTR_STATIC("体が動かなくなる", "paralysis"),         10,   5, NULL, FALSE },
    { JSTR_STATIC("毒の", "poison"),                         8,   5, NULL, FALSE },
    { JSTR_STATIC("強さが増す", "gain strength"),           15, 150, NULL, FALSE },
    { JSTR_STATIC("見えないものが見える", "see invisible"),  2, 100, NULL, FALSE },
    { JSTR_STATIC("傷を治す", "healing"),             15, 130, NULL, FALSE },
    { JSTR_STATIC("怪物探知の", "monster detection"),        6, 130, NULL, FALSE },
    { JSTR_STATIC("魔法探知の", "magic detection"),          6, 105, NULL, FALSE },
    { JSTR_STATIC("レベルが上がる", "raise level"),          2, 250, NULL, FALSE },
    { JSTR_STATIC("とてもよく傷を治す", "extra healing"),  5, 200, NULL, FALSE },
    { JSTR_STATIC("素早くなる", "haste self"),               4, 190, NULL, FALSE },
    { JSTR_STATIC("強さが元にもどる", "restore strength"),  14, 130, NULL, FALSE },
    { JSTR_STATIC("目が見えなくなる", "blindness"),          4,   5, NULL, FALSE },
    { JSTR_STATIC("のどをうるおす", "thirst quenching"),      1,   5, NULL, FALSE },
};
struct obj_info ring_info[MAXRINGS] = {
    { JSTR_STATIC("守りの", "protection"),		                 9, 400, NULL, FALSE },
    { JSTR_STATIC("強さが増す", "add strength"),		             9, 400, NULL, FALSE },
    { JSTR_STATIC("強さが減らない", "sustain strength"),	             5, 280, NULL, FALSE },
    { JSTR_STATIC("探索の", "searching"),		                10, 420, NULL, FALSE },
    { JSTR_STATIC("見えないものが見える", "see invisible"),		10, 310, NULL, FALSE },
    { JSTR_STATIC("飾りものの", "adornment"),		             1,  10, NULL, FALSE },
    { JSTR_STATIC("怪物を怒らせる", "aggravate monster"),	            10,  10, NULL, FALSE },
    { JSTR_STATIC("機敏になる", "dexterity"),		             8, 440, NULL, FALSE },
    { JSTR_STATIC("ダメージを増やす", "increase damage"),	         8, 400, NULL, FALSE },
    { JSTR_STATIC("再生の", "regeneration"),		                 4, 460, NULL, FALSE },
    { JSTR_STATIC("消化が遅くなる", "slow digestion"),	        	 9, 240, NULL, FALSE },
    { JSTR_STATIC("テレポートする", "teleportation"),	        	 5,  30, NULL, FALSE },
    { JSTR_STATIC("身を隠す", "stealth"),		             7, 470, NULL, FALSE },
    { JSTR_STATIC("よろいが丈夫になる", "maintain armor"),	     	 5, 380, NULL, FALSE },
};
struct obj_info scr_info[MAXSCROLLS] = {
    { JSTR_STATIC("怪物を混乱させる", "monster confusion"),       8, 140, NULL, FALSE },
    { JSTR_STATIC("魔法の地図の", "magic mapping"),               5, 150, NULL, FALSE },
    { JSTR_STATIC("怪物を封じこめる", "hold monster"),            3, 180, NULL, FALSE },
    { JSTR_STATIC("眠りにおちる", "sleep"),                       5,   5, NULL, FALSE },
    { JSTR_STATIC("よろいに魔法をかける", "enchant armor"),       8, 160, NULL, FALSE },
    { JSTR_STATIC("持ちものの種類がわかる", "identify"),         27, 100, NULL, FALSE },
    { JSTR_STATIC("怪物を怖がらせる", "scare monster"),           4, 200, NULL, FALSE },
    { JSTR_STATIC("食糧を見つける", "food detection"),          4,  50, NULL, FALSE },
    { JSTR_STATIC("テレポートする", "teleportation"),             7, 165, NULL, FALSE },
    { JSTR_STATIC("武器に魔法をかける", "enchant weapon"),       10, 150, NULL, FALSE },
    { JSTR_STATIC("怪物を作りだす", "create monster"),            5,  75, NULL, FALSE },
    { JSTR_STATIC("のろいを解く", "remove curse"),                8, 105, NULL, FALSE },
    { JSTR_STATIC("怪物を怒らせる", "aggravate monsters"),        4,  20, NULL, FALSE },
    { JSTR_STATIC("白紙の", "blank paper"),                       1,   5, NULL, FALSE },
    { JSTR_STATIC("武器を怪物殺しにする", "vorpalize weapon"),    1, 300, NULL, FALSE },
};

struct obj_info weap_info[MAXWEAPONS + 1] = {
    { JSTR_STATIC("つちほこ", "mace"),                 10,   8, NULL, FALSE },
    { JSTR_STATIC("長い剣", "long sword"),             10,  15, NULL, FALSE },
    { JSTR_STATIC("短弓", "short bow"),                10,  15, NULL, FALSE },
    { JSTR_STATIC("矢", "arrow"),                      10,   1, NULL, FALSE },
    { JSTR_STATIC("短剣", "dagger"),                   10,   2, NULL, FALSE },
    { JSTR_STATIC("両手持ちの剣", "two handed sword"), 10,  75, NULL, FALSE },
    { JSTR_STATIC("投げ矢", "dart"),                  10,   1, NULL, FALSE },
    { JSTR_STATIC("クロスボウ", "crossbow"),           10,  30, NULL, FALSE },
    { JSTR_STATIC("クロスボウの矢", "crossbow bolt"), 10,   1, NULL, FALSE },
    { JSTR_STATIC("やり", "spear"),                   10,   5, NULL, FALSE },
    { NULL, 0, 0, NULL, FALSE },    /* DO NOT REMOVE: fake entry for dragon/ice monster breath */
};
struct obj_info ws_info[MAXSTICKS] = {
    { JSTR_STATIC("灯りの", "light"),               12, 250, NULL, FALSE },
    { JSTR_STATIC("打撃の", "striking"),             9,  75, NULL, FALSE },
    { JSTR_STATIC("いなずまの", "lightning"),        3, 330, NULL, FALSE },
    { JSTR_STATIC("炎の", "fire"),                  3, 330, NULL, FALSE },
    { JSTR_STATIC("冷気の", "cold"),                 3, 330, NULL, FALSE },
    { JSTR_STATIC("変身させる", "polymorph"),       15, 310, NULL, FALSE },
    { JSTR_STATIC("魔法のミサイルの", "magic missile"), 10, 170, NULL, FALSE },
    { JSTR_STATIC("怪物を素早くする", "haste monster"), 9,   5, NULL, FALSE },
    { JSTR_STATIC("怪物を遅くする", "slow monster"), 11, 350, NULL, FALSE },
    { JSTR_STATIC("いのちを削る", "drain life"),     9, 300, NULL, FALSE },
    { JSTR_STATIC("役に立たない", "nothing"),        1,   5, NULL, FALSE },
    { JSTR_STATIC("怪物を遠くに飛ばす", "teleport away"), 5, 340, NULL, FALSE },
    { JSTR_STATIC("怪物を引き寄せる", "teleport to"),    5,  50, NULL, FALSE },
    { JSTR_STATIC("魔力を封じる", "cancellation"),   5, 280, NULL, FALSE },
};

struct h_list helpstr[] = {
    {'?',	JSTR_STATIC("	ヘルプを表示する", "	prints help"),				TRUE},
    {'/',	JSTR_STATIC("	画面記号の一覧を表示", "	list of symbols"),			TRUE},
    {'h',	JSTR_STATIC("	左に進む", "	left"),					TRUE},
    {'j',	JSTR_STATIC("	下に進む", "	down"),					TRUE},
    {'k',	JSTR_STATIC("	上に進む", "	up"),					TRUE},
    {'l',	JSTR_STATIC("	右に進む", "	right"),					TRUE},
    {'y',	JSTR_STATIC("	左上に進む", "	up & left"),				TRUE},
    {'u',	JSTR_STATIC("	右上に進む", "	up & right"),				TRUE},
    {'b',	JSTR_STATIC("	左下に進む", "	down & left"),				TRUE},
    {'n',	JSTR_STATIC("	右下に進む", "	down & right"),				TRUE},
    {'H',	JSTR_STATIC("	左に走る", "	run left"),				FALSE},
    {'J',	JSTR_STATIC("	下に走る", "	run down"),				FALSE},
    {'K',	JSTR_STATIC("	上に走る", "	run up"),				FALSE},
    {'L',	JSTR_STATIC("	右に走る", "	run right"),				FALSE},
    {'Y',	JSTR_STATIC("	左上に走る", "	run up & left"),				FALSE},
    {'U',	JSTR_STATIC("	右上に走る", "	run up & right"),			FALSE},
    {'B',	JSTR_STATIC("	左下に走る", "	run down & left"),			FALSE},
    {'N',	JSTR_STATIC("	右下に走る", "	run down & right"),			FALSE},
    {CTRL('H'),	JSTR_STATIC("	何かを見つけるまで左に進む", "	run left until adjacent"),		FALSE},
    {CTRL('J'),	JSTR_STATIC("	何かを見つけるまで下に進む", "	run down until adjacent"),		FALSE},
    {CTRL('K'),	JSTR_STATIC("	何かを見つけるまで上に進む", "	run up until adjacent"),			FALSE},
    {CTRL('L'),	JSTR_STATIC("	何かを見つけるまで右に進む", "	run right until adjacent"),		FALSE},
    {CTRL('Y'),	JSTR_STATIC("	何かを見つけるまで左上に進む", "	run up & left until adjacent"),		FALSE},
    {CTRL('U'),	JSTR_STATIC("	何かを見つけるまで右上に進む", "	run up & right until adjacent"),		FALSE},
    {CTRL('B'),	JSTR_STATIC("	何かを見つけるまで左下に進む", "	run down & left until adjacent"),	FALSE},
    {CTRL('N'),	JSTR_STATIC("	何かを見つけるまで右下に進む", "	run down & right until adjacent"),	FALSE},
    {'\0',	JSTR_STATIC("<SHIFT><方角>: 走る", "	<SHIFT><dir>: run that way"),		TRUE},
    {'\0',	JSTR_STATIC("<CTRL><方角>: 何かを見つけるまで走る", "	<CTRL><dir>: run till adjacent"),	TRUE},
    {'f',	JSTR_STATIC("<方角>	やられそうになるまで戦う", "<dir>	fight till death or near death"),	TRUE},
    {'t',	JSTR_STATIC("<方角>	物を投げる", "<dir>	throw something"),			TRUE},
    {'m',	JSTR_STATIC("<方角>	落ちているものの上に乗る", "<dir>	move onto without picking up"),	TRUE},
    {'z',	JSTR_STATIC("<方角>	つえ/魔法の棒を振りかざす", "<dir>	zap a wand in a direction"),		TRUE},
    {'^',	JSTR_STATIC("<方角>	わなの種類を調べる", "<dir>	identify trap type"),			TRUE},
    {'s',	JSTR_STATIC("	わなと隠しとびらを探す", "	search for trap/secret door"),		TRUE},
    {'>',	JSTR_STATIC("	階段を降りる", "	go down a staircase"),			TRUE},
    {'<',	JSTR_STATIC("	階段を上る", "	go up a staircase"),			TRUE},
    {'.',	JSTR_STATIC("	その場で休む", "	rest for a turn"),			TRUE},
    {',',	JSTR_STATIC("	足元のものを拾う", "	pick something up"),			TRUE},
    {'i',	JSTR_STATIC("	持ちものを見る", "	inventory"),				TRUE},
    {'I',	JSTR_STATIC("	１つの持ちものを見る", "	inventory single item"),			TRUE},
    {'q',	JSTR_STATIC("	水薬を飲む", "	quaff potion"),				TRUE},
    {'r',	JSTR_STATIC("	巻き物を読む", "	read scroll"),				TRUE},
    {'e',	JSTR_STATIC("	食糧を食べる", "	eat food"),				TRUE},
    {'w',	JSTR_STATIC("	武器を手に持つ", "	wield a weapon"),			TRUE},
    {'W',	JSTR_STATIC("	よろいを着る", "	wear armor"),				TRUE},
    {'T',	JSTR_STATIC("	よろいを脱ぐ", "	take armor off"),			TRUE},
    {'P',	JSTR_STATIC("	指輪をはめる", "	put on ring"),				TRUE},
    {'R',	JSTR_STATIC("	指輪をはずす", "	remove ring"),				TRUE},
    {'d',	JSTR_STATIC("	持ちものを落とす", "	drop object"),				TRUE},
    {'c',	JSTR_STATIC("	持ちものに呼び名をつける", "	call object"),				TRUE},
    {'a',	JSTR_STATIC("	直前のコマンドを繰り返す", "	repeat last command"),			TRUE},
    {')',	JSTR_STATIC("	現在の武器を見る", "	print current weapon"),			TRUE},
    {']',	JSTR_STATIC("	現在のよろいを見る", "	print current armor"),			TRUE},
    {'=',	JSTR_STATIC("	現在の指輪を見る", "	print current rings"),			TRUE},
    {'@',	JSTR_STATIC("	現在のステータスを表示", "	print current stats"),			TRUE},
    {'D',	JSTR_STATIC("	今まで何を発見したかを表示", "	recall what's been discovered"),		TRUE},
    {'o',	JSTR_STATIC("	オプションの表示・設定", "	examine/set options"),			TRUE},
    {'M',	JSTR_STATIC("	マクロを登録・変更する", "	define/change macro"),			TRUE},
    {'x',	JSTR_STATIC("	登録したマクロを実行する", "	run macro"),			TRUE},
    {CTRL('R'),	JSTR_STATIC("	画面を描き直す", "	redraw screen"),				TRUE},
    {CTRL('P'),	JSTR_STATIC("	最後のメッセージを再表示する", "	repeat last message"),			TRUE},
    {ESCAPE,	JSTR_STATIC("	コマンドを中止する", "	cancel command"),			TRUE},
    {'S',	JSTR_STATIC("	ゲームをセーブして終了する", "	save game"),				TRUE},
    {'Q',	JSTR_STATIC("	ゲームを終了する", "	quit"),					TRUE},
    {'!',	JSTR_STATIC("	シェルに抜ける", "	shell escape"),				TRUE},
    {'F',	JSTR_STATIC("<方角>	どちらかが倒れるまで戦う", "<dir>	fight till either of you dies"),		TRUE},
    {'v',	JSTR_STATIC("	バージョン情報を表示する", "	print version number"),			TRUE},
    {0,		NULL, FALSE }
};
/* numscores and Numname are defined in mach_dep.c. */
int between;

#define _X_ { EMPTY }

struct delayed_action d_list[MAXDAEMONS] = {
    _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,
    _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,
};

int group = 2;


static void
jrogue_apply_default_fruit(void)
{
    if (strcmp(fruit, "slime-mold") == 0 ||
	strcmp(fruit, "Slime Mold") == 0 ||
	strcmp(fruit, "ネバネバかび") == 0)
	jrogue_strlcpy(fruit, JSTR("ネバネバかび", "Slime Mold"), MAXSTR);
}

void
jrogue_apply_language(void)
{
    jrogue_apply_init_language();
    jrogue_apply_fight_language();
    jrogue_apply_options_language();
    jrogue_apply_potions_language();
    jrogue_apply_rip_language();
    Numname = JSTR("テン", "Ten");
    jrogue_apply_default_fruit();
    jrogue_rebuild_scroll_names();
    tr_name[0] = JSTR("落とし戸のわな", "a trapdoor");
    tr_name[1] = JSTR("矢のわな", "an arrow trap");
    tr_name[2] = JSTR("眠りガスのわな", "a sleeping gas trap");
    tr_name[3] = JSTR("とらばさみのわな", "a beartrap");
    tr_name[4] = JSTR("テレポートのわな", "a teleport trap");
    tr_name[5] = JSTR("毒投げ矢のわな", "a poison dart trap");
    monsters[0].m_name = JSTR("水の怪物", "aquator");
    monsters[1].m_name = JSTR("こうもり", "bat");
    monsters[2].m_name = JSTR("ケンタウロス", "centaur");
    monsters[3].m_name = JSTR("ドラゴン", "dragon");
    monsters[4].m_name = JSTR("エミュー", "emu");
    monsters[5].m_name = JSTR("ハエトリグサ", "venus flytrap");
    monsters[6].m_name = JSTR("グリフィン", "griffin");
    monsters[7].m_name = JSTR("ホブゴブリン", "hobgoblin");
    monsters[8].m_name = JSTR("氷の怪物", "ice monster");
    monsters[9].m_name = JSTR("ジャバウォック", "jabberwock");
    monsters[10].m_name = JSTR("はやぶさ", "kestrel");
    monsters[11].m_name = JSTR("レプラコーン", "leprechaun");
    monsters[12].m_name = JSTR("メデューサ", "medusa");
    monsters[13].m_name = JSTR("ニンフ", "nymph");
    monsters[14].m_name = JSTR("オーク", "orc");
    monsters[15].m_name = JSTR("幽霊", "phantom");
    monsters[16].m_name = JSTR("半シマウマ", "quagga");
    monsters[17].m_name = JSTR("がらがらへび", "rattlesnake");
    monsters[18].m_name = JSTR("スライム", "slime");
    monsters[19].m_name = JSTR("トロル", "troll");
    monsters[20].m_name = JSTR("ウル＝ヴァイル", "ur-vile");
    monsters[21].m_name = JSTR("ヴァンパイア", "vampire");
    monsters[22].m_name = JSTR("死霊", "wraith");
    monsters[23].m_name = JSTR("ゼロック", "xeroc");
    monsters[24].m_name = JSTR("イエティ", "yeti");
    monsters[25].m_name = JSTR("ゾンビ", "zombie");
    jrogue_strlcpy(monsters[5].m_stats.s_dmg, "0x0", sizeof monsters[5].m_stats.s_dmg);
    arm_info[0].oi_name = JSTR("革のよろい", "leather armor");
    arm_info[1].oi_name = JSTR("鉄環のよろい", "ring mail");
    arm_info[2].oi_name = JSTR("鋲つき革のよろい", "studded leather armor");
    arm_info[3].oi_name = JSTR("鱗のよろい", "scale mail");
    arm_info[4].oi_name = JSTR("鎖のよろい", "chain mail");
    arm_info[5].oi_name = JSTR("鉄片のよろい", "splint mail");
    arm_info[6].oi_name = JSTR("帯金のよろい", "banded mail");
    arm_info[7].oi_name = JSTR("全身のよろい", "plate mail");
    pot_info[0].oi_name = JSTR("頭が混乱する", "confusion");
    pot_info[1].oi_name = JSTR("体が動かなくなる", "paralysis");
    pot_info[2].oi_name = JSTR("毒の", "poison");
    pot_info[3].oi_name = JSTR("強さが増す", "gain strength");
    pot_info[4].oi_name = JSTR("見えないものが見える", "see invisible");
    pot_info[5].oi_name = JSTR("傷を治す", "healing");
    pot_info[6].oi_name = JSTR("怪物探知の", "monster detection");
    pot_info[7].oi_name = JSTR("魔法探知の", "magic detection");
    pot_info[8].oi_name = JSTR("レベルが上がる", "raise level");
    pot_info[9].oi_name = JSTR("とてもよく傷を治す", "extra healing");
    pot_info[10].oi_name = JSTR("素早くなる", "haste self");
    pot_info[11].oi_name = JSTR("強さが元にもどる", "restore strength");
    pot_info[12].oi_name = JSTR("目が見えなくなる", "blindness");
    pot_info[13].oi_name = JSTR("のどをうるおす", "thirst quenching");
    ring_info[0].oi_name = JSTR("守りの", "protection");
    ring_info[1].oi_name = JSTR("強さが増す", "add strength");
    ring_info[2].oi_name = JSTR("強さが減らない", "sustain strength");
    ring_info[3].oi_name = JSTR("探索の", "searching");
    ring_info[4].oi_name = JSTR("見えないものが見える", "see invisible");
    ring_info[5].oi_name = JSTR("飾りものの", "adornment");
    ring_info[6].oi_name = JSTR("怪物を怒らせる", "aggravate monster");
    ring_info[7].oi_name = JSTR("機敏になる", "dexterity");
    ring_info[8].oi_name = JSTR("ダメージを増やす", "increase damage");
    ring_info[9].oi_name = JSTR("再生の", "regeneration");
    ring_info[10].oi_name = JSTR("消化が遅くなる", "slow digestion");
    ring_info[11].oi_name = JSTR("テレポートする", "teleportation");
    ring_info[12].oi_name = JSTR("身を隠す", "stealth");
    ring_info[13].oi_name = JSTR("よろいが丈夫になる", "maintain armor");
    scr_info[0].oi_name = JSTR("怪物を混乱させる", "monster confusion");
    scr_info[1].oi_name = JSTR("魔法の地図の", "magic mapping");
    scr_info[2].oi_name = JSTR("怪物を封じこめる", "hold monster");
    scr_info[3].oi_name = JSTR("眠りにおちる", "sleep");
    scr_info[4].oi_name = JSTR("よろいに魔法をかける", "enchant armor");
    scr_info[5].oi_name = JSTR("持ちものの種類がわかる", "identify");
    scr_info[6].oi_name = JSTR("怪物を怖がらせる", "scare monster");
    scr_info[7].oi_name = JSTR("食糧を見つける", "food detection");
    scr_info[8].oi_name = JSTR("テレポートする", "teleportation");
    scr_info[9].oi_name = JSTR("武器に魔法をかける", "enchant weapon");
    scr_info[10].oi_name = JSTR("怪物を作りだす", "create monster");
    scr_info[11].oi_name = JSTR("のろいを解く", "remove curse");
    scr_info[12].oi_name = JSTR("怪物を怒らせる", "aggravate monsters");
    scr_info[13].oi_name = JSTR("白紙の", "blank paper");
    scr_info[14].oi_name = JSTR("武器を怪物殺しにする", "vorpalize weapon");
    weap_info[0].oi_name = JSTR("つちほこ", "mace");
    weap_info[1].oi_name = JSTR("長い剣", "long sword");
    weap_info[2].oi_name = JSTR("短弓", "short bow");
    weap_info[3].oi_name = JSTR("矢", "arrow");
    weap_info[4].oi_name = JSTR("短剣", "dagger");
    weap_info[5].oi_name = JSTR("両手持ちの剣", "two handed sword");
    weap_info[6].oi_name = JSTR("投げ矢", "dart");
    weap_info[7].oi_name = JSTR("クロスボウ", "crossbow");
    weap_info[8].oi_name = JSTR("クロスボウの矢", "crossbow bolt");
    weap_info[9].oi_name = JSTR("やり", "spear");
    ws_info[0].oi_name = JSTR("灯りの", "light");
    ws_info[1].oi_name = JSTR("打撃の", "striking");
    ws_info[2].oi_name = JSTR("いなずまの", "lightning");
    ws_info[3].oi_name = JSTR("炎の", "fire");
    ws_info[4].oi_name = JSTR("冷気の", "cold");
    ws_info[5].oi_name = JSTR("変身させる", "polymorph");
    ws_info[6].oi_name = JSTR("魔法のミサイルの", "magic missile");
    ws_info[7].oi_name = JSTR("怪物を素早くする", "haste monster");
    ws_info[8].oi_name = JSTR("怪物を遅くする", "slow monster");
    ws_info[9].oi_name = JSTR("いのちを削る", "drain life");
    ws_info[10].oi_name = JSTR("役に立たない", "nothing");
    ws_info[11].oi_name = JSTR("怪物を遠くに飛ばす", "teleport away");
    ws_info[12].oi_name = JSTR("怪物を引き寄せる", "teleport to");
    ws_info[13].oi_name = JSTR("魔力を封じる", "cancellation");
    helpstr[0].h_desc = JSTR("	ヘルプを表示する", "	prints help");
    helpstr[1].h_desc = JSTR("	画面記号の一覧を表示", "	list of symbols");
    helpstr[2].h_desc = JSTR("	左に進む", "	left");
    helpstr[3].h_desc = JSTR("	下に進む", "	down");
    helpstr[4].h_desc = JSTR("	上に進む", "	up");
    helpstr[5].h_desc = JSTR("	右に進む", "	right");
    helpstr[6].h_desc = JSTR("	左上に進む", "	up & left");
    helpstr[7].h_desc = JSTR("	右上に進む", "	up & right");
    helpstr[8].h_desc = JSTR("	左下に進む", "	down & left");
    helpstr[9].h_desc = JSTR("	右下に進む", "	down & right");
    helpstr[10].h_desc = JSTR("	左に走る", "	run left");
    helpstr[11].h_desc = JSTR("	下に走る", "	run down");
    helpstr[12].h_desc = JSTR("	上に走る", "	run up");
    helpstr[13].h_desc = JSTR("	右に走る", "	run right");
    helpstr[14].h_desc = JSTR("	左上に走る", "	run up & left");
    helpstr[15].h_desc = JSTR("	右上に走る", "	run up & right");
    helpstr[16].h_desc = JSTR("	左下に走る", "	run down & left");
    helpstr[17].h_desc = JSTR("	右下に走る", "	run down & right");
    helpstr[18].h_desc = JSTR("	何かを見つけるまで左に進む", "	run left until adjacent");
    helpstr[19].h_desc = JSTR("	何かを見つけるまで下に進む", "	run down until adjacent");
    helpstr[20].h_desc = JSTR("	何かを見つけるまで上に進む", "	run up until adjacent");
    helpstr[21].h_desc = JSTR("	何かを見つけるまで右に進む", "	run right until adjacent");
    helpstr[22].h_desc = JSTR("	何かを見つけるまで左上に進む", "	run up & left until adjacent");
    helpstr[23].h_desc = JSTR("	何かを見つけるまで右上に進む", "	run up & right until adjacent");
    helpstr[24].h_desc = JSTR("	何かを見つけるまで左下に進む", "	run down & left until adjacent");
    helpstr[25].h_desc = JSTR("	何かを見つけるまで右下に進む", "	run down & right until adjacent");
    helpstr[26].h_desc = JSTR("<SHIFT><方角>: 走る", "	<SHIFT><dir>: run that way");
    helpstr[27].h_desc = JSTR("<CTRL><方角>: 何かを見つけるまで走る", "	<CTRL><dir>: run till adjacent");
    helpstr[28].h_desc = JSTR("<方角>	やられそうになるまで戦う", "<dir>	fight till death or near death");
    helpstr[29].h_desc = JSTR("<方角>	物を投げる", "<dir>	throw something");
    helpstr[30].h_desc = JSTR("<方角>	落ちているものの上に乗る", "<dir>	move onto without picking up");
    helpstr[31].h_desc = JSTR("<方角>	つえ/魔法の棒を振りかざす", "<dir>	zap a wand in a direction");
    helpstr[32].h_desc = JSTR("<方角>	わなの種類を調べる", "<dir>	identify trap type");
    helpstr[33].h_desc = JSTR("	わなと隠しとびらを探す", "	search for trap/secret door");
    helpstr[34].h_desc = JSTR("	階段を降りる", "	go down a staircase");
    helpstr[35].h_desc = JSTR("	階段を上る", "	go up a staircase");
    helpstr[36].h_desc = JSTR("	その場で休む", "	rest for a turn");
    helpstr[37].h_desc = JSTR("	足元のものを拾う", "	pick something up");
    helpstr[38].h_desc = JSTR("	持ちものを見る", "	inventory");
    helpstr[39].h_desc = JSTR("	１つの持ちものを見る", "	inventory single item");
    helpstr[40].h_desc = JSTR("	水薬を飲む", "	quaff potion");
    helpstr[41].h_desc = JSTR("	巻き物を読む", "	read scroll");
    helpstr[42].h_desc = JSTR("	食糧を食べる", "	eat food");
    helpstr[43].h_desc = JSTR("	武器を手に持つ", "	wield a weapon");
    helpstr[44].h_desc = JSTR("	よろいを着る", "	wear armor");
    helpstr[45].h_desc = JSTR("	よろいを脱ぐ", "	take armor off");
    helpstr[46].h_desc = JSTR("	指輪をはめる", "	put on ring");
    helpstr[47].h_desc = JSTR("	指輪をはずす", "	remove ring");
    helpstr[48].h_desc = JSTR("	持ちものを落とす", "	drop object");
    helpstr[49].h_desc = JSTR("	持ちものに呼び名をつける", "	call object");
    helpstr[50].h_desc = JSTR("	直前のコマンドを繰り返す", "	repeat last command");
    helpstr[51].h_desc = JSTR("	現在の武器を見る", "	print current weapon");
    helpstr[52].h_desc = JSTR("	現在のよろいを見る", "	print current armor");
    helpstr[53].h_desc = JSTR("	現在の指輪を見る", "	print current rings");
    helpstr[54].h_desc = JSTR("	現在のステータスを表示", "	print current stats");
    helpstr[55].h_desc = JSTR("	今まで何を発見したかを表示", "	recall what's been discovered");
    helpstr[56].h_desc = JSTR("	オプションの表示・設定", "	examine/set options");
    helpstr[57].h_desc = JSTR("	マクロを登録・変更する", "	define/change macro");
    helpstr[58].h_desc = JSTR("	登録したマクロを実行する", "	run macro");
    helpstr[59].h_desc = JSTR("	画面を描き直す", "	redraw screen");
    helpstr[60].h_desc = JSTR("	最後のメッセージを再表示する", "	repeat last message");
    helpstr[61].h_desc = JSTR("	コマンドを中止する", "	cancel command");
    helpstr[62].h_desc = JSTR("	ゲームをセーブして終了する", "	save game");
    helpstr[63].h_desc = JSTR("	ゲームを終了する", "	quit");
    helpstr[64].h_desc = JSTR("	シェルに抜ける", "	shell escape");
    helpstr[65].h_desc = JSTR("<方角>	どちらかが倒れるまで戦う", "<dir>	fight till either of you dies");
    helpstr[66].h_desc = JSTR("	バージョン情報を表示する", "	print version number");
}
