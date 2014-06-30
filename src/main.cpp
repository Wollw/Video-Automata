#include <iostream>
#include <cv.h>
#include <highgui.h>
//#include <gflags/gflags.h>

#define THREAD_COUNT 8

using namespace std;
using namespace cv;

typedef Vec3b Color;
typedef vector<Mat> World;
typedef Color (*Rule)(Color c, const vector<Color> &);
typedef struct {
    unsigned int thread_id;
    unsigned int thread_count;
    World *src;
    World *dst;
    Rule r;
} thread_data;

void set_color(World &w, int x, int y, int t, Color c);
Color get_color(const World &w, int x, int y, int t);
void *update_thread(void *data);
void update(World *src, World *dst, Rule r);
Color my_rule(Color c, const vector<Color> &ns);

/*** Customize this rule ***/
Color my_rule(Color c, const vector<Color> &cs) {
    float r = 0;
    float g = 0;
    float b = 0;
    for (int i = 0; i < (int)cs.size(); i++) {
        r += cs[i][2];
        g += cs[i][1];
        b += cs[i][0];
    }
    r /= cs.size();
    g /= cs.size();
    b /= cs.size();
    c[0] = b;
    c[1] = g;
    c[2] = r;
    return c;
}
/***************************/

void set_color(World &w, int x, int y, int t, Color c) {
    Mat m = w.at(t);
    m.at<Vec3b>(y,x) = c;
}

Color get_color(const World &w, int x, int y, int t) {
    Mat m = w.at(t);
    return m.at<Vec3b>(y,x);
}

void *update_thread(void *data) {
    thread_data *d = (thread_data*)data;

    World *src = d->src;
    World *dst = d->dst;
    Rule r = d->r;

    int x_min = 0;
    int y_min = 0;
    int t_min = 0;
    int x_max = src->at(0).cols-1;
    int y_max = src->at(0).rows-1;
    int t_max = src->size()-1;
    for (int t = d->thread_id; t < (int)src->size(); t+=d->thread_count) {
        cout << ".";
        for (int y = 0; y < src->at(t).rows; y++)
        for (int x = 0; x < src->at(t).cols; x++) {
            Color c = get_color(*src, x, y, t);
            vector<Color> neighbors;
            for (int dt = -1; dt <= 1; dt++) {
                if (t+dt >= t_min && t+dt <= t_max) {
                    if (x-1 >= x_min) neighbors.push_back(get_color(*src,x-1,y,t+dt));
                    if (x+1 <= x_max) neighbors.push_back(get_color(*src,x+1,y,t+dt));
                    if (y-1 >= y_min) neighbors.push_back(get_color(*src,x,y-1,t+dt));
                    if (y+1 <= y_max) neighbors.push_back(get_color(*src,x,y+1,t+dt));
                    if (x-1 >= x_min && y-1 >= y_min) neighbors.push_back(get_color(*src,x-1,y-1,t+dt));
                    if (x+1 <= x_max && y-1 >= y_min) neighbors.push_back(get_color(*src,x+1,y-1,t+dt));
                    if (x-1 >= x_min && y+1 <= y_max) neighbors.push_back(get_color(*src,x-1,y+1,t+dt));
                    if (x+1 <= x_max && y+1 <= y_max) neighbors.push_back(get_color(*src,x+1,y+1,t+dt));
                    if (dt != 0) neighbors.push_back(get_color(*src,x,y,t+dt));
                }
            }
            set_color(*dst, x, y, t, r(c, neighbors));
        }
    }

    return NULL;
}

void update(World *src, World *dst, Rule r) {
    pthread_t threads[THREAD_COUNT];
    int irets[THREAD_COUNT];
    thread_data data[THREAD_COUNT];

    for (unsigned int i = 0; i < THREAD_COUNT; i++) {
        data[i] = thread_data{i, THREAD_COUNT, src, dst, my_rule};
        irets[i] = pthread_create(&threads[i], NULL, update_thread, &data);
        if (irets[i]) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",irets[i]);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);

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

    VideoWriter vw;
    int ex = CV_FOURCC('I', 'Y', 'U', 'V');
    vw.open("out.avi", ex, vc.get(CV_CAP_PROP_FPS), f.size());

    vector<Mat> *src = &framesFront;
    vector<Mat> *dst = &framesBack;
    for (int i = 0; i < vc.get(CV_CAP_PROP_FRAME_COUNT); i++) {
        cout << "Frame " << i;
        update(src, dst, my_rule);
        swap(src, dst);
        cout << "writing...";
        vw.write(dst->at(i));
        cout << "done." << endl;
    }

    return 0;
}
