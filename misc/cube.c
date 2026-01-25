#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#define WIDTH 66
#define HEIGHT 33

// #define

char buffer[HEIGHT][WIDTH];

static int origin_x = WIDTH/2;
static int origin_y = HEIGHT/2;

typedef float theta;
typedef struct p2{
    float x, y;
}p2;

void rot2d(p2* p, theta t){
    p2 temp;
    temp.x = p->x*cosf(t) - p->y*sinf(t);
    temp.y = p->x*sinf(t) + p->y*cosf(t);
    *p = temp;
}

typedef struct p3{
    float x, y, z;
}p3;

// void Rx(p3* p, theta t){
//     p3* temp;
//     temp->x = p->x;
//     temp->y = p->y*cosf(t) - p->z*sinf(t);
//     temp->z = p->y*sinf(t) + p->z*cosf(t);
//     p = temp;
// }

// void Ry(p3* p, theta t){
//     p3* temp;
//     temp->x = p->x*cosf(t) + p->z*sinf(t);
//     temp->y = p->y;
//     temp->z = -p->x*sinf(t) + p->z*cosf(t);
//     p = temp;
// }

// void Rz(p3* p, theta t){
//     p3* temp;
//     temp->x = p->x*cosf(t) - p->y*sinf(t);
//     temp->y = p->x*sinf(t) + p->y*cosf(t);
//     temp->z = p->z;
//     p = temp; 
// }

void Rx(p3* p, theta t){
    p3 temp;
    temp.x = p->x;
    temp.y = p->y*cosf(t) - p->z*sinf(t);
    temp.z = p->y*sinf(t) + p->z*cosf(t);
    *p = temp;  // copy back to original
}

void Ry(p3* p, theta t){
    p3 temp;
    temp.x = p->x*cosf(t) + p->z*sinf(t);
    temp.y = p->y;
    temp.z = -p->x*sinf(t) + p->z*cosf(t);
    *p = temp;
}

void Rz(p3* p, theta t){
    p3 temp;
    temp.x = p->x*cosf(t) - p->y*sinf(t);
    temp.y = p->x*sinf(t) + p->y*cosf(t);
    temp.z = p->z;
    *p = temp;
}


void print_window(){
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            printf("%c", buffer[i][j]);
        }
        printf("|\n");
    }
}

void set_dummy_buffer(char c){
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            buffer[i][j] = c;
        }
    }
}

void set(int x, int y, char c) {
    int bx = origin_x + x;
    int by = origin_y + y;

    if (bx < 0 || bx >= WIDTH || by < 0 || by >= HEIGHT)
        return;

    buffer[by][bx] = c;
}

typedef struct point_body{
    float pos_x, pos_y;
    float vel_x, vel_y;
    float dt;
} p_body;

void update(p_body *obj){
    obj->pos_x += obj->vel_x * obj->dt;
    obj->pos_y += obj->vel_y * obj->dt;
    if (obj->pos_x < -origin_x) {
        obj->pos_x = -origin_x;
        obj->vel_x *= -1;
    }
    if (obj->pos_x > origin_x - 1) {
        obj->pos_x = origin_x - 1;
        obj->vel_x *= -1;
    }
    if (obj->pos_y < -origin_y) {
        obj->pos_y = -origin_y;
        obj->vel_y *= -1;
    }
    if (obj->pos_y > origin_y - 1) {
        obj->pos_y = origin_y - 1;
        obj->vel_y *= -1;
    }
}

// static theta t = 0.0f;

void update_buffer(p_body *obj){
    float t = 0.0f;
    p2 test = {.x = obj->pos_x, .y = obj->pos_y};
    rot2d(&test, t);
    set((int)test.x, (int)test.y, '*');
    update(obj);
    t += 2.5f;
}

void rot_update(p2* point){
    float t = 0.0f;

    set((int)point->x*2, (int)point->y, '*');
    rot2d(point, t);
    t+= 3.0f;
}

// void draw_line(p2* start, p2* end){
//     float x1 = start->x, y1 = start->y;
//     set((int)x1*2,(int)y1, '*');
//     float x2 = end->x, y2 = end->y;
//     set((int)x2*2,(int)y2, '*');

//     float dx = x2 - x1; // x2>x1
//     float dy = y2 - y1;
//     float m = dy/dx;

//     for(float x = x1; x < x2; x += 0.5f){
//         float y = m * (x - x1) + y1;
//         set((int)x*2, (int)y, '*');
//     }
// }

void draw_line(p2 a, p2 b){
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    int steps = fabs(dx) > fabs(dy) ? fabs(dx*2) : fabs(dy);
    if(steps == 0) steps = 1;
    float x_inc = dx/steps;
    float y_inc = dy/steps;
    float x = a.x, y = a.y;
    for(int i=0;i<=steps;i++){
        set(x*2, y, '*'); // scale x for terminal
        x += x_inc;
        y += y_inc;
    }
}


void draw_rot(p2* start, p2* end, theta t){
    draw_line(*start, *end);
    rot2d(start, t);
    rot2d(end, t);
    // draw_line(start, end);
}

p2 project(p3 p){
    p2 out;
    out.x = p.x;          // just drop z
    out.y = p.y;
    return out;
}

// p2 project(p3 p){
//     float d = 10.0f;      // distance to camera
//     p2 out;
//     out.x = p.x / (d - p.z);
//     out.y = p.y / (d - p.z);
//     return out;
// }

int main(){
    float s = 10.0f;

    p3 cube[8] = {
        {-s, -s, -s}, { s, -s, -s},
        { s,  s, -s}, {-s,  s, -s},
        {-s, -s,  s}, { s, -s,  s},
        { s,  s,  s}, {-s,  s,  s}
    };
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // bottom
        {4,5},{5,6},{6,7},{7,4}, // top
        {0,4},{1,5},{2,6},{3,7}  // vertical
    };


    // bool draw = true;
    int frames = 600;
    printf("\033[2J");
    printf("\033[H");
    p_body test_body = {.pos_x = 0, .pos_y = 0, .vel_x = 1, .vel_y = 1, .dt = 0.06f};
    p2 pt = {.x = 6, .y = 0};
        // set_dummy_buffer(' ');

    p2 v1 = {.x =0, .y = 0};
    p2 v2 = {.x = 9, .y = 0};
    p2 v3 = {.x = 0, .y = 9};
    p2 v4 = {.x = 9, .y = 9};
    // while(frames--){
    //     printf("\033[2J");
    //     // printf("n1");
    //     set_dummy_buffer(' ');
    //     // set(0,0,'*');
    //     // update_buffer(&test_body);
    //     // draw_square(0,0,10,'*');
    //     // rot_update(&pt);
    //     // draw_line(&start, &end);
    //     // draw_rot(&start, &end, 180.0f);
        
    //     draw_rot(&v1, &v2, 10.0f);
    //     draw_rot(&v2, &v4, 10.0f);
    //     draw_rot(&v3, &v4, 10.0f);
    //     draw_rot(&v3, &v1, 10.0f);
    //     print_window();
    //     printf("\033[H");
    //     // if(frames == 0) break;
    //     // frames++;
    //     usleep(135000);
    //     // usleep(16000);
    // }
    // float t = 0.0f; // global rotation angle

    // while(frames--){
    //     printf("\033[2J");
    //     set_dummy_buffer(' ');

    //     // make rotated copies
    //     p2 rv1 = v1, rv2 = v2, rv3 = v3, rv4 = v4;
    //     rot2d(&rv1, t);
    //     rot2d(&rv2, t);
    //     rot2d(&rv3, t);
    //     rot2d(&rv4, t);

    //     // draw square edges
    //     draw_line(rv1, rv2);
    //     draw_line(rv2, rv4);
    //     draw_line(rv4, rv3);
    //     draw_line(rv3, rv1);

    //     print_window();
    //     printf("\033[H");

    //     t += 0.05f; // spin slowly
    //     usleep(135000);
    // }
    p2 cube_proj[8]; // 2D projected vertices for drawing
    float t = 0.0f;
    float a = 45.0f;
    while(frames--){
        set_dummy_buffer(' ');

        // rotate all vertices
        for(int i=0;i<8;i++){
            p3 v = cube[i];      // make copy, keep original cube intact
            Rx(&v, t*0.3f);
            // Ry(&v, t*0.5f);
            Ry(&v, a);
            Rz(&v, a);

            // Rz(&v, t*0.2f);
            cube_proj[i] = project(v);
        }

        // draw edges
        for(int i=0;i<12;i++){
            draw_line(cube_proj[edges[i][0]], cube_proj[edges[i][1]]);
        }

        print_window();
        printf("\033[H");
        t += 0.05f;
        usleep(15000);
    }

    return 0;
}