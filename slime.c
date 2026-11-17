/*
 * Code for handling the special properties of the slime.
 * Adapted for the DOS-style macOS/Unix source from the DOS 1.48 logic.
 */

#include <curses.h>
#include "rogue.h"

static coord slimy;

int
plop_monster(int r, int c, coord *cp)
{
    int y, x;
    int appear = 0;
    int ch;

    for (y = r - 1; y <= r + 1; y++)
    {
        for (x = c - 1; x <= c + 1; x++)
        {
            if ((y == hero.y && x == hero.x) || x < 0 || x >= NUMCOLS
                || y <= 0 || y >= NUMLINES - 1)
                continue;
            ch = winat(y, x);
            if (step_ok(ch))
            {
                if (ch == SCROLL)
                {
                    THING *obj = find_obj(y, x);

                    if (obj != NULL && obj->o_which == S_SCARE)
                        continue;
                }
                if (rnd(++appear) == 0)
                {
                    cp->y = y;
                    cp->x = x;
                }
            }
        }
    }
    return appear;
}

static int
new_slime(THING *tp)
{
    int y, x, ty, tx;
    THING *ntp;
    coord sp;
    int ret = FALSE;

    tp->t_flags |= ISFLY;
    ty = tp->t_pos.y;
    tx = tp->t_pos.x;
    if (plop_monster(ty, tx, &sp) == 0)
    {
        for (y = ty - 1; y <= ty + 1; y++)
        {
            for (x = tx - 1; x <= tx + 1; x++)
            {
                if (x < 0 || x >= NUMCOLS || y <= 0 || y >= NUMLINES - 1)
                    continue;
                if (winat(y, x) == 'S' && (ntp = moat(y, x)) != NULL)
                {
                    if (ntp->t_flags & ISFLY)
                        continue;
                    if (new_slime(ntp))
                    {
                        y = ty + 2;
                        x = tx + 2;
                    }
                }
            }
        }
    }
    else
    {
        ret = TRUE;
        slimy = sp;
    }
    tp->t_flags &= ~ISFLY;
    return ret;
}

void
slime_split(THING *tp)
{
    THING *nslime;

    if (!new_slime(tp) || (nslime = new_item()) == NULL)
        return;
    msg(JSTR("スライムは分裂した。うへえ！", "The slime divides.  Ick!"));
    new_monster(nslime, 'S', &slimy);
    if (cansee(slimy.y, slimy.x))
    {
        nslime->t_oldch = chat(slimy.y, slimy.x);
        jrogue_mvaddch_map(slimy.y, slimy.x, 'S');
    }
    runto(&slimy);
}
