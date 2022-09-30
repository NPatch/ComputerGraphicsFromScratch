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

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

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

enum LightType 
{
    POINT,
    DIRECTIONAL,
    AMBIENT
};

struct Light
{
    Vector3 position;
    Vector3 direction;
    float intensity;
    LightType type;
};

const Vector3 CAMERA_ORIGIN = { 0 };
const Vector3 CAMERA_LOOK_DIRECTION = { 0, 0, 1 };

Sphere objects[] = 
{
        Sphere{ Vector3{ 0.0f, -1.0f, 3.0f }, 1.0f , RED},
        Sphere{ Vector3{ 2.0f, 0.0f, 4.0f }, 1.0f , BLUE },
        Sphere{ Vector3{ -2.0f, 0.0f, 4.0f }, 1.0f , GREEN },
        Sphere{ Vector3{ 0.0f, -5001.0f, 0.0f }, 5000.0f , YELLOW }
};


Light lights[] =
{
        Light{ Vector3{2.0f, 1.0f, 0.0f }, Vector3{  0.0f, -1.0f, 3.0f },  0.6f , LightType::POINT},
        Light{ Vector3{0.0f, 0.0f, 0.0f }, Vector3{  1.0f,  4.0f, 4.0f },  0.2f , LightType::DIRECTIONAL },
        Light{ Vector3{0.0f, 0.0f, 0.0f }, Vector3{  0.0f,  0.0f, 0.0f },  0.2f , LightType::AMBIENT}
};

Vector4 Vector4Scale(Vector4 v, float scalar)
{
    Vector4 result = { v.x * scalar, v.y * scalar, v.z * scalar , v.w /** scalar*/};
    return result;
}

Color ColorMultiply(Color col, float factor) 
{
    Color c = {
        (unsigned char)(Clamp((float)col.r * factor,0.0f, 255.0f)),
        (unsigned char)(Clamp((float)col.g * factor,0.0f, 255.0f)),
        (unsigned char)(Clamp((float)col.b * factor,0.0f, 255.0f)),
        255
    };
    return c;
}

void SetPixel(Image* buf, int x, int y, Color c) {
    ImageDrawPixel(buf, x, y, c);
}

RayIntersection IntersectRaySphere(Ray R, Sphere sp)
{
    float r = sp.radius;
    Vector3 CO = Vector3Subtract(CAMERA_ORIGIN, sp.center);

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

float ComputeLighting(Vector3 point, Vector3 sphere_center) 
{
    Vector3 normal = Vector3Subtract(point, sphere_center);

    float light_contrib = 0.0f;
    for (int i=0;i< COUNT_OF(lights);i++)
    {
        Light& l = lights[i];

        Vector3 beam_dir = Vector3Zero();

        if (l.type == LightType::AMBIENT) 
        {
            light_contrib += l.intensity;
        }
        else {
            if (l.type == LightType::POINT)
            {
                beam_dir = Vector3Subtract(l.position, point);
            }
            else if (l.type == LightType::DIRECTIONAL)
            {
                beam_dir = l.direction;
            }

            float n_to_l = Vector3DotProduct(normal, beam_dir);
            float coefficient = n_to_l / (Vector3Length(normal) * Vector3Length(beam_dir));
            light_contrib += l.intensity * coefficient;
        }
    }
    
    return light_contrib;
}

Color TraceRay(Image* img, Ray r, float tmin, float tmax)
{
    float closest_t = INFINITY;
    Sphere* closest_sphere = NULL;
    for (int i = 0; i < COUNT_OF(objects); i++)
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
        return WHITE;
    }

    Vector3 intersection_point = Vector3Add(r.position, Vector3Scale(r.direction, closest_t));
    float light_contrib = ComputeLighting(intersection_point, closest_sphere->center);

    return ColorMultiply(closest_sphere->color, light_contrib);
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
    Image img = GenImageColor(CANVAS_WIDTH, CANVAS_HEIGHT, WHITE); //CPU land
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
            DrawScene(&img);           
            UpdateTexture(tex, img.data);             // Update GPU with new CPU data.
        }
        
        //Blitting the texture on screen using a rect
        BeginDrawing ();
        {
            DrawTextureRec(tex, canvas_rect, Vector2Zero(), WHITE); //white tint
            DrawFPS(10, 10);
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