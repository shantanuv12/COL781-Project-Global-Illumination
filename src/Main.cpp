#include "./../include/Scene.h"
#include "./../include/Math.h"
#include "./../include/Sphere.h"
#include "./../include/Ray.h"
#include "./../include/Snowman.h"
#include <iostream>
#include <fstream>

using namespace std;

Sphere sp[] = {
    //Scene: radius, position, emission, color, material
    Sphere(1e5, vec3d(1e5 + 1, 40.8, 81.6), vec3d(), vec3d(.75, .25, .25), DIFFUSE),   //Left
    Sphere(1e5, vec3d(-1e5 + 99, 40.8, 81.6), vec3d(), vec3d(.25, .25, .75), DIFFUSE), //Rght
    Sphere(1e5, vec3d(50, 40.8, 1e5), vec3d(), vec3d(.75, .75, .75), DIFFUSE),         //Back
    Sphere(1e5, vec3d(50, 40.8, -1e5 + 170), vec3d(), vec3d(), DIFFUSE),               //Frnt
    Sphere(1e5, vec3d(50, 1e5, 81.6), vec3d(), vec3d(.75, .75, .75), DIFFUSE),         //Botm
    Sphere(1e5, vec3d(50, -1e5 + 81.6, 81.6), vec3d(), vec3d(.75, .75, .75), DIFFUSE), //Top
    Sphere(16.5, vec3d(27, 16.5, 47), vec3d(), vec3d(1, 1, 1) * .999, SPECULAR),       //Mirr
    Sphere(16.5, vec3d(73, 16.5, 78), vec3d(), vec3d(1, 1, 1) * .999, REFRACTION),     //Glas
    Sphere(600, vec3d(50, 681.6 - .27, 81.6), vec3d(12, 12, 12), vec3d(), DIFFUSE)     //Lite
};

void GImain(int argc, const char *argv[])
{
    int w = 1024, h = 768;
    int samples = 1;
    if (argc == 2)
    {
        samples = atoi(argv[1]) / 4;
    }

    Ray camera(vec3d(50, 52, 295.6), vec3d(0, -0.042612, -1).normalize());
    vec3d v = vec3d(0, -0.042612, -1).normalize();
    vec3d cx = vec3d(w * 0.5135 / h), cy = (cx.cross(camera.direction)).normalize() * 0.5135;
    vec3d r, *c = new vec3d[w * h];

    Scene myScene(sp, 9, vec3d(50, 60, 85), vec3d(M_PI * 10000, M_PI * 10000, M_PI * 10000), 0);

    // cout << "scene created" << endl;
#pragma omp parallel for schedule(dynamic, 1) private(r)
    for (int y = 0; y < h; y++)
    {
        fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samples * 4, 100. * y / (h - 1));
        for (unsigned short x = 0, Xi[3] = {0, 0, y * y * y}; x < w; x++)
        {
            //Subpixel 2 x 2
            for (int sy = 0, i = (h - y - 1) * w + x; sy < 2; sy++)
            {
                for (int sx = 0; sx < 2; sx++, r = vec3d())
                {
                    for (int s = 0; s < samples; s++)
                    {
                        double r1 = 2 * erand48(Xi);
                        double dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * erand48(Xi);
                        double dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);

                        vec3d d = cx * (((sx + 0.5 + dx) / 2 + x) / w - 0.5) + cy * (((sy + 0.5 + dy) / 2 + y) / h - 0.5) + camera.direction;

                        r = r + myScene.ray_tracer(Ray(camera.origin + d * 140, d.normalize()), 0, Xi) * (1.0 / samples);
                        // cout << "Ray tracer done" << endl;
                    }
                    c[i] = c[i] + vec3d(clamp(r.x), clamp(r.y), clamp(r.z)) * 0.25;
                }
            }
        }
    }
    fprintf(stderr, "\n");
    FILE *f = fopen("image.ppm", "w"); // Write image to PPM file.
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i = 0; i < w * h; i++)
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
}

void PMmain(int argc, const char *argv[])
{
    int w = 1024, h = 768;
    int samples = 1;
    int BKT = 1000;
    int estimate = 10;
    if (argc == 2)
    {
        samples = atoi(argv[1]) / BKT;
        if (samples == 0)
            samples = 1;
    }
    if (argc == 3)
    {
        estimate = atoi(argv[2]);
    }

    Scene myScene(sp, 9, vec3d(50, 60, 85), vec3d(M_PI * 10000, M_PI * 10000, M_PI * 10000), estimate);

    for (int i = 0; i < samples; i++)
    {
        int m = BKT * i;
        vec3d f;
        Ray r;
        fprintf(stderr, "\rPhoton Pass %5.2f%%", 100. * (i + 1) / samples);
        for (int j = 0; j < BKT; j++)
        {
            myScene.generatePhotonRay(&r, &f, m + j);
            myScene.photon_tracer(r, 0, false, f, m + j + 1);
        }
    }
    fprintf(stderr, "\n#photons:%ld\nBuilding kd-tree...\n", myScene.photons.size());
    if (!myScene.photons.empty())
        myScene.tree.build(&myScene.photons[0], myScene.photons.size());

    Ray camera(vec3d(50, 52, 295.6), vec3d(0, -0.042612, -1).normalize());
    vec3d v = vec3d(0, -0.042612, -1).normalize();
    vec3d cx = vec3d(w * 0.5135 / h), cy = (cx.cross(camera.direction)).normalize() * 0.5135;
    vec3d *c = new vec3d[w * h];
    // cout << "scene created" << endl;
#pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < h; y++)
    {
        fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samples * 4, 100. * y / (h - 1));
        for (int x = 0; x < w; x++)
        {
            //Subpixel 2 x 2
            const double s = 1.0 / (BKT * samples * 2 * 2);
            vec3d r(0), dmy(0);
            for (int sy = 0; sy < 2; sy++)
            {
                for (int sx = 0; sx < 2; sx++)
                {
                    vec3d d = cx * ((x + sx * 0.5 + 0.25) / w - 0.5) + cy * (-(y + sy * 0.5 + 0.25) / h + 0.5) + camera.direction;
                    r = r + myScene.photon_tracer(Ray(camera.origin + d * 140, d.normalize()), 0, true, dmy, 0);
                }
                c[x + y * w] = r * s;
            }
        }
    }
    fprintf(stderr, "\n");
    FILE *f = fopen("image.ppm", "w"); // Write image to PPM file.
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i = 0; i < w * h; i++)
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
}

int main(int argc, const char *argv[])
{
    //GImain(argc, argv);
    PMmain(argc, argv);
}