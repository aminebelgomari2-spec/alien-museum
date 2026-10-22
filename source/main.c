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

static float playerX = 0.0f, playerZ = -2.0f;
static float playerAngle = 0.0f;
static int hasHeart = 0;
static int gameWon = 0;
static int aliensKilled = 0;
static Alien aliens[5];

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

    u64 lastTick = osGetTime();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // Contrôles caméra
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) {
            playerAngle -= (float)cp.dx * 0.00008f;
        }

        float moveSpeed = 0.08f;
        if (kHeld & KEY_X) { playerX += sinf(playerAngle) * moveSpeed; playerZ += cosf(playerAngle) * moveSpeed; }
        if (kHeld & KEY_B) { playerX -= sinf(playerAngle) * moveSpeed; playerZ -= cosf(playerAngle) * moveSpeed; }
        if (kHeld & KEY_Y) { playerX -= cosf(playerAngle) * moveSpeed; playerZ += sinf(playerAngle) * moveSpeed; }
        if (kHeld & KEY_A) { playerX += cosf(playerAngle) * moveSpeed; playerZ -= cosf(playerAngle) * moveSpeed; }

        // Déplacement des aliens
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

        // Tir sur les aliens avec R
        if (kDown & KEY_R) {
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist < 6.0f) {
                        aliens[i].health -= 50.0f;
                        if (aliens[i].health <= 0) {
                            aliens[i].active = 0;
                            aliensKilled++;
                            if (aliensKilled >= 5) hasHeart = 1;
                        }
                        break;
                    }
                }
            }
        }

        if (hasHeart && fabsf(playerX) < 1.5f && fabsf(playerZ) < 1.5f) {
            gameWon = 1;
        }

        // --- RENDU ECRAN DU HAUT (3D + HUD 2D) ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x1A1A2EFF, 0); // Fond bleu/nuit visible
        C3D_FrameDrawOn(target);

        C2D_Prepare();
        C2D_SceneBegin(target);

        // Dessin de faux "murs 3D" stylisés en 2D pour structurer l'espace visuel en attendant les polygones 3D complets
        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                float relX = aliens[i].x - playerX;
                float relZ = aliens[i].z - playerZ;
                // Projection simple à l'écran
                int screenX = 200 + (int)(relX * 40.0f);
                int screenY = 120 + (int)(relZ * 10.0f);
                if(screenX > 0 && screenX < 400 && screenY > 0 && screenY < 240) {
                    C2D_DrawCircleSolid(screenX, screenY, 0.5f, 15, C2D_Color32(255, 50, 50, 255)); // Alien visuel
                }
            }
        }

        // Viseur central fixe
        C2D_DrawRectSolid(198, 119, 0.5f, 4, 2, C2D_Color32(0, 255, 255, 255));
        C2D_DrawRectSolid(199, 118, 0.5f, 2, 4, C2D_Color32(0, 255, 255, 255));

        // Mini-carte radar en haut à droite
        C2D_DrawRectSolid(300, 10, 0.5f, 90, 90, C2D_Color32(10, 10, 20, 220));
        C2D_DrawCircleSolid(345, 55, 0.5f, 3, C2D_Color32(0, 255, 0, 255)); // Joueur sur radar
        for(int i = 0; i < 5; i++) {
            if (aliens[i].active) {
                C2D_DrawCircleSolid(345 + (int)((aliens[i].x - playerX) * 8.0f), 55 + (int)((aliens[i].z - playerZ) * 8.0f), 0.5f, 2, C2D_Color32(255, 0, 0, 255));
            }
        }

        C3D_FrameEnd(0);

        // --- AFFICHAGE STABLE DE L'ECRAN DU BAS ---
        // On ne rafraîchit l'affichage texte qu'une fois toutes les ~150ms pour éliminer le clignotement (bug de lecture)
        u64 currentTick = osGetTime();
        if (currentTick - lastTick > 150) {
            consoleClear();
            printf("\x1b[1;1H=== MUSEE ALIEN : 3D ===");
            printf("\x1b[3;1HPos  : X=%.1f | Z=%.1f", (double)playerX, (double)playerZ);
            printf("\x1b[4;1HCap  : %.0f degres", (double)(playerAngle * 57.2958f));
            printf("\x1b[6;1HAliens elimines : %d / 5", aliensKilled);
            printf("\x1b[7;1HCoeur d'Alien    : %s", hasHeart ? "[ TROUVE ]" : "[ MANQUANT ]");

            if (gameWon) {
                printf("\x1b[10;1H*** VICTOIRE TOTALE ! ***");
            } else if (hasHeart) {
                printf("\x1b[10;1H! Retournez au centre (0,0) !");
            } else {
                printf("\x1b[10;1HUtilisez [R] pour tirer sur les aliens.");
            }
            lastTick = currentTick;
        }

        // Petite pause pour stabiliser le CPU de la 3DS
        svcSleepThread(1000000);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
