#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <set>

struct point {
    int x, y;

    point(int _x, int _y) {
        x = _x;
        y = _y;
    }
};

#define GRID 500
#define SEGS 300

struct seg {
    int x0;
    int y0;
    int x1;
    int y1;
    int xx, yy;

    int dx;
    int dy;
    int sx;
    int sy;
    int err;
    bool reversed;

    std::vector<point> points;

    seg(int _x0, int _y0, int _x1, int _y1) {
        if (_y1 >= _y0) {
            x0 = _x0;
            y0 = _y0;
            x1 = _x1;
            y1 = _y1;

            reversed = false;
        } else {
            x0 = _x1;
            y0 = _y1;
            x1 = _x0;
            y1 = _y0;

            reversed = true;
        }

        restart();
    }

    void restart() {
        xx = x0;
        yy = y0;

        dx = abs(x1 - x0);
        sx = (x0 < x1) ? 1 : -1;

        dy = abs(y1 - y0);
        sy = (y0 < y1) ? 1 : -1;

        err = ((dx > dy) ? dx : -dy) / 2;
    }

    void out(const char *op, std::vector<std::set<int> > &pixels, int id) {
        // printf("%d %d %s ", xx, yy, op);
        pixels[yy * GRID + xx].insert(id);
    }

    void dump(std::set<int> &s) {
        std::set<int>::iterator i;
        for (i = s.begin(); i != s.end(); ++i) {
            printf("%d,", *i);
        }
        printf(" ");
    }

    void docheck(bool ch, std::vector<std::set<int> > &pixels, int id, int &ox, int &oy, int xx, int yy) {
        if (!ch) {
            return;
        }

        int oi = oy * GRID + ox;
        int ni = yy * GRID + xx;

        if (pixels[oi] != pixels[ni]) {
#if 0
            printf("not the same: ");
            dump(pixels[oi]);
            dump(pixels[ni]);
            printf("\n");
#endif

            // Leaving a shared segment or changing how shared
            if (pixels[oi].size() > 1) {
                printf("%d %d lineto ", ox, oy);
                points.push_back(point(ox, oy));
            }
            // Entering a shared segment or changing how shared
            if (pixels[ni].size() > 1) {
                printf("%d %d lineto ", xx, yy);
                points.push_back(point(xx, yy));
            }
        } else {
#if 0
            printf("the same: ");
            dump(pixels[oi]);
            dump(pixels[oi]);
            printf("\n");
#endif
        }

        ox = xx;
        oy = yy;
    }

    void run(std::vector<std::set<int> > &pixels, int id, bool check) {
        out("moveto", pixels, id);
        int ox = xx, oy = yy;

        if (check) {
            printf("%d %d moveto ", xx, yy);
            points.push_back(point(xx, yy));
        }

        while (xx != x1 || yy != y1) {
            int e2 = err;
            bool across = false, down = false;

            if (e2 > -dx) {
                err -= dy;
                across = true;
            }

            if (e2 < dy) {
                err += dx;
                down = true;
            }

            if (across && down) {
                xx += sx;

                if (sx < 0) {
                    out("lineto", pixels, id);
                    docheck(check, pixels, id, ox, oy, xx, yy);
                }

                yy += sy;
                out("lineto", pixels, id);
                docheck(check, pixels, id, ox, oy, xx, yy);
            } else if (across) {
                xx += sx;
                out("lineto", pixels, id);
                docheck(check, pixels, id, ox, oy, xx, yy);
            } else if (down) {
                yy += sy;
                out("lineto", pixels, id);
                docheck(check, pixels, id, ox, oy, xx, yy);
            }
        }

        if (check) {
            printf("%d %d lineto ", xx, yy);
            points.push_back(point(xx, yy));
        }

        printf("stroke\n");
    }
};

// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
static bool get_line_intersection(point p0, point p1, point p2, point p3, point &out) {
    double s1_x = p1.x - p0.x;
    double s1_y = p1.y - p0.y;
    double s2_x = p3.x - p2.x;
    double s2_y = p3.y - p2.y;

    double denom = -s2_x * s1_y + s1_x * s2_y;
    if (denom == 0) {
        // collinear
        return false;
    }

    double s, t;
    s = (-s1_y * (p0.x - p2.x) + s1_x * (p0.y - p2.y)) / denom;
    if (s <= 0 || s >= 1) {
        return false;
    }

    t = (s2_x * (p0.y - p2.y) - s2_y * (p0.x - p2.x)) / denom;
    if (t <= 0 || t >= 1) {
        return false;
    }

    if (s > 0 && s < 1 && t > 0 && t < 1) {
        out = point(p0.x + (t * s1_x), p0.y + (t * s1_y));
        return true;
    } else {
        printf("got here with %f %f\n", s, t);
        return false;
    }
}

int main(int argc, char **argv) {
    bool check = false;

    {
        int i;
        extern int optind;
        while ((i = getopt(argc, argv, "c")) != -1) {
            switch (i) {
                case 'c':
                    check = true;
                    break;
            }
        }
    }

    srand(1);
    std::vector<seg> segs;
    std::vector<std::set<int> > pixels;
    pixels.resize(GRID * GRID);

    printf("0 setlinewidth\n");
    printf("1 1 0 setrgbcolor\n");

    for (size_t i = 0; i < SEGS; i++) {
        int x0 = rand() % GRID;
        int y0 = rand() % GRID;
        int x1 = rand() % GRID;
        int y1 = rand() % GRID;

        segs.push_back(seg(x0, y0, x1, y1));
        printf("%d %d moveto %d %d lineto stroke\n", segs[i].x0, segs[i].y0, segs[i].x1, segs[i].y1);
        segs[i].run(pixels, i, false);
    }

    printf("0 setgray\n");

    std::vector<seg> cut;

    for (size_t i = 0; i < SEGS; i++) {
        segs[i].restart();
        segs[i].run(pixels, i, true);

        for (size_t j = 0; j + 1 < segs[i].points.size(); j++) {
            if (segs[i].points[j].x != segs[i].points[j + 1].x || segs[i].points[j].y != segs[i].points[j + 1].y) {
                cut.push_back(seg(segs[i].points[j].x, segs[i].points[j].y, segs[i].points[j + 1].x, segs[i].points[j + 1].y));
                // printf("%d,%d %d,%d xxx\n", segs[i].points[j].x, segs[i].points[j].y, segs[i].points[j + 1].x, segs[i].points[j + 1].y);
            }
        }
    }

    if (check) {
        for (size_t i = 0; i < cut.size(); i++) {
            for (size_t j = i + 1; j < cut.size(); j++) {
                point p(0, 0);
                if (get_line_intersection(point(cut[i].x0, cut[i].y0), point(cut[i].x1, cut[i].y1),
                                          point(cut[j].x0, cut[j].y0), point(cut[j].x1, cut[j].y1), p)) {
                    printf("%d,%d to %d,%d vs %d,%d %d,%d %lu and %lu at %d,%d\n",
                        cut[i].x0, cut[i].y0, cut[i].x1, cut[i].y1,
                        cut[j].x0, cut[j].y0, cut[j].x1, cut[j].y1, i, j, p.x, p.y);
                }
            }
        }
    }
}
