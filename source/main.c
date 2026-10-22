#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCR_W 400
#define SCR_H 240
#define NEAR_Z 0.25f

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float x, y, z; } V3;

static V3 vmk(float x, float y, float z) { V3 v = { x, y, z }; return v; }
static V3 vadd(V3 a, V3 b) { return vmk(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 vsub(V3 a, V3 b) { return vmk(a.x - b.x, a.y - b.y, a.z - b.z); }
static float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

typedef struct {
    float x, z;
    float health;
    int active;
} Alien;

static float playerX = 0.0f, playerY = 1.0f, playerZ = -3.0f;
static float playerYaw = 0.0f;   // Horizontal (gauche / droite)
static float playerPitch = 0.0f; // Vertical (haut / bas)
static int aliensKilled = 0;
static int hasHeart = 0;
static Alien aliens[5];

void initGame(void) {
    playerX = 0.0f;
    playerY = 1.0f;
    playerZ = -3.0f;
    playerYaw = 0.0f;
    playerPitch = 0.0f;
    aliensKilled = 0;
    hasHeart = 0;

    for(int i = 0; i < 5; i++) {
        aliens[i].x = (float)((rand() % 14) - 7);
        aliens[i].z = (float)((rand() % 10) + 4);
        aliens[i].health = 100.0f;
        aliens[i].active = 1;
    }
}

// Moteur de rendu 3D projeté
static void drawPoly(const V3 *pts, int n, u32 clr, float focal) {
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
        sx[i] = SCR_W / 2 + focal * out[i].x / out[i].z;
        sy[i] = SCR_H / 2 - focal * out[i].y / out[i].z;
    }
    for (int i = 1; i < m - 1; i++)
        C2D_DrawTriangle(sx[0], sy[0], clr, sx[i], sy[i], clr, sx[i + 1], sy[i + 1], clr, 0);
}

static void drawCube3D(V3 pos, V3 size, u32 clr, V3 camPos, V3 camR, V3 camU, V3 camF, float focal) {
    float hx = size.x / 2.0f, hy = size.y / 2.0f, hz = size.z / 2.0f;
    V3 corners[8] = {
        {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
        {-hx, -hy,  hz}, {hx, -hy,  hz}, {hx, hy,  hz}, {-hx, hy,  hz}
    };
    V3 camCorners[8];
    for(int i = 0; i < 8; i++) {
        V3 w = vadd(pos, corners[i]);
        V3 d = vsub(w, camPos);
        camCorners[i] = vmk(vdot(d, camR), vdot(d, camU), vdot(d, camF));
    }

    int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
    };

    for(int f = 0; f < 6; f++) {
        V3 poly[4];
        for(int v = 0; v < 4; v++) poly[v] = camCorners[faces[f][v]];
        drawPoly(poly, 4, clr, focal);
    }
}

// Modélisation d'un Alien 3D (Tête, Torse, Bras, Jambes)
static void drawAlien3D(V3 pos, V3 camPos, V3 camR, V3 camU, V3 camF, float focal) {
    u32 greenColor = C2D_Color32(50, 220, 80, 255);
    u32 darkGreen  = C2D_Color32(30, 150, 50, 255);
    u32 eyeColor    = C2D_Color32(255, 0, 0, 255);

    // Torse
    drawCube3D(vadd(pos, vmk(0, 0.7f, 0)), vmk(0.6f, 0.8f, 0.4f), greenColor, camPos, camR, camU, camF, focal);
    // Tête
    drawCube3D(vadd(pos, vmk(0, 1.4f, 0)), vmk(0.5f, 0.5f, 0.5f), greenColor, camPos, camR, camU, camF, focal);
    // Yeux
    drawCube3D(vadd(pos, vmk(0.12f, 1.45f, -0.26f)), vmk(0.1f, 0.1f, 0.1f), eyeColor, camPos, camR, camU, camF, focal);
    drawCube3D(vadd(pos, vmk(-0.12f, 1.45f, -0.26f)), vmk(0.1f, 0.1f, 0.1f), eyeColor, camPos, camR, camU, camF, focal);
    // Bras
    drawCube3D(vadd(pos, vmk(-0.45f, 0.7f, 0)), vmk(0.2f, 0.7f, 0.2f), darkGreen, camPos, camR, camU, camF, focal);
    drawCube3D(vadd(pos, vmk(0.45f, 0.7f, 0)), vmk(0.2f, 0.7f, 0.2f), darkGreen, camPos, camR, camU, camF, focal);
    // Jambes
    drawCube3D(vadd(pos, vmk(-0.2f, 0.15f, 0)), vmk(0.2f, 0.5f, 0.2f), darkGreen, camPos, camR, camU, camF, focal);
    drawCube3D(vadd(pos, vmk(0.2f, 0.15f, 0)), vmk(0.2f, 0.5f, 0.2f), darkGreen, camPos, camR, camU, camF, focal);
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH24);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    initGame();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        int isAiming = (kHeld & KEY_L);
        float focal = isAiming ? 420.0f : 270.0f; // Effet Zoom en mode visée
        float sens = isAiming ? 0.00004f : 0.00009f;

        // Contrôle Caméra Libre à 360° avec le Circle Pad
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) playerYaw   += (float)cp.dx * sens;
        if (abs(cp.dy) > 15) playerPitch += (float)cp.dy * sens;

        // Limite de la caméra verticale (regard haut/bas)
        if (playerPitch > 1.2f)  playerPitch = 1.2f;
        if (playerPitch < -1.2f) playerPitch = -1.2f;

        // Déplacements corrigés : X (Avancer), B (Reculer), Y (Droite), A (Gauche)
        float speed = 0.09f;
        if (kHeld & KEY_X) {
            playerX += sinf(playerYaw) * speed;
            playerZ += cosf(playerYaw) * speed;
        }
        if (kHeld & KEY_B) {
            playerX -= sinf(playerYaw) * speed;
            playerZ -= cosf(playerYaw) * speed;
        }
        if (kHeld & KEY_Y) {
            playerX += cosf(playerYaw) * speed;
            playerZ -= sinf(playerYaw) * speed;
        }
        if (kHeld & KEY_A) {
            playerX -= cosf(playerYaw) * speed;
            playerZ += sinf(playerYaw) * speed;
        }

        // Tir avec la gâchette R
        if (kDown & KEY_R) {
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist < (isAiming ? 7.0f : 4.5f)) {
                        aliens[i].active = 0;
                        aliensKilled++;
                        if (aliensKilled >= 5) hasHeart = 1;
                        break;
                    }
                }
            }
        }

        // --- RENDU 3D DÉCOR MUSÉE & ALIENS ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x12121EFF, 0);
        C3D_FrameDrawOn(target);

        C2D_Prepare();
        C2D_SceneBegin(target);

        // Vecteurs de la caméra 3D avec Yaw et Pitch
        V3 camPos = vmk(playerX, playerY, playerZ);
        float cy = cosf(playerYaw), sy = sinf(playerYaw);
        float cp_f = cosf(playerPitch), sp = sinf(playerPitch);

        V3 camF = vmk(sy * cp_f, sp, cy * cp_f);
        V3 camR = vmk(cy, 0, -sy);
        V3 camU = vmk(-sy * sp, cp_f, -cy * sp);

        // Sol du Musée (Quadrillage de dalles)
        for(int x = -10; x <= 10; x += 2) {
            for(int z = -10; z <= 15; z += 2) {
                u32 tileCol = ((x + z) % 4 == 0) ? C2D_Color32(180, 180, 190, 255) : C2D_Color32(120, 120, 130, 255);
                drawCube3D(vmk((float)x, -0.1f, (float)z), vmk(1.9f, 0.1f, 1.9f), tileCol, camPos, camR, camU, camF, focal);
            }
        }

        // Murs du Musée
        u32 wallCol = C2D_Color32(80, 90, 120, 255);
        for(int x = -10; x <= 10; x += 2) {
            drawCube3D(vmk((float)x, 2.0f, 16.0f), vmk(1.9f, 4.0f, 0.4f), wallCol, camPos, camR, camU, camF, focal);
            drawCube3D(vmk((float)x, 2.0f, -10.0f), vmk(1.9f, 4.0f, 0.4f), wallCol, camPos, camR, camU, camF, focal);
        }

        // Piédestaux d'exposition du musée
        u32 pedCol = C2D_Color32(200, 170, 110, 255);
        drawCube3D(vmk(-4.0f, 0.5f, 4.0f), vmk(1.2f, 1.0f, 1.2f), pedCol, camPos, camR, camU, camF, focal);
        drawCube3D(vmk(4.0f, 0.5f, 4.0f), vmk(1.2f, 1.0f, 1.2f), pedCol, camPos, camR, camU, camF, focal);

        // Affichage des Aliens 3D
        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                float dx = playerX - aliens[i].x;
                float dz = playerZ - aliens[i].z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist > 0.8f) {
                    aliens[i].x += (dx / dist) * 0.009f;
                    aliens[i].z += (dz / dist) * 0.009f;
                }
                drawAlien3D(vmk(aliens[i].x, 0.0f, aliens[i].z), camPos, camR, camU, camF, focal);
            }
        }

        // Viseur HUD (change de couleur en mode visée)
        u32 crosshairCol = isAiming ? C2D_Color32(255, 30, 30, 255) : C2D_Color32(0, 255, 255, 255);
        C2D_DrawRectSolid(197, 119, 0.5f, 6, 2, crosshairCol);
        C2D_DrawRectSolid(199, 117, 0.5f, 2, 6, crosshairCol);

        C3D_FrameEnd(0);

        // --- HUD CONSOLE BAS ---
        consoleClear();
        printf("\x1b[1;1H=== MUSEE ALIEN : EDITION 3D ===");
        printf("\x1b[3;1HCommandes :");
        printf("\x1b[4;1H [X] Avancer | [B] Reculer");
        printf("\x1b[5;1H [A] Gauche  | [Y] Droite");
        printf("\x1b[6;1H [L] Visée Zoom | [R] Tirer");
        printf("\x1b[8;1HAliens elimines : %d / 5", aliensKilled);
        if (hasHeart) {
            printf("\x1b[10;1H*** VICTOIRE ! LE MUSÉE EST SÉCURISÉ ! ***");
        }

        svcSleepThread(1000000);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
