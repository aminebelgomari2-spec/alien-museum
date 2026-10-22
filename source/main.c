#include <citro3d.h>
#include <citro2d.h>
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
    float x, z;
    float health;
    int active;
} Alien;

static float playerX = 0.0f, playerZ = 0.0f;
static float playerAngle = 0.0f;
static int hasHeart = 0;
static int gameWon = 0;
static int aliensKilled = 0;
static Alien aliens[5];
static touchPosition touch;

void initGame(void) {
    playerX = 0.0f;
    playerZ = -2.0f;
    playerAngle = 0.0f;
    hasHeart = 0;
    gameWon = 0;
    aliensKilled = 0;
    
    for(int i = 0; i < 5; i++) {
        aliens[i].x = (float)(rand() % 16 - 8);
        aliens[i].z = (float)(rand() % 12 + 4);
        aliens[i].health = 100.0f;
        aliens[i].active = 1;
    }
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
        hidTouchRead(&touch);

        if (kDown & KEY_START) break;

        // Contrôles de la caméra
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) {
            playerAngle -= (float)cp.dx * 0.00008f;
        }

        float moveSpeed = 0.08f;
        if (kHeld & KEY_X) {
            playerX += sinf(playerAngle) * moveSpeed;
            playerZ += cosf(playerAngle) * moveSpeed;
        }
        if (kHeld & KEY_B) {
            playerX -= sinf(playerAngle) * moveSpeed;
            playerZ -= cosf(playerAngle) * moveSpeed;
        }
        if (kHeld & KEY_Y) {
            playerX -= cosf(playerAngle) * moveSpeed;
            playerZ += sinf(playerAngle) * moveSpeed;
        }
        if (kHeld & KEY_A) {
            playerX += cosf(playerAngle) * moveSpeed;
            playerZ -= sinf(playerAngle) * moveSpeed;
        }

        // Déplacements simples des aliens vers le joueur
        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                float dx = playerX - aliens[i].x;
                float dz = playerZ - aliens[i].z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist > 0.8f) {
                    aliens[i].x += (dx / dist) * 0.015f;
                    aliens[i].z += (dz / dist) * 0.015f;
                }
            }
        }

        // Tir
        int isAiming = (kHeld & KEY_L);
        if (kDown & KEY_R) {
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist < 5.0f) {
                        aliens[i].health -= isAiming ? 100.0f : 50.0f;
                        if (aliens[i].health <= 0) {
                            aliens[i].active = 0;
                            aliensKilled++;
                            if (aliensKilled >= 5) {
                                hasHeart = 1;
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (hasHeart && fabsf(playerX) < 1.5f && fabsf(playerZ) < 1.5f) {
            gameWon = 1;
        }

        // --- DEBUT RENDU 3D ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x05050A00, 0);
        C3D_FrameDrawOn(target);

        // Dessin 2D d'ambiance / HUD sur l'écran du haut via C2D
        C2D_Prepare();
        C2D_SceneBegin(target);
        
        // Viseur au centre
        C2D_DrawRectSolid(198, 119, 4, 2, C2D_Color32(255, 0, 0, 255));
        C2D_DrawRectSolid(199, 118, 2, 4, C2D_Color32(255, 0, 0, 255));

        // Radar / Carte en haut à droite
        C2D_DrawRectSolid(310, 10, 80, 80, C2D_Color32(20, 20, 30, 200));
        C2D_DrawCircleSolid(350 + (int)(playerX * 3.0f), 50 + (int)(playerZ * 3.0f), 0, 3, C2D_Color32(0, 255, 0, 255));

        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                C2D_DrawCircleSolid(350 + (int)(aliens[i].x * 3.0f), 50 + (int)(aliens[i].z * 3.0f), 0, 3, C2D_Color32(255, 0, 0, 255));
            }
        }

        C3D_FrameEnd(0);

        // UI Console sur l'écran du bas
        consoleClear();
        printf("\x1b[1;1H=== MUSEE ALIEN 3D ===");
        printf("\x1b[3;1HPosition : X=%.1f | Z=%.1f", (double)playerX, (double)playerZ);
        printf("\x1b[4;1HAngle    : %.1f degres", (double)(playerAngle * 57.2958f));
        printf("\x1b[6;1HAliens elimines : %d/5", aliensKilled);
        printf("\x1b[7;1HCoeur d'Alien    : %s", hasHeart ? "[ OUI ]" : "[ NON ]");

        if (gameWon) {
            printf("\x1b[10;1H*** VICTOIRE ! LA PRESIDENTE EST SAUVEE ! ***");
        } else if (hasHeart) {
            printf("\x1b[10;1H! COEUR RECUPERE ! Retournez en (0,0) !");
        } else {
            printf("\x1b[10;1HTuez tous les aliens pour extraire le coeur.");
        }

        svcSleepThread(10000000);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
