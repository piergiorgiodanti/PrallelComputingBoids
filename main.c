#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
} Boid;

void init_boids(Boid* boids, int num_boids, int screenWidth, int screenHeight, int marginx, int marginy);
void update_boid_logic(Boid* current, Boid* boids, int num_boids, float avoidfactor, float protected_range, float matchingfactor, float centeringfactor, float visual_range);
void draw_boids(Boid* boids, int num_boids);

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    const int marginx = 200;
    const int marginy = 150;
    const int leftmargin = marginx;
    const int rightmargin = screenWidth - marginx;
    const int topmargin = marginy;
    const int bottommargin = screenHeight - marginy;

    const int num_boids = 1000;

    float protected_range = 20.0f;
    float visual_range = 80.0f;
    float avoidfactor = 0.05f;
    float matchingfactor = 0.05f;
    float centeringfactor = 0.0005f;
    float turnfactor = 5.0f;
    float maxspeed = 400.0f;
    float minspeed = 200.0f;

    Boid* boids = (Boid*)malloc(sizeof(Boid) * num_boids);
    if (boids == NULL) return 1;

    init_boids(boids, num_boids, screenWidth, screenHeight, marginx, marginy);

    InitWindow(screenWidth, screenHeight, "Boids Simulation");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        for (int i = 0; i < num_boids; i++) {
            update_boid_logic(&boids[i], boids, num_boids, avoidfactor, protected_range, matchingfactor, centeringfactor, visual_range);

            // Screen edges
            if (boids[i].x < leftmargin) boids[i].vx = boids[i].vx + turnfactor;
            if (boids[i].x > rightmargin) boids[i].vx = boids[i].vx - turnfactor;
            if (boids[i].y > bottommargin) boids[i].vy = boids[i].vy - turnfactor;
            if (boids[i].y < topmargin) boids[i].vy = boids[i].vy + turnfactor;

            // Speed limits
            float speed = sqrt(boids[i].vx*boids[i].vx + boids[i].vy*boids[i].vy);
            if (speed > maxspeed) {
                boids[i].vx = (boids[i].vx/speed)*maxspeed;
                boids[i].vy = (boids[i].vy/speed)*maxspeed;
            }
            if (speed < minspeed) {
                boids[i].vx = (boids[i].vx/speed)*minspeed;
                boids[i].vy = (boids[i].vy/speed)*minspeed;
            }

            // Update position
            boids[i].x += boids[i].vx * GetFrameTime();
            boids[i].y += boids[i].vy * GetFrameTime();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawRectangleLines(marginx, marginy, screenWidth - 2*marginx, screenHeight - 2*marginy, LIGHTGRAY);
        draw_boids(boids, num_boids);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    free(boids);
    return 0;
}

void init_boids(Boid* boids, int num_boids, int screenWidth, int screenHeight, int marginx, int marginy) {
    for (int i = 0; i < num_boids; i++) {
        boids[i].x = marginx + (rand() % (screenWidth - 2 * marginx));
        boids[i].y = marginy + (rand() % (screenHeight - 2 * marginy));

        boids[i].vx = (float)((rand() % 200) - 100);
        boids[i].vy = (float)((rand() % 200) - 100);
    }
}

void update_boid_logic(Boid* current, Boid* boids, int num_boids, float avoidfactor, float protected_range, float matchingfactor, float centeringfactor, float visual_range) {
    float close_dx = 0, close_dy = 0;      // Per Separation
    float vel_avg_x = 0, vel_avg_y = 0;    // Per Alignment
    float pos_avg_x = 0, pos_avg_y = 0;    // Per Cohesion
    int neighbors = 0;

    for (int i = 0; i < num_boids; i++) {
        if (&boids[i] == current) continue;

        float dx = current->x - boids[i].x;
        float dy = current->y - boids[i].y;

        float dist_abs_x = fabsf(dx);
        float dist_abs_y = fabsf(dy);

        // SEPARATION
        if (dist_abs_x < protected_range && dist_abs_y < protected_range) {
            close_dx += dx;
            close_dy += dy;
        }

        // ALIGNMENT & COHESION
        if (dist_abs_x < visual_range && dist_abs_y < visual_range) {
            vel_avg_x += boids[i].vx;
            vel_avg_y += boids[i].vy;
            pos_avg_x += boids[i].x;
            pos_avg_y += boids[i].y;
            neighbors++;
        }
    }

    // Applichiamo Separation
    current->vx += close_dx * avoidfactor;
    current->vy += close_dy * avoidfactor;

    if (neighbors > 0) {
        // Applichiamo Alignment
        vel_avg_x /= neighbors;
        vel_avg_y /= neighbors;
        current->vx += (vel_avg_x - current->vx) * matchingfactor;
        current->vy += (vel_avg_y - current->vy) * matchingfactor;

        // Applichiamo Cohesion
        pos_avg_x /= neighbors;
        pos_avg_y /= neighbors;
        current->vx += (pos_avg_x - current->x) * centeringfactor;
        current->vy += (pos_avg_y - current->y) * centeringfactor;
    }
}

void draw_boids(Boid* boids, int num_boids) {
    float size = 8.0f;

    for (int i = 0; i < num_boids; i++) {

        float speed = sqrtf(boids[i].vx * boids[i].vx + boids[i].vy * boids[i].vy);

        float dx = (speed > 0) ? boids[i].vx / speed : 0;
        float dy = (speed > 0) ? boids[i].vy / speed : -1;

        // Punta del boid
        Vector2 head = { boids[i].x + dx * size, boids[i].y + dy * size };

        // Base del boid
        float side_x = -dy * (size / 2);
        float side_y = dx * (size / 2);

        Vector2 left  = { boids[i].x - dx * (size / 2) + side_x, boids[i].y - dy * (size / 2) + side_y };
        Vector2 right = { boids[i].x - dx * (size / 2) - side_x, boids[i].y - dy * (size / 2) - side_y };

        DrawTriangle(head, left, right, BLUE);
        DrawTriangleLines(head, left, right, DARKBLUE);
    }
}



