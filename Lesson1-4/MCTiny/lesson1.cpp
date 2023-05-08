//
//  lesson1.cpp
//  MCTiny
//
//  Created by 苗超 on 2023/2/2.
//

#include <vector>
#include <cmath>
#include "common.h"

extern Model *g_model;

static void frist_draw_line(int x0, int y0,
                     int x1, int y1,
                     TGAImage &image, TGAColor color) {
    
    float step = 0.01; // 控制步长
    /*
     所有绘制次数 = 1/step = 100
     但一共需要绘制：x1 - x0个像素
     效率低！
     */
    for (float t=0.0; t<1.0; t+=step) {
        int x = x0 * (1.0-t) + x1 * t;
        int y = y0 * (1.0-t) + y1 * t;
        image.set(x, y, color);
    }
}

static void second_draw_line(int x0, int y0,
                      int x1, int y1,
                      TGAImage &image, TGAColor color) {
    // 只能处理x1 > x0的情况
    for (int x = x0; x <= x1; x++) {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
        image.set(x, y, color);
    }
}

// Bresenham's algorithm
static void final_draw_line(int x0, int y0,
                     int x1, int y1,
                     TGAImage &image, TGAColor color) {
    bool steep = false; // 标记位: 斜率的绝对值是否大于1
    
    if (std::abs(x0-x1)<std::abs(y0-y1)) {
        steep = true;
        // steep > 1: 对调x,y坐标
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    
    if (x0>x1) { // x0 > x1: 对调端点坐标
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    int dx = x1-x0;
    int dy = y1-y0;
    int derror2 = std::abs(dy)*2;
    int error2 = 0;
    int y = y0;
    for (int x=x0; x<=x1; x++) {
        if (steep) {// steep > 1: 还原对调后的坐标
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1>y0?1:-1);
            error2 -= dx*2;
        }
    }
}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    final_draw_line(x0, y0, x1, y1, image, color);
}

static void draw_line() {
    {
        TGAImage image(100, 100, TGAImage::RGB);
        // (12, 20) -> (80, 40)
        frist_draw_line(12, 20, 80, 40, image, white);
        
        
        // (12, 25) -> (80, 45)
        second_draw_line(12, 25, 80, 45, image, white);
        // (25, 12) -> (45, 80)
        second_draw_line(20, 13, 45, 80 , image, red );
        // (80, 45) -> (12, 25)
        second_draw_line(80, 45, 12, 25, image, blue);
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("lesson1_draw_line_simple.tga");
    }
    
    {
        TGAImage image(100, 100, TGAImage::RGB);
        // (12, 20) -> (80, 40)
        final_draw_line(12, 20, 80, 40, image, white);
        // (12, 25) -> (80, 45)
        final_draw_line(12, 25, 80, 45, image, white);
        // (25, 12) -> (45, 80)
        final_draw_line(20, 13, 45, 80 , image, red );
        // (80, 45) -> (12, 25)
        final_draw_line(80, 45, 12, 25, image, blue);
        
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("lesson1_draw_line_bresenham.tga");
    }
}

static void draw_model() {
    int width  = 800;
    int height = 800;
    TGAImage image(width, height, TGAImage::RGB);
    
    for (int i=0; i< g_model->nfaces(); i++) {
        
        std::vector<Vec3i> face = g_model->face(i); // 获取模型的第i个面
        for (int j=0; j<3; j++) {
            /*
             获取顶点顺序
             i=0: A,B
             i=1: B,C
             i=2: C,A
             */
            int vertextAIndex = face[j][0];
            int vertextBIndex = face[(j+1)%3][0];
            
            Vec3f v0 = g_model->vert(vertextAIndex);
            Vec3f v1 = g_model->vert(vertextBIndex);
            
            /*
             根据顶点v0和v1画线
             先要进行模型坐标到屏幕坐标的转换
             (-1,-1)对应(0,0), (1,1)对应(width,height)
             */
            int x0 = (v0.x+1.)*width/2.;
            int y0 = (v0.y+1.)*height/2.;
            int x1 = (v1.x+1.)*width/2.;
            int y1 = (v1.y+1.)*height/2.;
        
            final_draw_line(x0, y0, x1, y1, image, white);
        }
    }
    
    image.flip_vertically();
    image.write_tga_file("lesson1_draw_model.tga");
}

void lesson1() {
    draw_line();
    draw_model();
}
