#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCR_W 400
#define SCR_H 240
#define FOCAL 270.0f
#define NEAR_Z 0.25f

typedef struct { float x, y, z; } V3;

static V3 vmk(float x, float y, float z) { V3 v = { x, y, z }; return v; }
static V3 vadd(V3 a, V3 b) { return vmk(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 vsub(V3 a, V3 b) { return vmk(a.x - b.x, a.y - b.y, a.z - b.z); }
static V3 vmul(V3 a, float s) { return vmk(a.x * s, a.y * s, a.z * s); }
static float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static float vlen(V3 a) { return sqrtf(vdot(a, a)); }

typedef struct {
    float x, z;
    float health;
    int active;
} Alien;

static float playerX = 0.0f, playerZ = 0.0f;
static float playerYaw = 0.0f;
static int aliensKilled = 0;
static int hasHeart = 0;
static Alien aliens[5];

void initGame(void) {
    playerX = 0.0f;
    playerZ = -3.0f;
    playerYaw = 0.0f;
    aliensKilled = 0;
    hasHeart = 0;

    for(int i = 0; i < 5; i++) {
        aliens[i].x = (float)((rand() % 10) - 5);
        aliens[i].z = (float)((rand() % 10) + 4);
        aliens[i].health = 100.0f;
        aliens[i].active = 1;
    }
}

// Fonction de dessin de polygones 3D projetés sur l'écran 3DS
static void drawPoly(const V3 *pts, int n, u32 clr) {
    V3 out[20];
    int m = 0;
    for (int i = 0; i < n; i++) {
        V3 a = pts[i], b = pts[(i + 1) % n];
        int ia = a.z >= NEAR_Z, ib = b.z >= NEAR_Z;
        if (ia) out[m++] = a;
        if (ia != ib) {
            float t = (NEAR_Z - a.z) / (b.z - a.z);
            out[m++] = vmk(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, NEAR_Z);
        }
    }
    if (m < 3) return;
    float sx[20], sy[20];
    for (int i = 0; i < m; i++) {
        sx[i] = SCR_W / 2 + FOCAL * out[i].x / out[i].z;
        sy[i] = SCR_H / 2 - FOCAL * out[i].y / out[i].z;
    }
    for (int i = 1; i < m - 1; i++)
        C2D_DrawTriangle(sx[0], sy[0], clr, sx[i], sy[i], clr, sx[i + 1], sy[i + 1], clr, 0);
}

// Dessine un cube 3D plein dans l'espace
static void drawCube3D(V3 pos, float size, u32 clr, V3 camPos, V3 camR, V3 camU, V3 camF) {
    float h = size / 2.0f;
    V3 corners[8] = {
        {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h},
        {-h, -h,  h}, {h, -h,  h}, {h, h,  h}, {-h, h,  h}
    };
    V3 worldCorners[8];
    for(int i = 0; i < 8; i++) {
        worldCorners[i] = vadd(pos, corners[i]);
    }
    
    // Transformation caméra
    V3 camCorners[8];
    for(int i = 0; i < 8; i++) {
        V3 d = vsub(worldCorners[i], camPos);
        camCorners[i] = vmk(vdot(d, camR), vdot(d, camU), vdot(d, camF));
    }

    int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
    };

    for(int f = 0; f < 6; f++) {
        V3 poly[4];
        for(int v = 0; v < 4; v++) {
            poly[v] = camCorners[faces[f][v]];
        }
        drawPoly(poly, 4, clr);
    }
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* topTarget = C3D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    initGame();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // Contrôles Circle Pad pour se déplacer
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) playerYaw += (float)cp.dx * 0.00008f;

        float speed = 0.08f;
        if (kHeld & KEY_DUP || kHeld & KEY_Y) {
            playerX += sinf(playerYaw) * speed;
            playerZ += cosf(playerYaw) * speed;
        }
        if (kHeld & KEY_DDOWN || kHeld & KEY_A) {
            playerX -= sinf(playerYaw) * speed;
            playerZ -= cosf(playerYaw) * speed;
        }

        // Tir sur les aliens avec la gâchette R
        if (kDown & KEY_R) {
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    if (sqrtf(dx*dx + dz*dz) < 3.5f) {
                        aliens[i].active = 0;
                        aliensKilled++;
                        if (aliensKilled >= 5) hasHeart = 1;
                        break;
                    }
                }
            }
        }

        // --- RENDU 3D ÉCRAN DU HAUT ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(20, 20, 35, 255));
        C2D_SceneBegin(topTarget);

        // Configuration de la caméra 3D subjective
        V3 camPos = vmk(playerX, 1.0f, playerZ);
        float cy = cosf(playerYaw), sy = sinf(playerYaw);
        V3 camF = vmk(sy, 0, cy);
        V3 camR = vmk(cy, 0, -sy);
        V3 camU = vmk(0, 1, 0);

        // Dessin des murs du musée en 3D
        for(int x = -4; x <= 4; x += 2) {
            drawCube3D(vmk((float)x, 1.0f, 6.0f), 1.8f, C2D_Color32(100, 120, 180, 255), camPos, camR, camU, camF);
            drawCube3D(vmk((float)x, 1.0f, -6.0f), 1.8f, C2D_Color32(100, 120, 180, 255), camPos, camR, camU, camF);
        }

        // Dessin des aliens vivants en 3D (cubes rouges menaçants)
        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                // Approche lente vers le joueur
                float dx = playerX - aliens[i].x;
                float dz = playerZ - aliens[i].z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist > 0.6f) {
                    aliens[i].x += (dx / dist) * 0.008f;
                    aliens[i].z += (dz / dist) * 0.008f;
                }
                drawCube3D(vmk(aliens[i].x, 0.8f, aliens[i].z), 1.2f, C2D_Color32(230, 50, 50, 255), camPos, camR, camU, camF);
            }
        }

        // Viseur 2D fixe au centre de l'écran
        C2D_DrawRectSolid(198, 118, 0.5f, 4, 2, C2D_Color32(0, 255, 255, 255));
        C2D_DrawRectSolid(199, 117, 0.5f, 2, 4, C2D_Color32(0, 255, 255, 255));

        C3D_FrameEnd(0);

        // --- AFFICHAGE CONSOLE ÉCRAN DU BAS ---
        consoleClear();
        printf("\x1b[1;1H=== MUSEE ALIEN : 3D REEL ===");
        printf("\x1b[3;1HPosition : X=%.1f | Z=%.1f", (double)playerX, (double)playerZ);
        printf("\x1b[4;1HAliens elimines : %d / 5", aliensKilled);
        printf("\x1b[6;1HUtilisez [R] pour tirer sur les cubes rouges !");
        if (hasHeart) {
            printf("\x1b[8;1H*** COEUR EXTRAIT ! VICTOIRE ! ***");
        }

        svcSleepThread(1000000);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
