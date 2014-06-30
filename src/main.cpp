#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <gflags/gflags.h>

using namespace std;
using namespace cv;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;
typedef vector<Mat> World;
typedef Color (*Rule)(const vector<Color> &);

void set_color(World &w, int x, int y, int t, Color c) {
    Mat m = w.at(t);
    Vec3b cvec;
    m.at<Vec3b>(y,x)[0] = c.b;
    m.at<Vec3b>(y,x)[1] = c.g;
    m.at<Vec3b>(y,x)[2] = c.r;
}

Color get_color(const World &w, int x, int y, int t) {
    Mat m = w.at(t);
    Color c;
    c.b = m.at<Vec3b>(y,x)[0];
    c.g = m.at<Vec3b>(y,x)[1];
    c.r = m.at<Vec3b>(y,x)[2];
    return c;
}

void update(World &src, World &dst, Rule r) {
    int x_min = 0;
    int y_min = 0;
    int t_min = 0;
    int x_max = src.at(0).cols-1;
    int y_max = src.at(0).rows-1;
    int t_max = src.size()-1;
    for (int t = 0; t < src.size(); t++) {
        cout << "Frame " << t << "..." << endl;
        for (int y = 0; y < src.at(t).rows; y++)
        for (int x = 0; x < src.at(t).cols; x++) {
            Color c = get_color(src, x, y, t);
            vector<Color> neighbors;
            if (x-1 >= x_min) neighbors.push_back(get_color(src,x-1,y,t));
            if (x+1 <= x_max) neighbors.push_back(get_color(src,x+1,y,t));
            if (y-1 >= y_min) neighbors.push_back(get_color(src,x,y-1,t));
            if (y+1 <= y_max) neighbors.push_back(get_color(src,x,y+1,t));
            if (x-1 >= x_min && y-1 >= y_min) neighbors.push_back(get_color(src,x-1,y-1,t));
            if (x+1 <= x_max && y-1 >= y_min) neighbors.push_back(get_color(src,x+1,y-1,t));
            if (x-1 >= x_min && y+1 <= y_max) neighbors.push_back(get_color(src,x-1,y+1,t));
            if (x+1 <= x_max && y+1 <= y_max) neighbors.push_back(get_color(src,x+1,y+1,t));
            set_color(dst, x, y, t, r(neighbors));
            //set_color(dst, x, y, t, c);
        }
        imshow("WINDOW", dst.at(t));
        if (waitKey(1) == '')
            return;
    }
}

Color my_rule(const vector<Color> &cs) {
    Color c = {0,0,0};
    for (int i = 0; i < cs.size(); i++) {
        c.r += cs[i].r;
        c.g += cs[i].g;
        c.b += cs[i].b;
    }
    c.r /= cs.size();
    c.g /= cs.size();
    c.b /= cs.size();
    return c;
}

int main(int argc, char **argv) {
    VideoCapture vc = VideoCapture(argv[1]);
    if (!vc.isOpened())
        return 1;

    vector<Mat> framesFront, framesBack;

    Mat f;
    for (int i = 0; i < vc.get(CV_CAP_PROP_FRAME_COUNT); i++) {
        cout << "Frame " << i << "..." << endl;
        vc >> f;
        framesFront.push_back(f.clone());
        framesBack.push_back(f.clone());
    }

    update(framesFront, framesBack, my_rule);

    namedWindow("WINDOW", CV_WINDOW_NORMAL);
    setWindowProperty("WINDOW", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    for (int i = 0; i < framesBack.size(); i++) {
        imshow("WINDOW", framesBack.at(i));
        if (waitKey(1) == '')
            return 0;
    }

    return 0;
}

/*
 * data Automaton = Automaton World Rule
 * type World = [Cells]
 * type Rule = World -> World
 */
