/*
 * Score file structure
 *
 * @(#)score.h	4.6 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * Score entries are language-independent.  sc_death_id stores the reason
 * category below; sc_killer_id stores the monster letter or one of the
 * special death-source IDs used by killname().  sc_rank stores the player's
 * experience rank at the end of the game.  sc_name is a user-entered UTF-8
 * string and is preserved verbatim.
 */
#define SC_DEATH_KILLED       0
#define SC_DEATH_QUIT         1
#define SC_DEATH_WINNER       2

#define sc_death_id sc_flags
#define sc_killer_id sc_monster
#define sc_rank sc_time

struct sc_ent {
    jrogue_uid_t sc_uid;
    int sc_score;
    unsigned int sc_flags;
    int sc_monster;
    char sc_name[MAXSTR];
    int sc_level;
    unsigned int sc_time;
};

typedef struct sc_ent SCORE;

void	rd_score(SCORE *top_ten);
void	wr_score(SCORE *top_ten);
