#include <citro3d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct {
    float x, y, z;
    float health;
    int active;
} Alien;

static float playerX = 0.0f, playerY = 0.0f, playerZ = -3.0f;
static float playerAngle = 0.0f;
static Alien aliens[5];
static int aliensKilled = 0;

void initGame(void) {
    playerX = 0.0f;
    playerY = 0.0f;
    playerZ = -4.0f;
    playerAngle = 0.0f;
    aliensKilled = 0;

    for(int i = 0; i < 5; i++) {
        aliens[i].x = (float)((rand() % 10) - 5);
        aliens[i].y = 0.0f;
        aliens[i].z = (float)((rand() % 10) + 3);
        aliens[i].health = 100.0f;
        aliens[i].active = 1;
    }
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH24);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    initGame();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // Commandes de déplacement
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) {
            playerAngle -= (float)cp.dx * 0.0001f;
        }

        float speed = 0.1f;
        if (kHeld & KEY_DUP || kHeld & KEY_Y) {
            playerX += sinf(playerAngle) * speed;
            playerZ += cosf(playerAngle) * speed;
        }
        if (kHeld & KEY_DDOWN || kHeld & KEY_A) {
            playerX -= sinf(playerAngle) * speed;
            playerZ -= cosf(playerAngle) * speed;
        }

        // Tir
        if (kDown & KEY_R) {
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    if (sqrtf(dx*dx + dz*dz) < 4.0f) {
                        aliens[i].active = 0;
                        aliensKilled++;
                        break;
                    }
                }
            }
        }

        // --- RENDU ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x101018FF, 0);
        C3D_FrameDrawOn(target);

        C3D_FrameEnd(0);

        // --- AFFICHAGE BAS (HUD) ---
        consoleClear();
        printf("\x1b[1;1H=== MUSEE ALIEN : 3D ===");
        printf("\x1b[3;1HXYZ: %.1f, %.1f, %.1f", (double)playerX, (double)playerY, (double)playerZ);
        printf("\x1b[4;1HAliens elimines : %d / 5", aliensKilled);
        printf("\x1b[6;1HUtilisez le Stick pour bouger");
        printf("\x1b[7;1HEt la gachette [R] pour tirer !");

        svcSleepThread(1000000);
    }

    C3D_Fini();
    gfxExit();
    return 0;
}
