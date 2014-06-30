#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <unistd.h>

using namespace std;
using namespace cv;

typedef Vec3b Color;
typedef vector<Mat> World;
typedef Color (*Rule)(Color, const vector<Color> &);
typedef struct {
    unsigned int thread_id;
    unsigned int thread_count;
    World *src;
    World *dst;
    Rule r;
} thread_data;

void set_color(World &, int x, int y, int t, Color);
Color get_color(const World &, int x, int y, int t);
void *update_thread(void *);
void update(World *src, World *dst, Rule, unsigned int thread_count);
Color my_rule(Color, const vector<Color> &);

/*** Customize this rule ***/
Color my_rule(Color c, const vector<Color> &cs) {
    float b = 0;
    float g = 0;
    float r = 0;
    for (int i = 0; i < (int)cs.size(); i++) {
        b += cs[i][0];
        g += cs[i][1];
        r += cs[i][2];
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
        for (int y = 0; y < src->at(t).rows; y++) {
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
    }

    return NULL;
}

void update(World *src, World *dst, Rule r, unsigned int thread_count) {
    pthread_t threads[thread_count];
    int irets[thread_count];
    thread_data data[thread_count];

    for (unsigned int i = 0; i < thread_count; i++) {
        data[i] = thread_data{i, thread_count, src, dst, my_rule};
        irets[i] = pthread_create(&threads[i], NULL, update_thread, &data[i]);
        if (irets[i]) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",irets[i]);
            exit(EXIT_FAILURE);
        }
    }

    for (unsigned int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);

    int tvalue  = 1;
    char *ivalue = NULL;
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "t:i:")) != -1) {
        switch (c) {
            case 't':
                tvalue = atoi(optarg);
                break;
            case 'i':
                ivalue = optarg;
                break;
            case '?':
                if (optopt == 'i')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if(isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return 1;
            default:
                abort();
        }
    }

    cout << argc - optind << endl;
    char usage[] = "usage: %s -i inputfile [-t threadcount] outputfile\n";
    if (ivalue == NULL || argc - optind != 1) {
        fprintf(stderr, usage, argv[0]);
        return 1;
    }


    VideoCapture vc = VideoCapture(ivalue);
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
    vw.open(argv[optind], ex, vc.get(CV_CAP_PROP_FPS), f.size());

    vector<Mat> *src = &framesFront;
    vector<Mat> *dst = &framesBack;
    cout << "Frame 1...";
    vw.write(dst->at(0));
    cout << "done." << endl;
    for (int i = 1; i < vc.get(CV_CAP_PROP_FRAME_COUNT); i++) {
        cout << "Frame " << i+1;
        update(src, dst, my_rule, tvalue);
        cout << "writing...";
        vw.write(dst->at(i));
        swap(src, dst);
        cout << "done." << endl;
    }

    return 0;
}
