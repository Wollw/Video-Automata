#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <gflags/gflags.h>

using namespace std;
using namespace cv;

typedef Vec3b Color;
typedef vector<Mat> World;
typedef Color (*Rule)(const vector<Color> &);

void set_color(World &w, int x, int y, int t, Color c) {
    Mat m = w.at(t);
    m.at<Vec3b>(y,x) = c;
}

Color get_color(const World &w, int x, int y, int t) {
    Mat m = w.at(t);
    return m.at<Vec3b>(y,x);
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
    float r = 0;
    float g = 0;
    float b = 0;
    for (int i = 0; i < cs.size(); i++) {
        r += cs[i][2];
        g += cs[i][1];
        b += cs[i][0];
    }
    r /= cs.size();
    g /= cs.size();
    b /= cs.size();
    Color c;
    c[0] = b;
    c[1] = g;
    c[2] = r;
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
