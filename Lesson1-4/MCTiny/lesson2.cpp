//
//  lesson2.cpp
//  MCTiny
//
//  Created by 苗超 on 2023/2/2.
//

#include "common.h"
#include "geometry.h"
#include "tgaimage.h"

extern Model *g_model;

// line sweep 方法需要用到lesson01的内容
extern void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color);

static void line(Vec2i v1, Vec2i v2, TGAImage &image, TGAColor color) {
    line(v1.x, v1.y, v2.x, v2.y, image, color);
}

static void triangle_full(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image) {
    // t0.y < t1.y < t2.y
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    
    line(t2, t0, image, red); // A
    line(t0, t1, image, green); // B
    line(t1, t2, image, blue); // C
}

static void triangle_bottom_half(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image) {
    
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!)
    if (t0.y>t1.y) std::swap(t0, t1);
    if (t0.y>t2.y) std::swap(t0, t2);
    if (t1.y>t2.y) std::swap(t1, t2);
    
    int total_height = t2.y-t0.y;
    for (int y=t0.y; y<=t1.y; y++) {
        int segment_height = t1.y-t0.y+1;
        float alpha = (float)(y-t0.y)/total_height;
        float beta  = (float)(y-t0.y)/segment_height; // be careful with divisions by zero
        Vec2i A = t0 + (t2-t0)*alpha;
        Vec2i B = t0 + (t1-t0)*beta;
        image.set(A.x, y, red);
        image.set(B.x, y, green);
    }
}


// Line sweeping
static void line_sweeping_full_triangle(Vec2i t0, Vec2i t1, Vec2i t2,
                                 TGAImage &image, TGAColor color) {
    if (t0.y==t1.y && t0.y==t2.y) return; // I dont care about degenerate triangles
    
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!)
    if (t0.y>t1.y) std::swap(t0, t1);
    if (t0.y>t2.y) std::swap(t0, t2);
    if (t1.y>t2.y) std::swap(t1, t2);
    
    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        // be careful: with above conditions no division by zero here
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height;
        
        Vec2i A = t0 + (t2-t0)*alpha;
        Vec2i B = second_half ? t1 + (t2-t1)*beta : t0 + (t1-t0)*beta;
        
        if (A.x>B.x) std::swap(A, B);
        
        for (int j=A.x; j<=B.x; j++) {
            // attention, due to int casts t0.y+i != A.y
            image.set(j, t0.y+i, color);
        }
    }
}


/// 计算重心坐标，主要是判断是否在三角形内
/// 以下方式是根据面积计算，所以，p坐标大于0才在三角形内
/// - Parameters:
///   - pts: 三角形的三个坐标
///   - p: 三角形所在平面内的点
Vec3f barycentric(Vec2i *pts, Vec2i p) {
    int xa = pts[0].x;
    int xb = pts[1].x;
    int xc = pts[2].x;
    
    int ya = pts[0].y;
    int yb = pts[1].y;
    int yc = pts[2].y;
    
    int x = p.x;
    int y = p.y;
    
    float gamma = static_cast<float>((ya - yb) * x + (xb - xa) * y + xa * yb - xb * ya) /
    static_cast<float>((ya - yb) * xc + (xb - xa) * yc + xa * yb - xb * ya);
    float beta = static_cast<float>((ya - yc) * x + (xc - xa) * y + xa * yc - xc * ya) / static_cast<float>((ya - yc) * xb + (xc - xa) * yb + xa * yc - xc * ya);
    float alpha = 1 - gamma - beta;
    
    return Vec3f(alpha, beta, gamma);
}


/// 获取三角形的包围盒，然后判断点是否在三角形内
static void pixel_inner_full_triangle(Vec2i *pts, TGAImage &image, TGAColor color) {
    Vec2i bboxmin(image.get_width()-1,  image.get_height()-1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width()-1, image.get_height()-1);
    
    /*
     包围盒模型渲染
     AABB: Axis-Aligned Bounding BOX
     */
    // 获得包围盒：左下角和右上角的两个点
    for (int i=0; i<3; i++) {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));
        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    
    // 依次遍历包围盒中的每个像素是否在三角形内
    Vec2i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) { // 以列方式遍历（先遍历行或列无关）
            Vec3f bc_screen  = barycentric(pts, P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) {
                continue;
            }
            image.set(P.x, P.y, color);
        }
    }
}


static void draw_triangle() {
    Vec2i t0[3] = {Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80)};
    Vec2i t1[3] = {Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180)};
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};
    
    {
        TGAImage image(200, 200, TGAImage::RGB);
        // 方式1：线扫描法
        line_sweeping_full_triangle(t0[0], t0[1], t0[2], image, red);
        line_sweeping_full_triangle(t1[0], t1[1], t1[2], image, green);
        line_sweeping_full_triangle(t2[0], t2[1], t2[2], image, blue);
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("lesson2_full_triangle_barycentric.tga");
    }
    
    {
        TGAImage image(200, 200, TGAImage::RGB);
        // 方式2：重心坐标法
        pixel_inner_full_triangle(t0, image, red);
        pixel_inner_full_triangle(t1, image, green);
        pixel_inner_full_triangle(t2, image, blue);
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("lesson2_full_triangle_line_sweep.tga");
    }
}

static void draw_model() {
    int width  = 800;
    int height = 800;
    TGAImage image(width, height, TGAImage::RGB);
    
    for (int i=0; i< g_model->nfaces(); i++) {
        std::vector<Vec3i> face = g_model->face(i); // 获取模型的第i个面
        Vec2i screenCoords[3]; //存贮第i个面三个顶点的屏幕坐标
        for (int j = 0; j < 3; j++) {
            // 分别取出第i个面三个顶点的世界坐标
            int vertexIndex = face[j][0];
            Vec3f worldCoord = g_model->vert(vertexIndex);
            // NDC: Normalized Device Coordiantes，标准化设备坐标
            //转换为屏幕坐标
            screenCoords[j] = Vec2i((worldCoord.x + 1.) * width / 2.,
                                    (worldCoord.y + 1.) * height / 2.);
        }
        // 随机给三角形填充颜色
        pixel_inner_full_triangle(screenCoords,
                                  image,
                                  TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));

    }
    
    image.flip_vertically();
    image.write_tga_file("lesson2_draw_model_colorful.tga");
}

/// lighting: lambert shading
/// frequency: flat shading
/// 只有漫反射，未考虑摄像机与反射光线夹角，因此无高光。
static void draw_light_model() {
    int width  = 800;
    int height = 800;
    TGAImage image(width, height, TGAImage::RGB);
    
    Vec3f light_dir(0, 0, -1); // 定义点光源: 从原点照向屏幕外（这里里外没区别）
    
    for (int i=0; i< g_model->nfaces(); i++) {
        std::vector<Vec3i> face = g_model->face(i);//获取模型的第i个面
        Vec2i screenCoords[3]; //存贮第i个面三个顶点的屏幕坐标
        Vec3f worldCoords[3];  //存储第i个面三个顶点的世界坐标
        
        for (int j = 0; j < 3; j++) {
            // 分别取出第i个面三个顶点的世界坐标
            int vertexIndex = face[j][0];
            worldCoords[j] = g_model->vert(vertexIndex);
            screenCoords[j] = Vec2i((worldCoords[j].x + 1.) * width / 2., (worldCoords[j].y + 1.) * height / 2.);//转换为屏幕坐标
        }
        
        /*
         lambert law: l = k·(I/r²)·max(0, n·l)
         - k: diffuse coefficient, 漫反射系数
         - I/r²: 物体到光源的距离
         - n·l: n: 平面法线; l: 入射方向
         */
        
        // 计算三角形法线: n
        Vec3f normal = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
        normal.normalize(); // 归一化
    
        // 对lmabert模型做简化: k=1，不考虑物体到光源的距离
        float intensity = normal * light_dir; // 已重载向量`*`运算
        
        // back-face culling
        // 简单的背面剔除只对凸面体有效， convex shapes only.
        // <0: 光源从物体背面入射
        if(intensity > 0) {
            // 可以认为是flat shading，整个三角形用同一个光照亮度和颜色
            pixel_inner_full_triangle(screenCoords,
                                      image,
                                      TGAColor(intensity * 255,
                                               intensity * 255,
                                               intensity * 255,
                                               255));
        }
    }
    
    image.flip_vertically();
    image.write_tga_file("lesson2_draw_model_lambert_light.tga");
}

void lesson2() {
    draw_triangle();
    draw_model();
    draw_light_model();
}
