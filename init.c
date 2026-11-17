/*
 * global variable initializaton
 *
 * @(#)init.c	4.31 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"

/*
 * init_player:
 *	Roll her up
 */
void
init_player(void)
{
    THING *obj;

    pstats = max_stats;
    food_left = HUNGERTIME;
    /*
     * Give him some food first so the portable pack-ordering code displays
     * the initial inventory in the DOS 1.48 order: food, mace, bow, arrows, armor.
     */
    obj = new_item();
    if (obj == NULL)
	fatal("out of memory\n");
    obj->o_type = FOOD;
    obj->o_count = 1;
    add_pack(obj, TRUE);
    /*
     * Give him his weaponry.  First a mace.
     */
    obj = new_item();
    if (obj == NULL)
	fatal("out of memory\n");
    init_weapon(obj, MACE);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    cur_weapon = obj;
    /*
     * Now a +1 bow
     */
    obj = new_item();
    if (obj == NULL)
	fatal("out of memory\n");
    init_weapon(obj, BOW);
    obj->o_hplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * Now some arrows
     */
    obj = new_item();
    if (obj == NULL)
	fatal("out of memory\n");
    init_weapon(obj, ARROW);
    obj->o_count = rnd(15) + 25;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * And his suit of armor
     */
    obj = new_item();
    if (obj == NULL)
	fatal("out of memory\n");
    obj->o_type = ARMOR;
    obj->o_which = RING_MAIL;
    obj->o_arm = a_class[RING_MAIL] - 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    cur_armor = obj;
    add_pack(obj, TRUE);
}

/*
 * Contains defintions and functions for dealing with things like
 * potions and scrolls
 */

const char *rainbow[] = {
    JSTR_STATIC("こはく色", "amber"),
    JSTR_STATIC("淡い青緑色", "aquamarine"),
    JSTR_STATIC("黒色", "black"),
    JSTR_STATIC("青色", "blue"),
    JSTR_STATIC("茶色", "brown"),
    JSTR_STATIC("透明", "clear"),
    JSTR_STATIC("深紅色", "crimson"),
    JSTR_STATIC("水色", "cyan"),
    JSTR_STATIC("ベージュ色", "ecru"),
    JSTR_STATIC("金色", "gold"),
    JSTR_STATIC("緑色", "green"),
    JSTR_STATIC("灰色", "grey"),
    JSTR_STATIC("赤紫色", "magenta"),
    JSTR_STATIC("オレンジ色", "orange"),
    JSTR_STATIC("ピンク色", "pink"),
    JSTR_STATIC("格子柄", "plaid"),
    JSTR_STATIC("紫色", "purple"),
    JSTR_STATIC("赤色", "red"),
    JSTR_STATIC("銀色", "silver"),
    JSTR_STATIC("淡い茶色", "tan"),
    JSTR_STATIC("みかん色", "tangerine"),
    JSTR_STATIC("淡褐色", "topaz"),
    JSTR_STATIC("青緑色", "turquoise"),
    JSTR_STATIC("朱色", "vermilion"),
    JSTR_STATIC("すみれ色", "violet"),
    JSTR_STATIC("白色", "white"),
    JSTR_STATIC("黄色", "yellow"),
};

#define NCOLORS ((int)(sizeof rainbow / sizeof rainbow[0]))

static const char ep_c_set[] = "bcdfghjklmnpqrstvwxyz";
static const char ep_v_set[] = "aeiou";


STONE stones[] = {
    { JSTR_STATIC("めのう", "agate"),		 25},
    { JSTR_STATIC("アレキサンドライト", "alexandrite"),	 40},
    { JSTR_STATIC("紫水晶", "amethyst"),	 50},
    { JSTR_STATIC("カーネリアン", "carnelian"),	 40},
    { JSTR_STATIC("ダイヤ", "diamond"),	300},
    { JSTR_STATIC("エメラルド", "emerald"),	300},
    { JSTR_STATIC("ゲルマニウム", "germanium"),	225},
    { JSTR_STATIC("御影石", "granite"),	  5},
    { JSTR_STATIC("ざくろ石", "garnet"),		 50},
    { JSTR_STATIC("ひすい", "jade"),		150},
    { JSTR_STATIC("クリプトナイト", "kryptonite"),	300},
    { JSTR_STATIC("るり", "lapis lazuli"),	 50},
    { JSTR_STATIC("月長石", "moonstone"),	 50},
    { JSTR_STATIC("黒曜石", "obsidian"),	 15},
    { JSTR_STATIC("オニキス", "onyx"),		 60},
    { JSTR_STATIC("オパール", "opal"),		200},
    { JSTR_STATIC("真珠", "pearl"),		220},
    { JSTR_STATIC("ペリドット", "peridot"),	 63},
    { JSTR_STATIC("ルビー", "ruby"),		350},
    { JSTR_STATIC("サファイア", "sapphire"),	285},
    { JSTR_STATIC("安タンタル石", "stibotantalite"),	200},
    { JSTR_STATIC("虎目石", "tiger eye"),	 50},
    { JSTR_STATIC("トパーズ", "topaz"),		 60},
    { JSTR_STATIC("トルコ石", "turquoise"),	 70},
    { JSTR_STATIC("ターフェアイト", "taaffeite"),	300},
    { JSTR_STATIC("ジルコン", "zircon"),	 	 80},
};

#define NSTONES ((int)(sizeof stones / sizeof stones[0]))

const char *wood[] = {
    JSTR_STATIC("アボカド", "avocado wood"),
    JSTR_STATIC("バルサ", "balsa"),
    JSTR_STATIC("竹", "bamboo"),
    JSTR_STATIC("バニヤン", "banyan"),
    JSTR_STATIC("白樺", "birch"),
    JSTR_STATIC("杉", "cedar"),
    JSTR_STATIC("桜", "cherry"),
    JSTR_STATIC("辰砂", "cinnibar"),
    JSTR_STATIC("糸杉", "cypress"),
    JSTR_STATIC("ハナミズキ", "dogwood"),
    JSTR_STATIC("流木", "driftwood"),
    JSTR_STATIC("黒檀", "ebony"),
    JSTR_STATIC("楡", "elm"),
    JSTR_STATIC("ユーカリ", "eucalyptus"),
    JSTR_STATIC("堕落", "fall"),
    JSTR_STATIC("ツガ", "hemlock"),
    JSTR_STATIC("ヒイラギ", "holly"),
    JSTR_STATIC("鉄樹", "ironwood"),
    JSTR_STATIC("ククイノキ", "kukui wood"),
    JSTR_STATIC("マホガニー", "mahogany"),
    JSTR_STATIC("マンザニータ", "manzanita"),
    JSTR_STATIC("楓", "maple"),
    JSTR_STATIC("樫", "oaken"),
    JSTR_STATIC("柿", "persimmon wood"),
    JSTR_STATIC("ピーカン", "pecan"),
    JSTR_STATIC("松", "pine"),
    JSTR_STATIC("ポプラ", "poplar"),
    JSTR_STATIC("アメリカ杉", "redwood"),
    JSTR_STATIC("紫檀", "rosewood"),
    JSTR_STATIC("トウヒ", "spruce"),
    JSTR_STATIC("チーク", "teak"),
    JSTR_STATIC("胡桃", "walnut"),
    JSTR_STATIC("ゼブラウッド", "zebrawood"),
};

#define NWOOD ((int)(sizeof wood / sizeof wood[0]))

const char *metal[] = {
    JSTR_STATIC("アルミニウム", "aluminum"),
    JSTR_STATIC("ベリリウム", "beryllium"),
    JSTR_STATIC("骨", "bone"),
    JSTR_STATIC("真ちゅう", "brass"),
    JSTR_STATIC("青銅", "bronze"),
    JSTR_STATIC("銅", "copper"),
    JSTR_STATIC("琥珀金", "electrum"),
    JSTR_STATIC("金", "gold"),
    JSTR_STATIC("鉄", "iron"),
    JSTR_STATIC("鉛", "lead"),
    JSTR_STATIC("マグネシウム", "magnesium"),
    JSTR_STATIC("水銀", "mercury"),
    JSTR_STATIC("ニッケル", "nickel"),
    JSTR_STATIC("ピューター", "pewter"),
    JSTR_STATIC("プラチナ", "platinum"),
    JSTR_STATIC("鋼鉄", "steel"),
    JSTR_STATIC("銀", "silver"),
    JSTR_STATIC("シリコン", "silicon"),
    JSTR_STATIC("すず", "tin"),
    JSTR_STATIC("チタン", "titanium"),
    JSTR_STATIC("タングステン", "tungsten"),
    JSTR_STATIC("亜鉛", "zinc"),
};

#define NMETAL ((int)(sizeof metal / sizeof metal[0]))

int cNWOOD = NWOOD;
int cNMETAL = NMETAL;
int cNSTONES = NSTONES;
int cNCOLORS = NCOLORS;

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors(void)
{
    int i, j;
    int used[NCOLORS];

    for (i = 0; i < NCOLORS; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXPOTIONS; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!used[j]);
	used[j] = TRUE;
	p_colors[i] = rainbow[j];
    }
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
#define MAXNAME	40	/* Max number of characters in a name */
#define MAX_JA_SCROLL_NAME_WIDTH 18	/* Max display columns for Japanese scroll names */

static void
free_scroll_name(int which)
{
    if (s_names[which] != NULL)
    {
	free(s_names[which]);
	s_names[which] = NULL;
    }
}

static int
dos_rchr_index(const char *string)
{
    return rnd((int) strlen(string));
}

static int
dos_getsyl_id(void)
{
    int nc = (int) strlen(ep_c_set);
    int c1 = dos_rchr_index(ep_c_set);
    int v  = dos_rchr_index(ep_v_set);
    int c2 = dos_rchr_index(ep_c_set);

    return ((c1 * (int) strlen(ep_v_set)) + v) * nc + c2;
}

static const char *
dos_ja_syl_head(char consonant, char vowel)
{
    int vi;
    static const char *b[] = {"バ", "ベ", "ビ", "ボ", "ブ"};
    static const char *k[] = {"カ", "ケ", "キ", "コ", "ク"};
    static const char *d[] = {"ダ", "デ", "ディ", "ド", "ドゥ"};
    static const char *f[] = {"ファ", "フェ", "フィ", "フォ", "フ"};
    static const char *g[] = {"ガ", "ゲ", "ギ", "ゴ", "グ"};
    static const char *h[] = {"ハ", "ヘ", "ヒ", "ホ", "フ"};
    static const char *j[] = {"ジャ", "ジェ", "ジ", "ジョ", "ジュ"};
    static const char *r[] = {"ラ", "レ", "リ", "ロ", "ル"};
    static const char *m[] = {"マ", "メ", "ミ", "モ", "ム"};
    static const char *n[] = {"ナ", "ネ", "ニ", "ノ", "ヌ"};
    static const char *p[] = {"パ", "ペ", "ピ", "ポ", "プ"};
    static const char *s[] = {"サ", "セ", "シ", "ソ", "ス"};
    static const char *t[] = {"タ", "テ", "ティ", "ト", "ツ"};
    static const char *v[] = {"ヴァ", "ヴェ", "ヴィ", "ヴォ", "ヴ"};
    static const char *w[] = {"ワ", "ウェ", "ウィ", "ウォ", "ウ"};
    static const char *y[] = {"ヤ", "イェ", "イ", "ヨ", "ユ"};
    static const char *z[] = {"ザ", "ゼ", "ジ", "ゾ", "ズ"};

    vi = strchr(ep_v_set, vowel) ? (int) (strchr(ep_v_set, vowel) - ep_v_set) : 0;

    switch (consonant)
    {
	case 'b': return b[vi];
	case 'c': case 'k': case 'q': return k[vi];
	case 'd': return d[vi];
	case 'f': return f[vi];
	case 'g': return g[vi];
	case 'h': return h[vi];
	case 'j': return j[vi];
	case 'l': case 'r': return r[vi];
	case 'm': return m[vi];
	case 'n': return n[vi];
	case 'p': return p[vi];
	case 's': return s[vi];
	case 't': return t[vi];
	case 'v': return v[vi];
	case 'w': return w[vi];
	case 'y': return y[vi];
	case 'x': case 'z': return z[vi];
    }

    return k[vi];
}

static const char *
dos_ja_syl_tail(char consonant)
{
    switch (consonant)
    {
	case 'b': return "ブ";
	case 'c': case 'k': case 'q': return "ク";
	case 'd': return "ド";
	case 'f': case 'h': return "フ";
	case 'g': return "グ";
	case 'j': return "ジ";
	case 'l': case 'r': return "ル";
	case 'm': return "ム";
	case 'n': return "ン";
	case 'p': return "プ";
	case 's': return "ス";
	case 't': return "ト";
	case 'v': return "ヴ";
	case 'w': case 'y': return "ウ";
	case 'x': case 'z': return "ズ";
    }

    return "ク";
}

static const char *
dos_syl_from_id_lang(int id, int japanese)
{
    int nc = (int) strlen(ep_c_set);
    int nv = (int) strlen(ep_v_set);
    int c1, v, c2;
    static char tsyl[32];

    if (id < 0 || id >= nc * nv * nc)
	id = 0;

    c2 = id % nc;
    id /= nc;
    v = id % nv;
    c1 = id / nv;

    if (japanese)
    {
	snprintf(tsyl, sizeof tsyl, "%s%s",
	    dos_ja_syl_head(ep_c_set[c1], ep_v_set[v]),
	    dos_ja_syl_tail(ep_c_set[c2]));
    }
    else
    {
	tsyl[0] = ep_c_set[c1];
	tsyl[1] = ep_v_set[v];
	tsyl[2] = ep_c_set[c2];
	tsyl[3] = '\0';
    }

    return tsyl;
}

static const char *
dos_syl_from_id(int id)
{
    return dos_syl_from_id_lang(id, jrogue_lang_is_japanese);
}

static int
dos_syl_display_width(int id, int japanese)
{
    const char *s = dos_syl_from_id_lang(id, japanese);

    return japanese ? mbstrlen(s) : (int) strlen(s);
}

static int
dos_append_scroll_name(char **cpp, size_t limit, const char *text)
{
    size_t len = strlen(text);
    size_t used = (size_t) (*cpp - prbuf);

    if (used + len > limit - 1)
	return FALSE;

    memcpy(*cpp, text, len);
    *cpp += len;
    **cpp = '\0';
    return TRUE;
}

static void
dos_build_scroll_name(int which)
{
    int n, id, width, used_width, max_width;
    char *cp = prbuf;
    size_t limit = jrogue_lang_is_japanese ? 160 : MAXNAME;

    used_width = 0;
    max_width = jrogue_lang_is_japanese ? MAX_JA_SCROLL_NAME_WIDTH : MAXNAME - 1;
    prbuf[0] = '\0';
    for (n = 0; n < scroll_syll_count[which]; n++)
    {
	id = scroll_syll_ids[which][n];
	width = dos_syl_display_width(id, jrogue_lang_is_japanese);
	if (used_width > 0 && used_width + width > max_width)
	    break;
	if (!dos_append_scroll_name(&cp, limit, dos_syl_from_id(id)))
	    break;
	used_width += width;
	if (scroll_syll_breaks[which][n])
	{
	    if (used_width + 1 > max_width)
		break;
	    if (!dos_append_scroll_name(&cp, limit, " "))
		break;
	    used_width++;
	}
    }

    if (cp > prbuf && cp[-1] == ' ')
	*--cp = '\0';
    else
	*cp = '\0';

    free_scroll_name(which);
    s_names[which] = malloc(strlen(prbuf) + 1);
    if (s_names[which] != NULL)
	strcpy(s_names[which], prbuf);
}

void
jrogue_rebuild_scroll_names(void)
{
    int i;

    for (i = 0; i < MAXSCROLLS; i++)
	if (scroll_syll_count[i] > 0)
	    dos_build_scroll_name(i);
}

void
init_names(void)
{
    int nsyl;
    int id;
    int i, nwords, n, count, last_in_word, roman_used, ja_used, ja_width;

    for (i = 0; i < MAXSCROLLS; i++)
    {
	scroll_syll_count[i] = 0;
	for (n = 0; n < MAX_SCROLL_SYLLABLES; n++)
	{
	    scroll_syll_ids[i][n] = 0;
	    scroll_syll_breaks[i][n] = FALSE;
	}

	count = 0;
	roman_used = 0;
	ja_used = 0;
	/*
	 * DOS 1.48 shortened unidentified scroll titles on 40-column
	 * screens.  JEC uses an 80-column game screen, so terse must not
	 * affect the generated name or its random-number sequence.
	 */
	nwords = rnd(4) + 2;
	while (nwords-- && count < MAX_SCROLL_SYLLABLES)
	{
	    nsyl = rnd(2) + 1;
	    last_in_word = -1;
	    while (nsyl-- && count < MAX_SCROLL_SYLLABLES)
	    {
		id = dos_getsyl_id();
		ja_width = dos_syl_display_width(id, TRUE);
		if (roman_used + 3 > MAXNAME - 1
		    || ja_used + ja_width > MAX_JA_SCROLL_NAME_WIDTH)
		{
		    nwords = 0;
		    break;
		}
		scroll_syll_ids[i][count] = id;
		scroll_syll_breaks[i][count] = FALSE;
		roman_used += 3;
		ja_used += ja_width;
		last_in_word = count;
		count++;
	    }
	    if (last_in_word >= 0)
	    {
		scroll_syll_breaks[i][last_in_word] = TRUE;
		if (roman_used < MAXNAME)
		    roman_used++;
		if (ja_used < MAX_JA_SCROLL_NAME_WIDTH)
		    ja_used++;
	    }
	}
	scroll_syll_count[i] = count;
	dos_build_scroll_name(i);
    }
}


/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones(void)
{
    int used[NSTONES];
    int i, j;

    for (i = 0; i < NSTONES; i++)
	used[i] = FALSE;
    for (i = 0; i < MAXRINGS; i++)
    {
	do
	    j = rnd(NSTONES);
	until (!used[j]);
	used[j] = TRUE;
	r_stones[i] = stones[j].st_name;
	ring_info[i].oi_worth += stones[j].st_value;
    }
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials(void)
{
    int i, j;
    const char *str;
    int metused[NMETAL];
    int used[NWOOD];

    for (i = 0; i < NWOOD; i++)
	used[i] = FALSE;
    for (i = 0; i < NMETAL; i++)
	metused[i] = FALSE;
    for (i = 0; i < MAXSTICKS; i++)
    {
	for (;;)
	    if (rnd(2) == 0)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    ws_type[i] = JSTR("魔法の棒", "wand");
		    str = metal[j];
		    metused[j] = TRUE;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!used[j])
		{
		    ws_type[i] = JSTR("つえ", "staff");
		    str = wood[j];
		    used[j] = TRUE;
		    break;
		}
	    }
	ws_made[i] = str;
    }
}

#ifdef MASTER
# define	NT	NUMTHINGS, "things"
# define	MP	MAXPOTIONS, "potions"
# define	MS	MAXSCROLLS, "scrolls"
# define	MR	MAXRINGS, "rings"
# define	MWS	MAXSTICKS, "sticks"
# define	MW	MAXWEAPONS, "weapons"
# define	MA	MAXARMORS, "armor"
#else
# define	NT	NUMTHINGS
# define	MP	MAXPOTIONS
# define	MS	MAXSCROLLS
# define	MR	MAXRINGS
# define	MWS	MAXSTICKS
# define	MW	MAXWEAPONS
# define	MA	MAXARMORS
#endif

/*
 * sumprobs:
 *	Sum up the probabilities for items appearing
 */
void
sumprobs(struct obj_info *info, int bound
#ifdef MASTER
	, char *name
#endif
)
{
#ifdef MASTER
	struct obj_info *start = info;
#endif
    struct obj_info *endp;

    endp = info + bound;
    while (++info < endp)
	info->oi_prob += (info - 1)->oi_prob;
#ifdef MASTER
    badcheck(name, start, bound);
#endif
}

/*
 * init_probs:
 *	Initialize the probabilities for the various items
 */
void
init_probs(void)
{
    sumprobs(things, NT);
    sumprobs(pot_info, MP);
    sumprobs(scr_info, MS);
    sumprobs(ring_info, MR);
    sumprobs(ws_info, MWS);
    sumprobs(weap_info, MW);
    sumprobs(arm_info, MA);
}

#ifdef MASTER
/*
 * badcheck:
 *	Check to see if a series of probabilities sums to 100
 */
void
badcheck(const char *name, const struct obj_info *info, int bound)
{
    const struct obj_info *end;

    if (info[bound - 1].oi_prob == 100)
	return;
    printf("\nBad percentages for %s (bound = %d):\n", name, bound);
    for (end = &info[bound]; info < end; info++)
	printf("%3d%% %s\n", info->oi_prob, info->oi_name);
    printf("[hit RETURN to continue]");
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}
#endif

/*
 * pick_color:
 *	If he is halucinating, pick a random color name and return it,
 *	otherwise return the given color.
 */
const char *
pick_color(const char *col)
{
    return col;
}


static void
jrogue_capture_color_indexes(int pot_index[], int ring_index[],
    int stick_is_staff[], int stick_index[])
{
    int i, j;

    for (i = 0; i < MAXPOTIONS; i++)
    {
	pot_index[i] = -1;
	if (p_colors[i] == NULL)
	    continue;
	for (j = 0; j < cNCOLORS; j++)
	    if (p_colors[i] == rainbow[j] || strcmp(p_colors[i], rainbow[j]) == 0)
	    {
		pot_index[i] = j;
		break;
	    }
    }

    for (i = 0; i < MAXRINGS; i++)
    {
	ring_index[i] = -1;
	if (r_stones[i] == NULL)
	    continue;
	for (j = 0; j < cNSTONES; j++)
	    if (r_stones[i] == stones[j].st_name
		|| strcmp(r_stones[i], stones[j].st_name) == 0)
	    {
		ring_index[i] = j;
		break;
	    }
    }

    for (i = 0; i < MAXSTICKS; i++)
    {
	stick_is_staff[i] = FALSE;
	stick_index[i] = -1;
	if (ws_made[i] == NULL)
	    continue;

	for (j = 0; j < cNWOOD; j++)
	    if (ws_made[i] == wood[j] || strcmp(ws_made[i], wood[j]) == 0)
	    {
		stick_is_staff[i] = TRUE;
		stick_index[i] = j;
		break;
	    }

	if (stick_index[i] >= 0)
	    continue;

	for (j = 0; j < cNMETAL; j++)
	    if (ws_made[i] == metal[j] || strcmp(ws_made[i], metal[j]) == 0)
	    {
		stick_is_staff[i] = FALSE;
		stick_index[i] = j;
		break;
	    }
    }
}

static void
jrogue_reapply_color_indexes(int pot_index[], int ring_index[],
    int stick_is_staff[], int stick_index[])
{
    int i;

    for (i = 0; i < MAXPOTIONS; i++)
	if (pot_index[i] >= 0 && pot_index[i] < cNCOLORS)
	    p_colors[i] = rainbow[pot_index[i]];

    for (i = 0; i < MAXRINGS; i++)
	if (ring_index[i] >= 0 && ring_index[i] < cNSTONES)
	    r_stones[i] = stones[ring_index[i]].st_name;

    for (i = 0; i < MAXSTICKS; i++)
    {
	if (stick_index[i] < 0)
	    continue;

	if (stick_is_staff[i])
	{
	    if (stick_index[i] < cNWOOD)
	    {
		ws_type[i] = JSTR("つえ", "staff");
		ws_made[i] = wood[stick_index[i]];
	    }
	}
	else
	{
	    if (stick_index[i] < cNMETAL)
	    {
		ws_type[i] = JSTR("魔法の棒", "wand");
		ws_made[i] = metal[stick_index[i]];
	    }
	}
    }
}


void
jrogue_apply_init_language(void)
{
    int pot_index[MAXPOTIONS], ring_index[MAXRINGS];
    int stick_is_staff[MAXSTICKS], stick_index[MAXSTICKS];

    jrogue_capture_color_indexes(pot_index, ring_index, stick_is_staff, stick_index);

    rainbow[0] = JSTR("こはく色", "amber");
    rainbow[1] = JSTR("淡い青緑色", "aquamarine");
    rainbow[2] = JSTR("黒色", "black");
    rainbow[3] = JSTR("青色", "blue");
    rainbow[4] = JSTR("茶色", "brown");
    rainbow[5] = JSTR("透明", "clear");
    rainbow[6] = JSTR("深紅色", "crimson");
    rainbow[7] = JSTR("水色", "cyan");
    rainbow[8] = JSTR("ベージュ色", "ecru");
    rainbow[9] = JSTR("金色", "gold");
    rainbow[10] = JSTR("緑色", "green");
    rainbow[11] = JSTR("灰色", "grey");
    rainbow[12] = JSTR("赤紫色", "magenta");
    rainbow[13] = JSTR("オレンジ色", "orange");
    rainbow[14] = JSTR("ピンク色", "pink");
    rainbow[15] = JSTR("格子柄", "plaid");
    rainbow[16] = JSTR("紫色", "purple");
    rainbow[17] = JSTR("赤色", "red");
    rainbow[18] = JSTR("銀色", "silver");
    rainbow[19] = JSTR("淡い茶色", "tan");
    rainbow[20] = JSTR("みかん色", "tangerine");
    rainbow[21] = JSTR("淡褐色", "topaz");
    rainbow[22] = JSTR("青緑色", "turquoise");
    rainbow[23] = JSTR("朱色", "vermilion");
    rainbow[24] = JSTR("すみれ色", "violet");
    rainbow[25] = JSTR("白色", "white");
    rainbow[26] = JSTR("黄色", "yellow");
    wood[0] = JSTR("アボカド", "avocado wood");
    wood[1] = JSTR("バルサ", "balsa");
    wood[2] = JSTR("竹", "bamboo");
    wood[3] = JSTR("バニヤン", "banyan");
    wood[4] = JSTR("白樺", "birch");
    wood[5] = JSTR("杉", "cedar");
    wood[6] = JSTR("桜", "cherry");
    wood[7] = JSTR("辰砂", "cinnibar");
    wood[8] = JSTR("糸杉", "cypress");
    wood[9] = JSTR("ハナミズキ", "dogwood");
    wood[10] = JSTR("流木", "driftwood");
    wood[11] = JSTR("黒檀", "ebony");
    wood[12] = JSTR("楡", "elm");
    wood[13] = JSTR("ユーカリ", "eucalyptus");
    wood[14] = JSTR("堕落", "fall");
    wood[15] = JSTR("ツガ", "hemlock");
    wood[16] = JSTR("ヒイラギ", "holly");
    wood[17] = JSTR("鉄樹", "ironwood");
    wood[18] = JSTR("ククイノキ", "kukui wood");
    wood[19] = JSTR("マホガニー", "mahogany");
    wood[20] = JSTR("マンザニータ", "manzanita");
    wood[21] = JSTR("楓", "maple");
    wood[22] = JSTR("樫", "oaken");
    wood[23] = JSTR("柿", "persimmon wood");
    wood[24] = JSTR("ピーカン", "pecan");
    wood[25] = JSTR("松", "pine");
    wood[26] = JSTR("ポプラ", "poplar");
    wood[27] = JSTR("アメリカ杉", "redwood");
    wood[28] = JSTR("紫檀", "rosewood");
    wood[29] = JSTR("トウヒ", "spruce");
    wood[30] = JSTR("チーク", "teak");
    wood[31] = JSTR("胡桃", "walnut");
    wood[32] = JSTR("ゼブラウッド", "zebrawood");
    metal[0] = JSTR("アルミニウム", "aluminum");
    metal[1] = JSTR("ベリリウム", "beryllium");
    metal[2] = JSTR("骨", "bone");
    metal[3] = JSTR("真ちゅう", "brass");
    metal[4] = JSTR("青銅", "bronze");
    metal[5] = JSTR("銅", "copper");
    metal[6] = JSTR("琥珀金", "electrum");
    metal[7] = JSTR("金", "gold");
    metal[8] = JSTR("鉄", "iron");
    metal[9] = JSTR("鉛", "lead");
    metal[10] = JSTR("マグネシウム", "magnesium");
    metal[11] = JSTR("水銀", "mercury");
    metal[12] = JSTR("ニッケル", "nickel");
    metal[13] = JSTR("ピューター", "pewter");
    metal[14] = JSTR("プラチナ", "platinum");
    metal[15] = JSTR("鋼鉄", "steel");
    metal[16] = JSTR("銀", "silver");
    metal[17] = JSTR("シリコン", "silicon");
    metal[18] = JSTR("すず", "tin");
    metal[19] = JSTR("チタン", "titanium");
    metal[20] = JSTR("タングステン", "tungsten");
    metal[21] = JSTR("亜鉛", "zinc");
    stones[0].st_name = JSTR("めのう", "agate");
    stones[1].st_name = JSTR("アレキサンドライト", "alexandrite");
    stones[2].st_name = JSTR("紫水晶", "amethyst");
    stones[3].st_name = JSTR("カーネリアン", "carnelian");
    stones[4].st_name = JSTR("ダイヤ", "diamond");
    stones[5].st_name = JSTR("エメラルド", "emerald");
    stones[6].st_name = JSTR("ゲルマニウム", "germanium");
    stones[7].st_name = JSTR("御影石", "granite");
    stones[8].st_name = JSTR("ざくろ石", "garnet");
    stones[9].st_name = JSTR("ひすい", "jade");
    stones[10].st_name = JSTR("クリプトナイト", "kryptonite");
    stones[11].st_name = JSTR("るり", "lapis lazuli");
    stones[12].st_name = JSTR("月長石", "moonstone");
    stones[13].st_name = JSTR("黒曜石", "obsidian");
    stones[14].st_name = JSTR("オニキス", "onyx");
    stones[15].st_name = JSTR("オパール", "opal");
    stones[16].st_name = JSTR("真珠", "pearl");
    stones[17].st_name = JSTR("ペリドット", "peridot");
    stones[18].st_name = JSTR("ルビー", "ruby");
    stones[19].st_name = JSTR("サファイア", "sapphire");
    stones[20].st_name = JSTR("安タンタル石", "stibotantalite");
    stones[21].st_name = JSTR("虎目石", "tiger eye");
    stones[22].st_name = JSTR("トパーズ", "topaz");
    stones[23].st_name = JSTR("トルコ石", "turquoise");
    stones[24].st_name = JSTR("ターフェアイト", "taaffeite");
    stones[25].st_name = JSTR("ジルコン", "zircon");

    jrogue_reapply_color_indexes(pot_index, ring_index, stick_is_staff, stick_index);
}
