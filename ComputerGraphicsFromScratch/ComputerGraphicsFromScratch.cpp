/**
Chapter 2: Basic Raytracing

Using a simple fixed camera setup, we add spheres in front of the camera and perform raytracing on each "pixel" of the Canvas.
Using each ray, we check against each sphere to see if there's an intersection. Intersections are checked against the common
solution of the sphere equation and the ray line equation. The results are values that give us the point based on the ray equation (O + tD).
If the discriminant is negative, we have no intersection. If the discriminant is positive and t1==t2, we have one intersection 
point (the ray is basically tangent to the sphere on the intersection point). When t1 != t2, then we have two intersection points, 
one is the entry and the other the exit point of the ray on the sphere. We compare all solutions to find the minimum, since that
will also be the closest intersection point to the ray origin (in this case where the camera is positioned). The sphere with the
closest intersection point, dictates the color we use to paint the "pixel". If no intersection exists, we use Black.

In this case, to emulate painting pixels, we create a raylib Image which is a CPU buffer and then use it to paint, then upload it to a Texture2D
in GPU land, which we then blit to the screen.
*/

#include "raylib_renderdoc.h"
#include <raylib.h>
#include <raymath.h>

#define CAMERA_ORIGIN_DISTANCE 1.0f
#define VIEWPORT_WIDTH 1.0f
#define VIEWPORT_HEIGHT 1.0f
#define CANVAS_WIDTH 800
#define CANVAS_HEIGHT 800

struct Vector2Int
{
    int x;
    int y;
};

struct Sphere
{
    Vector3 center;
    float radius;
    Color color;
};

struct RayIntersection
{
    float t1;
    float t2;
};

const Vector3 CAMERA_ORIGIN = { 0 };
const Vector3 CAMERA_LOOK_DIRECTION = { 0, 0, 1 };

Sphere objects[] = 
{
        Sphere{ Vector3{ 0.0f, -1.0f, 3.0f }, 0.1f , RED},
        Sphere{ Vector3{ 2.0f, 0.0f, 4.0f }, 0.1f , BLUE },
        Sphere{ Vector3{ -2.0f, 0.0f, 4.0f }, 0.1f , GREEN }
};

void SetPixel(Image* buf, int x, int y, Color c) {
    ImageDrawPixel(buf, x, y, c);
}

RayIntersection IntersectRaySphere(Ray R, Sphere sp)
{
    float r = sp.radius;
    Vector3 CO = Vector3Subtract(CAMERA_ORIGIN, sp.center);
    CO = Vector3Normalize(CO);

    float a = Vector3DotProduct(R.direction, R.direction);
    float b = 2.0f * Vector3DotProduct(CO, R.direction);
    float c = Vector3DotProduct(CO, CO) - r*r;

    float discriminant = (b * b) - (4.0f * a * c);
    if (discriminant < 0.0f)
    {
        return RayIntersection{ INFINITY, INFINITY };
    }

    RayIntersection collision;
    collision.t1 = (-b + sqrtf(discriminant)) / (2.0f * a);
    collision.t2 = (-b - sqrtf(discriminant)) / (2.0f * a);
    return collision;
}

Vector2Int CanvasToScreen(Vector2Int canvas_point) 
 {
    int sx = (CANVAS_WIDTH  / 2) + canvas_point.x;
    int sy = (CANVAS_HEIGHT / 2) - canvas_point.y;
    return Vector2Int{ sx, sy };
}

Vector3 CanvasToViewport(Vector2Int canvas_point) 
{
    float vx = (float)canvas_point.x * (VIEWPORT_WIDTH / CANVAS_WIDTH);
    float vy = (float)canvas_point.y * (VIEWPORT_HEIGHT / CANVAS_HEIGHT);
    return Vector3{ vx, vy, CAMERA_ORIGIN_DISTANCE };
}

Color TraceRay(Image* img, Ray r, float tmin, float tmax)
{
    float closest_t = INFINITY;
    Sphere* closest_sphere = NULL;
    for (int i = 0; i < 3; i++)
    {
        Sphere& sph = objects[i];
        RayIntersection collision = IntersectRaySphere(r, sph);

        if (collision.t1 != INFINITY
            && collision.t2 != INFINITY)
        {
            if (collision.t1 >= tmin
                && collision.t1 <= tmax
                && collision.t1 < closest_t)
            {
                closest_t = collision.t1;
                closest_sphere = &sph;
            }
            if (collision.t2 >= tmin
                && collision.t2 <= tmax
                && collision.t2 < closest_t)
            {
                closest_t = collision.t2;
                closest_sphere = &sph;
            }
        }
    }

    if (closest_sphere == NULL)
    {
        return BLACK;
    }
    return closest_sphere->color;
}

void DrawScene(Image* img)
{
    for (int x = -CANVAS_WIDTH / 2; x < CANVAS_WIDTH / 2; x++)
    {
        for (int y = -CANVAS_HEIGHT / 2; y < CANVAS_HEIGHT / 2; y++)
        {
            Vector2Int canvas_pos = { x,y };
            Vector3 viewport_pos = CanvasToViewport(canvas_pos);
            Vector3 ray_direction = Vector3Subtract(viewport_pos, CAMERA_ORIGIN);
            ray_direction = Vector3Normalize(ray_direction);
            Ray r = { CAMERA_ORIGIN, ray_direction };

            Color col = TraceRay(img, r, 1.0f, INFINITY);
            canvas_pos = CanvasToScreen(canvas_pos);
            SetPixel(img, canvas_pos.x, canvas_pos.y, col);
        }
    }    
}

int main(void)
{
    LoadRenderDoc();
    InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Computer Graphics from Scratch");

    Rectangle canvas_rect = { 0.0f, 0.0f, CANVAS_WIDTH, CANVAS_HEIGHT };
    Image img = GenImageColor(CANVAS_WIDTH, CANVAS_HEIGHT, BLACK); //CPU land
    Texture2D tex = LoadTextureFromImage(img); //GPU land

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = CAMERA_ORIGIN;
    camera.target = CAMERA_LOOK_DIRECTION;
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 53.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose())
    {
        if (RenderDocIsFrameCapturing())
        {
            RenderDocBeginFrameCapture();
        }

        {//Draw directly onto a texture
            ImageClearBackground(&img, BLACK);
            DrawScene(&img);           
            UpdateTexture(tex, img.data);             // Update GPU with new CPU data.
        }
        
        //Blitting the texture on screen using a rect
        BeginDrawing ();
        {
            ClearBackground(BLACK);
            DrawTextureRec(tex, canvas_rect, Vector2Zero(), WHITE); //white tint
        }
        EndDrawing();

        if (RenderDocIsFrameCapturing())
        {
            RenderDocEndFrameCapture();
        }
    }

    UnloadImage(img);         // Unload CPU texture copy
    UnloadTexture(tex);       // Unload GPU texture

    CloseWindow();

    UnloadRenderDoc();

    return 0;
}