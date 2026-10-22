/* ===== MUSEE ALIEN - FPS d'horreur 3DS (raycasting logiciel, sans WiFi) ===== */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SW 400
#define SH 240
#define RAYS 200
#define MAPW 24
#define MAPH 24
#define MAXENT 24
#define NB_CORES 3
#define PI_F 3.14159265f

/* '#' mur, '2' vitrine, '.' sol */
static const char *worldMap[MAPH] = {
  "########################",
  "#.......#.......#......#",
  "#.......#.......#......#",
  "#..2.2..#..2.2..#..2.2.#",
  "#.......#.......#......#",
  "#.......#.......#......#",
  "#..2.2..#..2.2..#..2.2.#",
  "#.......#.......#......#",
  "#.......#.......#......#",
  "###.#####.#######.######",
  "#......................#",
  "#.2..........2.......2.#",
  "#......................#",
  "######.##########.######",
  "#......................#",
  "#..2..............2....#",
  "#......................#",
  "#......................#",
  "#...2..............2...#",
  "#......................#",
  "#......................#",
  "#......................#",
  "#......................#",
  "########################"
};

/* Sprites 16x16 */
static const char *alienBmp[16] = {
  "................",
  ".....gggggg.....",
  "....gggggggg....",
  "...gggggggggg...",
  "..gggggggggggg..",
  "..geeeggggeeeg..",
  "..geeeggggeeeg..",
  "..gggggggggggg..",
  "...gggggggggg...",
  ".....gggggg.....",
  "...gggggggggg...",
  "..gggggggggggg..",
  "..gg.gggggg.gg..",
  "..gg..gggg..gg..",
  ".....gg..gg.....",
  ".....gg..gg....."
};

static const char *coreBmp[16] = {
  "................",
  "..cccc....cccc..",
  ".cccccc..cccccc.",
  ".ccwwcccccccccc.",
  ".cwcccccccccccc.",
  ".cccccccccccccc.",
  "..cccccccccccc..",
  "...cccccccccc...",
  "....cccccccc....",
  ".....cccccc.....",
  "......cccc......",
  ".......cc.......",
  "................",
  "................",
  "................",
  "................"
};

static const char *persBmp[16] = {
  "................",
  "......hhhh......",
  ".....hhhhhh.....",
  ".....ssssss.....",
  ".....ssssss.....",
  ".....ssssss.....",
  "......ssss......",
  "....bbbbbbbb....",
  "...bbbbrrbbbb...",
  "..bbbbbrrbbbbb..",
  "..bbbbbrrbbbbb..",
  "..bbbbbbbbbbbb..",
  "..bbbbbbbbbbbb..",
  "....kkkkkkkk....",
  "....kkk..kkk....",
  "....kkk..kkk...."
};

#define NSPOOK 9
static const char *spook[NSPOOK] = {
  "Tu n'es pas cense etre ici.",
  "Les statues... elles ne bougeaient pas avant ?",
  "Quelque chose respire derriere toi.",
  "Les lumieres clignotent. Ne reste pas la.",
  "Ce musee est ferme depuis des annees.",
  "Tu entends des pas qui ne sont pas les tiens.",
  "Quelqu'un te regarde depuis le noir.",
  "Les tableaux ont des yeux.",
  "Ils savent que tu es entre."
};

enum { T_ALIEN, T_CORE, T_PRES, T_STATUE };
enum { S_TITLE, S_PLAY, S_WIN, S_DEAD };

typedef struct {
  int type, alive, hp;
  float x, y, flash, cd, wx, wy, wtimer;
} Ent;

static Ent ents[MAXENT];
static int nEnt;

static float posX, posY, ang, pitch;
static float dirX, dirY, planeX, planeY, planeLen = 0.66f, HS;
static int health, cores, delivered, state, horizon;
static float lightLevel = 1.0f, hurt = 0, shotCd = 0, muzzle = 0;
static float flickT = 5, flickLeft = 0, msgT = 0, spookT = 12, gameTime = 0;
static const char *msg = "";
static int dirty = 1;
static u8 *fb;
static float zbuf[RAYS];
static u8 bgcol[SH * 3];
static int vrow[SH];

/* ---------- utilitaires ---------- */
static int isFree(float x, float y) {
  int mx, my;
  if (x < 0 || y < 0) return 0;
  mx = (int)x; my = (int)y;
  if (mx >= MAPW || my >= MAPH) return 0;
  return worldMap[my][mx] == '.';
}

static int canStand(float x, float y, float r) {
  return isFree(x - r, y - r) && isFree(x + r, y - r) &&
         isFree(x - r, y + r) && isFree(x + r, y + r);
}

static int hasLOS(float x0, float y0, float x1, float y1) {
  float dx = x1 - x0, dy = y1 - y0;
  float d = sqrtf(dx * dx + dy * dy);
  int n = (int)(d / 0.25f) + 1, i;
  for (i = 1; i < n; i++) {
    float t = (float)i / (float)n;
    if (!isFree(x0 + dx * t, y0 + dy * t)) return 0;
  }
  return 1;
}

static float castRay(float ox, float oy, float rdx, float rdy, int *hx, int *hy, int *side) {
  int mx = (int)ox, my = (int)oy, sx, sy, sd = 0, i;
  float ddx = (rdx == 0.0f) ? 1e30f : fabsf(1.0f / rdx);
  float ddy = (rdy == 0.0f) ? 1e30f : fabsf(1.0f / rdy);
  float sdx, sdy;
  if (rdx < 0) { sx = -1; sdx = (ox - mx) * ddx; } else { sx = 1; sdx = (mx + 1.0f - ox) * ddx; }
  if (rdy < 0) { sy = -1; sdy = (oy - my) * ddy; } else { sy = 1; sdy = (my + 1.0f - oy) * ddy; }
  for (i = 0; i < 64; i++) {
    if (sdx < sdy) { sdx += ddx; mx += sx; sd = 0; }
    else           { sdy += ddy; my += sy; sd = 1; }
    if (mx < 0 || my < 0 || mx >= MAPW || my >= MAPH) break;
    if (worldMap[my][mx] != '.') {
      *hx = mx; *hy = my; *side = sd;
      return sd == 0 ? (sdx - ddx) : (sdy - ddy);
    }
  }
  *hx = 0; *hy = 0; *side = sd;
  return 40.0f;
}

static int shadeFor(float d) {
  float s = lightLevel * 256.0f / (1.0f + d * d * 0.09f);
  int v = (int)s;
  if (v < 4) v = 4;
  if (v > 256) v = 256;
  return v;
}

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static void say(const char *m) { msg = m; msgT = 7.0f; dirty = 1; }

static void addEnt(int type, float x, float y, int hp) {
  Ent *e;
  if (nEnt >= MAXENT) return;
  e = &ents[nEnt++];
  memset(e, 0, sizeof(*e));
  e->type = type; e->alive = 1; e->x = x; e->y = y; e->hp = hp;
}

static void resetGame(void) {
  nEnt = 0;
  posX = 11.5f; posY = 18.5f; ang = PI_F / 2.0f; pitch = 0;
  health = 100; cores = 0; delivered = 0;
  hurt = 0; shotCd = 0; muzzle = 0; msgT = 0; msg = ""; spookT = 14;
  addEnt(T_PRES, 11.5f, 21.5f, 0);
  /* coeurs de vaisseaux */
  addEnt(T_CORE, 4.5f, 4.5f, 0);
  addEnt(T_CORE, 12.5f, 4.5f, 0);
  addEnt(T_CORE, 19.5f, 4.5f, 0);
  /* aliens */
  addEnt(T_ALIEN, 2.5f, 7.5f, 4);
  addEnt(T_ALIEN, 6.5f, 2.5f, 4);
  addEnt(T_ALIEN, 10.5f, 7.5f, 4);
  addEnt(T_ALIEN, 14.5f, 2.5f, 4);
  addEnt(T_ALIEN, 20.5f, 7.5f, 4);
  addEnt(T_ALIEN, 18.5f, 2.5f, 4);
  addEnt(T_ALIEN, 14.5f, 11.5f, 4);
  addEnt(T_ALIEN, 2.5f, 16.5f, 4);
  addEnt(T_ALIEN, 21.5f, 20.5f, 4);
  /* statues */
  addEnt(T_STATUE, 5.5f, 10.5f, 0);
  addEnt(T_STATUE, 20.5f, 12.5f, 0);
  addEnt(T_STATUE, 7.5f, 19.5f, 0);
  dirty = 1;
}

/* ---------- dessin ---------- */
static void rect(int x, int y, int w, int h, int r, int g, int b) {
  int i, j;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SW) w = SW - x;
  if (y + h > SH) h = SH - y;
  if (w <= 0 || h <= 0) return;
  for (i = x; i < x + w; i++) {
    u8 *q = fb + (i * SH + (SH - 1 - y)) * 3;
    for (j = 0; j < h; j++, q -= 3) { q[0] = (u8)b; q[1] = (u8)g; q[2] = (u8)r; }
  }
}

static int pal(int mode, char c, int *r, int *g, int *b, float flash) {
  switch (mode) {
    case T_ALIEN:
      if (c == 'g') { *r = 70; *g = 190; *b = 90; }
      else if (c == 'e') { *r = 4; *g = 4; *b = 4; }
      else return 0;
      break;
    case T_CORE:
      if (c == 'c') { *r = 60; *g = 255; *b = 210; }
      else if (c == 'w') { *r = 255; *g = 255; *b = 255; }
      else return 0;
      break;
    default:
      switch (c) {
        case 'h': *r = 70;  *g = 45;  *b = 30;  break;
        case 's': *r = 225; *g = 185; *b = 155; break;
        case 'b': *r = 35;  *g = 55;  *b = 130; break;
        case 'r': *r = 190; *g = 30;  *b = 30;  break;
        case 'k': *r = 25;  *g = 25;  *b = 35;  break;
        default: return 0;
      }
      if (mode == T_STATUE) { int v = (*r + *g + *b) / 3 + 30; *r = v; *g = v; *b = v; }
  }
  if (flash > 0) { *r = 255; *g = (*g + 255) / 2; *b = (*b + 255) / 2; }
  return 1;
}

static void drawSprite(const char **bmp, int mode, float wx, float wy,
                       float hUnits, float zOff, float flash, int glow) {
  float sx = wx - posX, sy = wy - posY;
  float det = planeX * dirY - dirX * planeY, inv, tx, ty, unit;
  int h, scr, wr, floorY, topY, r0, ya, yb, y, c, sh;
  if (fabsf(det) < 1e-6f) return;
  inv = 1.0f / det;
  tx = inv * (dirY * sx - dirX * sy);
  ty = inv * (-planeY * sx + planeX * sy);
  if (ty < 0.2f) return;
  unit = HS / ty;
  h = (int)(unit * hUnits);
  if (h < 2) return;
  scr = (int)((RAYS * 0.5f) * (1.0f + tx / ty));
  wr = h / 2; if (wr < 1) wr = 1;
  floorY = horizon + (int)(unit * 0.5f) - (int)(unit * zOff);
  topY = floorY - h;
  r0 = scr - wr / 2;
  ya = topY < 0 ? 0 : topY;
  yb = floorY > SH ? SH : floorY;
  for (y = ya; y < yb; y++) {
    int v = (y - topY) * 16 / h;
    vrow[y] = v > 15 ? 15 : v;
  }
  sh = shadeFor(ty);
  if (glow) { sh += 110; if (sh > 256) sh = 256; }
  for (c = r0; c < r0 + wr; c++) {
    int u;
    u8 *p0;
    if (c < 0 || c >= RAYS) continue;
    if (ty >= zbuf[c]) continue;
    u = (c - r0) * 16 / wr; if (u > 15) u = 15;
    p0 = fb + (c * 2) * SH * 3;
    for (y = ya; y < yb; y++) {
      int r, g, b;
      u8 *q;
      char ch = bmp[vrow[y]][u];
      if (!pal(mode, ch, &r, &g, &b, flash)) continue;
      r = (r * sh) >> 8; g = (g * sh) >> 8; b = (b * sh) >> 8;
      q = p0 + (SH - 1 - y) * 3;
      q[0] = (u8)b; q[1] = (u8)g; q[2] = (u8)r;
      q[SH * 3 + 0] = (u8)b; q[SH * 3 + 1] = (u8)g; q[SH * 3 + 2] = (u8)r;
    }
  }
}

static void drawSprites(void) {
  int idx[MAXENT], n = 0, i, j, k;
  float dist[MAXENT];
  for (i = 0; i < nEnt; i++) {
    float dx, dy;
    if (!ents[i].alive) continue;
    dx = ents[i].x - posX; dy = ents[i].y - posY;
    dist[n] = dx * dx + dy * dy; idx[n++] = i;
  }
  for (i = 1; i < n; i++) {
    int ki = idx[i]; float kd = dist[i];
    j = i - 1;
    while (j >= 0 && dist[j] < kd) { dist[j + 1] = dist[j]; idx[j + 1] = idx[j]; j--; }
    dist[j + 1] = kd; idx[j + 1] = ki;
  }
  for (k = 0; k < n; k++) {
    Ent *e = &ents[idx[k]];
    switch (e->type) {
      case T_ALIEN:  drawSprite(alienBmp, T_ALIEN, e->x, e->y, 0.9f, 0.0f, e->flash, 0); break;
      case T_CORE:   drawSprite(coreBmp, T_CORE, e->x, e->y, 0.5f + 0.04f * sinf(gameTime * 4.0f),
                                0.22f + 0.06f * sinf(gameTime * 2.5f), 0, 1); break;
      case T_PRES:   drawSprite(persBmp, T_PRES, e->x, e->y, 1.0f, 0.0f, 0, 1); break;
      case T_STATUE: drawSprite(persBmp, T_STATUE, e->x, e->y, 1.0f, 0.0f, 0, 0); break;
    }
  }
}

static void render(int aiming) {
  int i, y;
  fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
  horizon = SH / 2 + (int)pitch;

  /* fond : plafond + sol avec degrade sombre */
  for (y = 0; y < SH; y++) {
    int dy = abs(y - horizon), r, g, b;
    float f = (float)dy / (SH * 0.5f), br;
    if (f > 1.0f) f = 1.0f;
    br = (0.04f + 0.55f * f) * lightLevel;
    if (y < horizon) { r = (int)(90 * br); g = (int)(105 * br); b = (int)(115 * br); }
    else             { r = (int)(120 * br); g = (int)(100 * br); b = (int)(75 * br); }
    r += (int)(hurt * 0.5f);
    bgcol[(SH - 1 - y) * 3 + 0] = (u8)clamp255(b);
    bgcol[(SH - 1 - y) * 3 + 1] = (u8)clamp255(g);
    bgcol[(SH - 1 - y) * 3 + 2] = (u8)clamp255(r);
  }

  /* murs */
  for (i = 0; i < RAYS; i++) {
    float cx = 2.0f * (float)i / (float)RAYS - 1.0f;
    float rdx = dirX + planeX * cx, rdy = dirY + planeY * cx, perp;
    int hx, hy, sd, lineH, ys, ye, y0, y1, sh, cr, cgc, cb, paint = 0, eyes = 0;
    int pY0 = 0, pY1 = 0, ph, type;
    float wallX = 0;
    u8 *p0 = fb + (i * 2) * SH * 3, *q;

    perp = castRay(posX, posY, rdx, rdy, &hx, &hy, &sd);
    if (perp < 0.05f) perp = 0.05f;
    zbuf[i] = perp;
    lineH = (int)(HS / perp);
    ys = horizon - lineH / 2; ye = horizon + lineH / 2;
    memcpy(p0, bgcol, SH * 3);

    type = worldMap[hy][hx];
    sh = shadeFor(perp);
    if (sd) sh = (sh * 190) >> 8;
    if (type == '2') { cr = 60;  cgc = 105; cb = 125; }
    else             { cr = 140; cgc = 130; cb = 105; }
    cr = (cr * sh) >> 8; cgc = (cgc * sh) >> 8; cb = (cb * sh) >> 8;
    cr += (int)(hurt * 0.4f);

    if (type == '#') {
      ph = (hx * 7 + hy * 13) % 5;
      if (ph < 2) {
        wallX = (sd == 0) ? (posY + perp * rdy) : (posX + perp * rdx);
        wallX -= floorf(wallX);
        if (wallX > 0.28f && wallX < 0.72f) {
          paint = 1;
          pY0 = ys + lineH * 22 / 100; pY1 = ys + lineH * 62 / 100;
          if (ph == 0 && (fabsf(wallX - 0.42f) < 0.03f || fabsf(wallX - 0.58f) < 0.03f)) eyes = 1;
        }
      }
    }

    y0 = ys < 0 ? 0 : ys; y1 = ye >= SH ? SH - 1 : ye;
    q = p0 + (SH - 1 - y0) * 3;
    for (y = y0; y <= y1; y++, q -= 3) {
      int r = cr, g = cgc, b = cb;
      if (y > ye - lineH / 12) { r = r * 6 / 10; g = g * 6 / 10; b = b * 6 / 10; }
      else if (paint && y > pY0 && y < pY1) {
        if (wallX < 0.31f || wallX > 0.69f || y < pY0 + 3 || y > pY1 - 3) {
          r = (120 * sh) >> 8; g = (95 * sh) >> 8; b = (40 * sh) >> 8;
        } else if (eyes && abs(y - (pY0 + pY1) / 2) < lineH / 40 + 1) {
          r = (230 * sh) >> 8; g = (230 * sh) >> 8; b = (200 * sh) >> 8;
        } else {
          int c3 = (hx + hy) % 3;
          r = ((c3 == 0 ? 90 : 30) * sh) >> 8;
          g = ((c3 == 1 ? 80 : 35) * sh) >> 8;
          b = ((c3 == 2 ? 100 : 40) * sh) >> 8;
        }
      }
      q[0] = (u8)clamp255(b); q[1] = (u8)clamp255(g); q[2] = (u8)clamp255(r);
    }
    memcpy(p0 + SH * 3, p0, SH * 3);
  }

  drawSprites();

  /* effet de visee (lunette) */
  if (aiming) {
    rect(0, 0, 70, SH, 0, 0, 0);
    rect(SW - 70, 0, 70, SH, 0, 0, 0);
    rect(70, 0, SW - 140, 24, 0, 0, 0);
    rect(70, SH - 24, SW - 140, 24, 0, 0, 0);
  }

  if (state == S_PLAY) {
    /* arme */
    int cx = aiming ? 200 : 275, top = aiming ? 168 : 182;
    rect(cx - 15, top + 24, 30, SH - (top + 24), 70, 75, 80);
    rect(cx - 6, top, 12, 26, 115, 120, 125);
    if (muzzle > 0) rect(cx - 12, top - 16, 24, 16, 255, 230, 120);
    /* viseur */
    rect(194, 119, 13, 2, 255, 60, 60);
    rect(199, 114, 2, 13, 255, 60, 60);
    /* HUD */
    rect(8, 8, 104, 10, 25, 25, 25);
    rect(10, 10, health > 0 ? health : 0, 6, 200, 30, 30);
    for (i = 0; i < cores; i++) rect(10 + i * 14, 24, 10, 10, 60, 255, 210);
    for (i = 0; i < delivered; i++) rect(10 + i * 14, 38, 10, 10, 255, 200, 60);
  }
}

/* ---------- ecran du bas (texte) ---------- */
static void drawConsole(int aliensLeft) {
  consoleClear();
  printf("\x1b[1;1H");
  if (state == S_TITLE) {
    printf(" =====  MUSEE ALIEN  =====\n\n");
    printf(" Tu t'es introduit dans le musee\n");
    printf(" ferme. Il est rempli d'aliens.\n\n");
    printf(" Recupere le coeur de leurs\n");
    printf(" vaisseaux (%d) et ramene-les a la\n", NB_CORES);
    printf(" presidente, dans le hall.\n\n");
    printf(" Tu n'es pas cense etre ici...\n\n\n");
    printf(" A : commencer\n");
    printf(" START : quitter\n");
  } else if (state == S_PLAY) {
    printf(" MUSEE ALIEN\n ------------------------------\n");
    printf(" Vie     : %d\n", health > 0 ? health : 0);
    printf(" Coeurs  : %d en main, %d/%d livres\n", cores, delivered, NB_CORES);
    printf(" Aliens  : %d\n\n", aliensLeft);
    printf(" > %s\n\n", msg);
    printf(" Stick : camera\n");
    printf(" X avancer   B reculer\n");
    printf(" Y gauche    A droite\n");
    printf(" R tirer     L viser (zoom)\n");
    printf(" START : quitter\n");
  } else if (state == S_WIN) {
    printf(" =====  VICTOIRE  =====\n\n");
    printf(" La presidente prend les coeurs.\n");
    printf(" Le musee devient silencieux.\n\n");
    printf(" ...pourquoi la porte d'entree\n");
    printf(" est-elle fermee a clef ?\n\n\n");
    printf(" A : rejouer    START : quitter\n");
  } else {
    printf(" =====  TU ES MORT  =====\n\n");
    printf(" Ils t'ont trouve.\n");
    printf(" Tu n'etais pas cense etre ici.\n\n\n");
    printf(" A : reessayer    START : quitter\n");
  }
}

/* ---------- logique ---------- */
static void shoot(int aim) {
  float spread = aim ? 0.012f : 0.12f;
  float jitter = ((float)(rand() % 2001) - 1000.0f) / 1000.0f * spread;
  int hx, hy, sd, i, best = -1;
  float wd = castRay(posX, posY, dirX, dirY, &hx, &hy, &sd), bestF = 1e9f;
  muzzle = 0.08f;
  for (i = 0; i < nEnt; i++) {
    Ent *e = &ents[i];
    float dx, dy, fwd, side, off;
    if (e->type != T_ALIEN || !e->alive) continue;
    dx = e->x - posX; dy = e->y - posY;
    fwd = dx * dirX + dy * dirY;
    side = -dx * dirY + dy * dirX;
    off = side - fwd * jitter;
    if (fwd > 0.2f && fwd < wd && fabsf(off) < 0.38f && fwd < bestF) { best = i; bestF = fwd; }
  }
  if (best >= 0) {
    Ent *e = &ents[best];
    e->hp -= aim ? 2 : 1;
    e->flash = 0.15f;
    if (e->hp <= 0) { e->alive = 0; say("Un de moins... mais d'autres arrivent."); }
  }
}

static void updatePlay(float dt, u32 kh, circlePosition cp) {
  int aim = (kh & KEY_L) ? 1 : 0, i;
  float turn = 0, look = 0, mx = 0, my = 0, rx, ry, spd, len, nx, ny, target;
  float pd;

  if (abs(cp.dx) > 15) turn = cp.dx / 156.0f;
  if (abs(cp.dy) > 15) look = cp.dy / 156.0f;
  if (turn > 1) { turn = 1; }
  if (turn < -1) { turn = -1; }
  if (look > 1) { look = 1; }
  if (look < -1) { look = -1; }
  ang += turn * (aim ? 0.9f : 2.4f) * dt;
  pitch += look * (aim ? 60.0f : 120.0f) * dt;
  if (pitch > 70) { pitch = 70; }
  if (pitch < -70) { pitch = -70; }

  target = aim ? 0.30f : 0.66f;
  planeLen += (target - planeLen) * (dt * 10.0f > 1 ? 1 : dt * 10.0f);
  dirX = cosf(ang); dirY = sinf(ang);
  planeX = -dirY * planeLen; planeY = dirX * planeLen;
  HS = (float)SW / (2.0f * planeLen);
  rx = -dirY; ry = dirX;

  if (kh & KEY_X) { mx += dirX; my += dirY; }
  if (kh & KEY_B) { mx -= dirX; my -= dirY; }
  if (kh & KEY_A) { mx += rx;   my += ry; }
  if (kh & KEY_Y) { mx -= rx;   my -= ry; }
  len = sqrtf(mx * mx + my * my);
  if (len > 1.0f) { mx /= len; my /= len; }
  spd = aim ? 1.1f : 2.6f;
  nx = posX + mx * spd * dt; ny = posY + my * spd * dt;
  if (canStand(nx, posY, 0.25f)) posX = nx;
  if (canStand(posX, ny, 0.25f)) posY = ny;

  /* tir */
  shotCd -= dt; muzzle -= dt;
  if ((kh & KEY_R) && shotCd <= 0) { shoot(aim); shotCd = 0.4f; }

  /* entites */
  pd = 99.0f;
  for (i = 0; i < nEnt; i++) {
    Ent *e = &ents[i];
    float dx = posX - e->x, dy = posY - e->y, d = sqrtf(dx * dx + dy * dy);
    if (!e->alive) continue;
    if (e->flash > 0) e->flash -= dt;
    switch (e->type) {
      case T_PRES:
        pd = d;
        if (d < 1.8f && cores > 0) {
          delivered += cores; cores = 0; dirty = 1;
          if (delivered >= NB_CORES) { state = S_WIN; dirty = 1; }
          else say("La presidente prend le coeur. Continue.");
        }
        break;
      case T_CORE:
        if (d < 0.8f) {
          e->alive = 0; cores++;
          say("Coeur recupere ! Ramene-le a la presidente.");
        }
        break;
      case T_ALIEN: {
        float ux, uy, sp, nx2, ny2;
        e->cd -= dt;
        if (d < 7.0f && hasLOS(e->x, e->y, posX, posY)) {
          ux = dx / (d + 0.001f); uy = dy / (d + 0.001f); sp = 1.15f;
        } else {
          e->wtimer -= dt;
          if (e->wtimer <= 0) {
            float a = (float)(rand() % 628) / 100.0f;
            e->wx = cosf(a); e->wy = sinf(a);
            e->wtimer = 2.0f + (float)(rand() % 3);
            if (rand() % 3 == 0) { e->wx = 0; e->wy = 0; }
          }
          ux = e->wx; uy = e->wy; sp = 0.5f;
        }
        if (d > 0.65f) {
          nx2 = e->x + ux * sp * dt; ny2 = e->y + uy * sp * dt;
          if (canStand(nx2, e->y, 0.3f)) e->x = nx2; else e->wtimer = 0;
          if (canStand(e->x, ny2, 0.3f)) e->y = ny2; else e->wtimer = 0;
        }
        if (d < 0.95f && e->cd <= 0) { health -= 10; hurt = 160; e->cd = 0.7f; dirty = 1; }
        break;
      }
      case T_STATUE: {
        float dx2 = e->x - posX, dy2 = e->y - posY;
        float fwd = dx2 * dirX + dy2 * dirY, side = -dx2 * dirY + dy2 * dirX;
        int inView = (fwd > 0 && fabsf(side) < fwd * (planeLen + 0.2f));
        if (!inView && d > 2.0f) {
          float nx2 = e->x + (dx / d) * 0.7f * dt, ny2 = e->y + (dy / d) * 0.7f * dt;
          if (canStand(nx2, e->y, 0.3f)) e->x = nx2;
          if (canStand(e->x, ny2, 0.3f)) e->y = ny2;
        }
        break;
      }
    }
  }

  /* lumieres */
  if (pd < 4.5f) { lightLevel = 1.0f; flickLeft = 0; }
  else {
    flickT -= dt;
    if (flickLeft > 0) {
      flickLeft -= dt;
      lightLevel = 0.1f + 0.6f * ((float)(rand() % 100) / 100.0f);
      if (rand() % 4 == 0) lightLevel = 0.04f;
    } else {
      lightLevel += (0.75f - lightLevel) * (dt * 4.0f > 1 ? 1 : dt * 4.0f);
      if (flickT <= 0) { flickLeft = 0.3f + 0.01f * (float)(rand() % 80); flickT = 4.0f + (float)(rand() % 9); }
    }
  }

  if (hurt > 0) { hurt -= 300.0f * dt; if (hurt < 0) hurt = 0; }
  if (health <= 0) { state = S_DEAD; dirty = 1; }

  /* messages d'ambiance */
  if (msgT > 0) { msgT -= dt; if (msgT <= 0) { msg = ""; dirty = 1; } }
  spookT -= dt;
  if (spookT <= 0) { say(spook[rand() % NSPOOK]); spookT = 14.0f + (float)(rand() % 14); }
}

int main(void) {
  u64 last;
  float conT = 0;
  int lastSig = -1;

  gfxInitDefault();
  consoleInit(GFX_BOTTOM, NULL);
  srand((unsigned)osGetTime());
  resetGame();
  state = S_TITLE;
  last = osGetTime();

  while (aptMainLoop()) {
    u64 now = osGetTime();
    float dt = (float)(now - last) / 1000.0f;
    u32 kd, kh;
    circlePosition cp;
    int aliensLeft = 0, i, sig;
    last = now;
    if (dt > 0.1f) dt = 0.1f;
    gameTime += dt;

    hidScanInput();
    kd = hidKeysDown(); kh = hidKeysHeld();
    hidCircleRead(&cp);
    if (kd & KEY_START) break;

    if (state == S_TITLE) {
      ang += 0.3f * dt; posX = 11.5f; posY = 18.5f; pitch = 0; lightLevel = 0.85f; hurt = 0;
      dirX = cosf(ang); dirY = sinf(ang);
      planeLen = 0.66f; planeX = -dirY * planeLen; planeY = dirX * planeLen;
      HS = (float)SW / (2.0f * planeLen);
      if (kd & KEY_A) { resetGame(); state = S_PLAY; say("Trouve les coeurs. Evite les statues..."); }
    } else if (state == S_PLAY) {
      updatePlay(dt, kh, cp);
    } else {
      if (state == S_DEAD) hurt = 160; else lightLevel = 1.0f;
      if (kd & KEY_A) { resetGame(); state = S_PLAY; say("Trouve les coeurs. Evite les statues..."); }
    }

    for (i = 0; i < nEnt; i++) if (ents[i].type == T_ALIEN && ents[i].alive) aliensLeft++;
    sig = health * 10000 + cores * 1000 + delivered * 100 + aliensLeft * 10 + state;
    if (sig != lastSig) { dirty = 1; lastSig = sig; }
    conT += dt;
    if (dirty && conT > 0.15f) { drawConsole(aliensLeft); dirty = 0; conT = 0; }

    render(state == S_PLAY && (kh & KEY_L));
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }

  gfxExit();
  return 0;
}
