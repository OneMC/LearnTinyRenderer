//
//  lesson3.cpp
//  MCTiny
//
//  Created by 苗超 on 2023/2/6.
//

#include "common.h"
extern Model *g_model;
/*
 face culling: z-buffer
 1. color/famee buffer: 存放每个像素的颜色
 2. depth/Z     buffer: 存放每个像素距离相机最近的深度值
 
 比较每个像素的深度值，如果距离相机更近，
 则将该像素的颜色写入frame buffer，深度写入depth buffer
 */

static int width  = 800;
static int height = 800;

/// 将重心坐标计算改为float型
Vec3f barycentric(Vec3f* pts, Vec3f P) {
    float xa = pts[0].x;
    float ya = pts[0].y;
    float xb = pts[1].x;
    float yb = pts[1].y;
    float xc = pts[2].x;
    float yc = pts[2].y;
    float x = P.x;
    float y = P.y;

    float gamma = ((ya - yb) * x + (xb - xa) * y + xa * yb - xb * ya) / ((ya - yb) * xc + (xb - xa) * yc + xa * yb - xb * ya);
    float beta = ((ya - yc) * x + (xc - xa) * y + xa * yc - xc * ya) / ((ya - yc) * xb + (xc - xa) * yb + xa * yc - xc * ya);
    float alpha = 1 - gamma - beta;
    
    return Vec3f(alpha, beta, gamma);
}


/// 世界坐标到屏幕坐标
/// - Parameter v: 世界坐标
Vec3f world2screen(Vec3f v) {
    /*
     因为代码使用模型的坐标已经归一化，而且模型就直接“压缩”在X-Y平面上，所以直接如下转为为屏幕坐标即可
     (-1.0, 1.0) -+1-> (0, 2.0) -/2-> (0, 1.0) - *(width,height) -> screen coordinate
     */
    float x = int((v.x+1.) * width/2.);
    float y = int((v.y+1.) * height / 2.);
    return Vec3f(x, y, v.z);
}

/// 根据z-buffer进行back-face culling
/// - Parameters:
///   - pts: 三角形顶点
///   - zbuffer: zbuffer缓存，与屏幕尺寸相同
///   - image: 图像
///   - color: 颜色
static void triangle(Vec3f *pts, float *zBuffer, TGAImage &image, TGAColor color) {
    
    // get axis aligned bounding box
    float minx = std::min({pts[0].x, pts[1].x, pts[2].x });
    float maxx = std::max({pts[0].x, pts[1].x, pts[2].x });
    float miny = std::min({pts[0].y, pts[1].y, pts[2].y });
    float maxy = std::max({pts[0].y, pts[1].y, pts[2].y });
    
    int min_x = (int)std::floor(minx);
    int max_x = (int)std::floor(maxx);
    int min_y = (int)std::floor(miny);
    int max_y = (int)std::floor(maxy);
    
    // world2screen后，在float-> int会出现数组越界
    if (max_x >= width) max_x = width - 1;
    if (max_y >= height) max_y = height - 1;
    
    /*
     遍历包围盒中的每个像素
     这里可以认为是正交投影，将3D空间的内容直接“压扁”到X-Y平面
     模型中z坐标的值就是其深度值，既z-value
     */
    for (int i = min_x; i <= max_x; i++) {
        for (int j = min_y; j <= max_y; j++) {
            
            Vec3f P(i, j, 0);
            Vec3f baryCoord = barycentric(pts, P);
            
            if (baryCoord.x < -0.01 ||
                baryCoord.y < -0.01 ||
                baryCoord.z < -0.01) {
                continue; // 如果像素再三角形外则跳过
            }
                
            /*
             重心坐标插值： V = αV₁ + βV₂ + γV₃
             V可以是任意属性，这里是z-buffer
             */
            float z_interpolation =
            baryCoord.x * pts[0].z +
            baryCoord.y * pts[1].z +
            baryCoord.z * pts[2].z;
            
            // 因为zbuffer是一维数组，所以(x,y)->z_value: P.x + P.y * width
            int zbufferIndex = int(P.x + P.y * width);
            if (z_interpolation > zBuffer[zbufferIndex]) {
                zBuffer[zbufferIndex] = z_interpolation;
                image.set(P.x, P.y, color);
            }
        }
    }
}

/// lighting: lambert shading
/// frequency: flat shading
/// back-face culling: z-buffer
static void draw_face_culling_model() {
    TGAImage image(width, height, TGAImage::RGB);
    
    // 定义屏幕尺寸相同大小的z-buffer
    int zBufferSize = width*height;
    float *zBuffer = new float[zBufferSize];
    for (int i=0; i<zBufferSize; i++) {
        // 初始化z-buffer为-∞
        zBuffer[i] = -std::numeric_limits<float>::max();
    }
    
    Vec3f light_dir(0, 0, -1); // 定义点光源: 从原点照向屏幕外（这里里外没区别）
    
    for (int i=0; i< g_model->nfaces(); i++) {
        Vec3f screenCoords[3]; //存贮第i个面三个顶点的屏幕坐标
        Vec3f worldCoords[3];  //存储第i个面三个顶点的世界坐标
        
        std::vector<Vec3i> face = g_model->face(i);//获取模型的第i个面
        for (int j = 0; j < 3; j++) {
            // 分别取出第i个面三个顶点的世界坐标
            int vertexIndex = face[j][0];
            Vec3f v = g_model->vert(vertexIndex);
            screenCoords[j] = world2screen(v);
            worldCoords[j] = v;
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
        // 先进行简单的背面剔除只对凸面体有效， convex shapes only.
        // <0: 光源从物体背面入射
        if(intensity > 0) {
            // 在三角形内部进行更深入的背面剔除
            // flat shading
            triangle(screenCoords,
                     zBuffer,
                     image,
                     TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }
    
    image.flip_vertically();
    image.write_tga_file("lesson3_draw_model_back_face_culling.tga");
}


// Homework
void triangle_texture(Vec3f *pts,
                      Vec2i *uvs,
                      float *zBuffer,
                      TGAImage &image,
                      int width = 800,
                      int height = 800 ) {
    
    // get axis aligned bounding box
    float minx = std::min({pts[0].x, pts[1].x, pts[2].x });
    float maxx = std::max({pts[0].x, pts[1].x, pts[2].x });
    float miny = std::min({pts[0].y, pts[1].y, pts[2].y });
    float maxy = std::max({pts[0].y, pts[1].y, pts[2].y });
    
    int min_x = (int)std::floor(minx);
    int max_x = (int)std::floor(maxx);
    int min_y = (int)std::floor(miny);
    int max_y = (int)std::floor(maxy);
    
    // world2screen后，在float-> int会出现数组越界
    if (max_x >= width) max_x = width - 1;
    if (max_y >= height) max_y = height - 1;
    
    /*
     遍历包围盒中的每个像素
     这里可以认为是正交投影，将3D空间的内容直接“压扁”到X-Y平面
     模型中z坐标的值就是其深度值，既z-value
     */
    for (int i = min_x; i <= max_x; i++) {
        for (int j = min_y; j <= max_y; j++) {
            
            Vec3f P(i, j, 0);
            Vec3f baryCoord = barycentric(pts, P);
            
            if (baryCoord.x < -0.01 ||
                baryCoord.y < -0.01 ||
                baryCoord.z < -0.01) {
                continue; // 如果像素再三角形外则跳过
            }
            
            
            Vec2i uvP =
            uvs[0] * baryCoord.x +
            uvs[1] * baryCoord.y +
            uvs[2] * baryCoord.z;

            float z_interpolation =
            baryCoord.x * pts[0].z +
            baryCoord.y * pts[1].z +
            baryCoord.z * pts[2].z;
            
            // 因为zbuffer是一维数组，所以(x,y)->z_value: P.x + P.y * width
            int zbufferIndex = int(P.x + P.y * width);
            if (z_interpolation > zBuffer[zbufferIndex]) {
                zBuffer[zbufferIndex] = z_interpolation;
                TGAColor color = g_model->diffuse(uvP);
                image.set(P.x, P.y, color);
            }
        }
    }
}


static void draw_texture_model() {
    TGAImage image(width, height, TGAImage::RGB);
    
    // 定义屏幕尺寸相同大小的z-buffer
    int zBufferSize = width*height;
    float *zBuffer = new float[zBufferSize];
    for (int i=0; i<zBufferSize; i++) {
        // 初始化z-buffer为-∞
        zBuffer[i] = -std::numeric_limits<float>::max();
    }
    
    Vec3f light_dir(0, 0, -1); // 定义点光源: 从原点照向屏幕外（这里里外没区别）
    
    for (int i=0; i< g_model->nfaces(); i++) {
        Vec3f screenCoords[3]; // 存贮第i个面三个顶点的屏幕坐标
        Vec3f worldCoords[3];  // 存储第i个面三个顶点的世界坐标
        Vec2i uvs[3];  // 存储第i个面三个顶点的纹理对应像素的坐标
        
        std::vector<Vec3i> face = g_model->face(i);//获取模型的第i个面
        for (int j = 0; j < 3; j++) {
            // 分别取出第i个面三个顶点的世界坐标
            int vertexIndex = face[j][0];
            Vec3f v = g_model->vert(vertexIndex);
            screenCoords[j] = world2screen(v);
            worldCoords[j] = v;
            uvs[j] = g_model->uv(i, j);
        }
        
        // 在三角形内部进行更深入的背面剔除
        // 只渲染纹理不添加光照
        triangle_texture(screenCoords,
                         uvs,
                         zBuffer,
                         image);
    }
    
    image.flip_vertically();
    image.write_tga_file("lesson3_draw_model_texture.tga");
}

void lesson3() {
    draw_face_culling_model();
    draw_texture_model();
}
