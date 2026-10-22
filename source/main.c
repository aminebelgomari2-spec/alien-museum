#include <citro3d.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <3ds.h>

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALING_NO))

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    float x, z;
    float health;
    int active;
} Alien;

// Variables de jeu
static float playerX = 0.0f, playerZ = 0.0f;
static float playerAngle = 0.0f;
static int hasHeart = 0;
static int gameWon = 0;
static int aliensKilled = 0;
static Alien aliens[5];
static touchPosition touch;

void initGame(void) {
    playerX = 0.0f;
    playerZ = 0.0f;
    playerAngle = 0.0f;
    hasHeart = 0;
    gameWon = 0;
    aliensKilled = 0;
    
    // Placer quelques aliens dans le musée
    for(int i = 0; i < 5; i++) {
        aliens[i].x = (float)(rand() % 10 - 5);
        aliens[i].z = (float)(rand() % 10 + 5);
        aliens[i].health = 100.0f;
        aliens[i].active = 1;
    }
}

int main(int argc, char* argv[]) {
    // Initialisation des services système
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_PrepareCtrl();
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH24);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    initGame();

    printf("\x1b[1;1H=== MUSEE ALIEN : HORREUR 3DS ===");
    printf("\x1b[3;1HCommandes :");
    printf("\x1b[4;1H- Joystick : Tourner la camera");
    printf("\x1b[5;1H- X : Avancer | B : Reculer");
    printf("\x1b[6;1H- Y : Gauche | A : Droite");
    printf("\x1b[7;1H- L : Viser | R : Tirer");
    printf("\x1b[9;1HObjectif : Trouver le coeur &");
    printf("\x1b[10;1Hramener le a la Presidente (Z=0)");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        hidTouchRead(&touch);

        if (kDown & KEY_START) break; // Quitter

        // Gestion de la caméra via le Circle Pad (Joystick)
        circlePosition cp;
        hidCircleRead(&cp);
        if (abs(cp.dx) > 15) {
            playerAngle -= (float)cp.dx * 0.00005f;
        }

        // Déplacements demandés
        float moveSpeed = 0.05f;
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

        // Tir (R) et Visée (L)
        int isAiming = (kHeld & KEY_L);
        if (kDown & KEY_R) {
            // Tirer sur le premier alien en vie dans la ligne de mire
            for(int i = 0; i < 5; i++) {
                if (aliens[i].active) {
                    float dx = aliens[i].x - playerX;
                    float dz = aliens[i].z - playerZ;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist < 4.0f) { // Portée de tir
                        aliens[i].health -= isAiming ? 100.0f : 50.0f;
                        if (aliens[i].health <= 0) {
                            aliens[i].active = 0;
                            aliensKilled++;
                            if (aliensKilled >= 5) {
                                hasHeart = 1; // Le dernier alien lâche le coeur
                            }
                        }
                        break;
                    }
                }
            }
        }

        // Condition de victoire : ramener le cœur au point de départ (0,0)
        if (hasHeart && fabsf(playerX) < 1.0f && fabsf(playerZ) < 1.0f) {
            gameWon = 1;
        }

        // Rendu graphique 3D de l'écran supérieur
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x101015FF, 0); // Fond sombre, angoissant
        C3D_FrameEnd();

        // Affichage console bas
        printf("\x1b[13;1HPos: X=%.1f Z=%.1f Angle=%.2f", (double)playerX, (double)playerZ, (double)playerAngle);
        printf("\x1b[14;1HAliens tues: %d/5 | Coeur: %s", aliensKilled, hasHeart ? "OUI" : "NON");
        if (gameWon) {
            printf("\x1b[16;1HVICTOIRE ! La presidente est sauvee !");
        } else if (hasHeart) {
            printf("\x1b[16;1HVous avez le coeur ! Ramenez-le en (0,0) !");
        }

        svcSleepThread(10000000); // ~10ms
    }

    // Libération des ressources
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}