/**
3D Coordinate Space is Left Handed (Yup, X right, Z fwd)

*/

#include "3rdparty/renderdoc_app.h"
#include <raylib.h>
#include <raymath.h>
#include <queue>
#include "raylib_renderdoc.h"

#define CAMERA_ORIGIN_DISTANCE 1.0f
#define VIEWPORT_WIDTH 1.0f
#define VIEWPORT_HEIGHT 1.0f
#define CANVAS_WIDTH 800
//#define IFNINITY 10000000000.0f
#define CANVAS_HEIGHT 800

struct Vector2Int
{
    int x;
    int y;
};

//struct PointLight
//{
//    Vector3 position;
//    float intensity;
//};
//
//struct DirectionalLight
//{
//    Vector3 direction;
//    float intensity;
//};
//
//struct AmbientLight
//{
//    float intensity;
//};

struct Sphere
{
    Vector3 center;
    float radius;
    Color color;
};

struct RayCollision1
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
float ComputeLighting(Vector3 point) 
{
    return 0.0f;
}
RayCollision1 IntersectRaySphere(Ray R, Sphere sp)
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
        return RayCollision1{ INFINITY, INFINITY };
    }

    RayCollision1 collision;
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
        RayCollision1 collision = IntersectRaySphere(r, sph);

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


struct DrawTextNotification
{
    char text[100] = { 0 };
    uint32_t text_count;
    uint32_t frame_count = 0;
    uint32_t frame_duration = 10;
};

std::queue<DrawTextNotification> notifs;

int main(void)
{
    LoadRenderDoc();
    InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Computer Graphics from Scratch");

    RenderTexture2D rtd = LoadRenderTexture(CANVAS_WIDTH, CANVAS_HEIGHT); 
    Image img = GenImageColor(CANVAS_WIDTH, CANVAS_HEIGHT, BLACK); //CPU land
    Rectangle canvas_rect = { 0.0f, 0.0f, CANVAS_WIDTH, -CANVAS_HEIGHT };
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

        BeginTextureMode(rtd);
        {
            ClearBackground(BLACK);

            ImageClearBackground(&img, BLACK);
            
            {//Draw here
                DrawScene(&img);
            }
            
            UpdateTexture(tex, img.data);             // Update texture with new image data
            DrawTexture(tex, 0, 0, WHITE);            // Draw the updated texture in the render texture (basically upload to gpu)
        }
        EndTextureMode();
        
        //Blitting the render texture on screen.
        BeginDrawing ();
        {
            ClearBackground(BLACK);
            DrawTextureRec(rtd.texture, canvas_rect, Vector2(), WHITE); //white tint
            DrawText("Watermark", CANVAS_WIDTH - 60, CANVAS_HEIGHT - 10, 10, RED);

            if (!notifs.empty())
            {
                DrawTextNotification& notif = notifs.front();
                if (notif.frame_count <= notif.frame_duration)
                {
                    DrawText(notif.text, 10, 10, 10, RED);
                    notif.frame_count++;
                }
                else 
                {
                    notifs.pop();
                }
            }
        }
        EndDrawing();

        if (RenderDocIsFrameCapturing())
        {
            RenderDocEndFrameCapture();
        }
    }

    UnloadImage(img);         // Unload texture
    UnloadTexture(tex);
    UnloadRenderTexture(rtd); // Unload render texture

    CloseWindow();

    UnloadRenderDoc();

    return 0;
}