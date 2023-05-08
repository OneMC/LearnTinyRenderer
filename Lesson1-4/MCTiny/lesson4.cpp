//
//  lesson4.cpp
//  MCTiny
//
//  Created by 苗超 on 2023/2/8.
//

#include <stdio.h>
#include "common.h"
#include "matrix.hpp"
extern Model *g_model;

/*
 Lesson4-6: MVP变换
 */

Model *model = NULL;
Vec3f eye(2,1,3);
Vec3f center(0,0,1);

const int width = 800;
const int height = 800;
const int depth = 255;

template <> template <> Vec3<int>::Vec3(const Vec3<float> &v) : x(int(v.x+.5)), y(int(v.y+.5)), z(int(v.z+.5)) {}
template <> template <> Vec3<float>::Vec3(const Vec3<int> &v) : x(v.x), y(v.y), z(v.z) {}

extern Vec3f barycentric(Vec3f* pts, Vec3f P);

Vec3f barycentric(Vec3i* pts, Vec3i P) {
    Vec3f floatPts[3]; // 转为float型，纯粹是为了少写个函数
    for(int i=0;i<3;i++) {
        floatPts[i] = Vec3f(pts[i]);
    }
    return barycentric(floatPts, Vec3f(P));
}

Vec3f cameraPos(0, 0, -3);//摄像机摆放的位置

Matrix local2homo(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.0f;
    return m;
}

/// 模型变换矩阵
/// 将模型坐标变为世界坐标
Matrix modelMatrix() {
    /*
     因为这里就放了一个模型，就放到了世界中心点
     所以可以使用单位矩阵就可以
     */
    return Matrix::identity(4);
}

/// lesson4 视图变换矩阵
Matrix viewMatrix() {
    /*
     当世界坐标与摄像机坐标系统一致时，直接使用单位矩阵即可
     */
    return Matrix::identity(4);
}

/// 拍照需要定义三个值：
/// 1. 相机的空间位置
/// 2. 相机看向的方向
/// 3. 相机本身的方向，横/竖/斜拍
///     比如风景通常横拍，人物更多用竖拍
/// game101-lecture04对此解释的更好
/// 需要注意：
/// eye和gaze是世界（World Space）坐标；
/// up本身就是一个方向矢量，而非世界坐标
///
/// - Parameters:
///   - eye: 相机所在位置，相当于相机坐标的原点，世界坐标
///   - center: 相机看向的坐标，世界坐标
///   - up: 相机本身的方向，方向矢量
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    /*
     center可以认为是相机坐标系的原点
     eye则在相机坐标系中z轴上并看向-z方向
     eye-center: z轴
     z^up: x轴，根据右手系算出x轴
     z^x: y轴。为什么不直接用up做y轴？因为up与z轴可能不正交
     */
    
    /*
     更改摄像机视角 = 更改物体位置和角度
     摄像机变换是先旋转再平移，所以物体需要先平移后旋转，且都是逆矩阵
     */
    
    //计算出z，根据z和up算出x，再算出y
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix rotation = Matrix::identity(4);
    Matrix translation = Matrix::identity(4);
    // 矩阵的第四列是用于平移的。因为观察位置从原点变为了center，所以需要将物体平移-center
    for (int i = 0; i < 3; i++) {
        rotation[i][3] = -center[i];
    }
    
    //正交矩阵的逆 = 正交矩阵的转置
    //矩阵的第一行即是现在的x
    //矩阵的第二行即是现在的y
    //矩阵的第三行即是现在的z
    //***矩阵的三阶子矩阵是当前视线旋转矩阵的逆矩阵***
    for (int i = 0; i < 3; i++) {
        rotation[0][i] = x[i];
        rotation[1][i] = y[i];
        rotation[2][i] = z[i];
    }
    //这样乘法的效果是先平移物体，再旋转
    Matrix res = rotation*translation;
    return res;
}



/// 透视投影变换矩阵
/// 这里的投影矩阵与game101不同
/// 这里只调整了matrix[3][2]，没有前后视平面
Matrix projectionMatrix() {
    Matrix projection = Matrix::identity(4);
    projection[3][2] = -1.0f / cameraPos.z;
    return projection;
}

Matrix projectionDivision(Matrix m) {
    m[0][0] = m[0][0] / m[3][0];
    m[1][0] = m[1][0] / m[3][0];
    m[2][0] = m[2][0] / m[3][0];
    m[3][0] = 1.0f;
    return m;
}

Vec3f homo2vertices(Matrix m) {
    return Vec3f(m[0][0], m[1][0], m[2][0]);
}

/// 将投影后并归一化的坐标转换到屏幕坐标
/// 这里的depth作用不大
/// 视口变换只有W和H
/// - Parameters:
///   - x: x坐标
///   - y: y坐标
///   - w: 视口宽
///   - h: 视口高
Matrix viewportMatrix(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;
    
    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

void triangle_line_sweeping(Vec3i t0, Vec3i t1, Vec3i t2,
                            float ity0, float ity1, float ity2,
                            Vec2i uv0, Vec2i uv1, Vec2i uv2,
                            float dis0, float dis1, float dis2,
                            TGAImage &image, float *zbuffer) {
    
    if (t0.y==t1.y && t0.y==t2.y) return;
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(ity0, ity1); std::swap(uv0, uv1);}
    if (t0.y > t2.y) { std::swap(t0, t2); std::swap(ity0, ity2); std::swap(uv0, uv2); }
    if (t1.y > t2.y) { std::swap(t1, t2); std::swap(ity1, ity2); std::swap(uv1, uv2); }
    
    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height;
        
        Vec3i A    = t0  + Vec3f(t2-t0  )*alpha;
        Vec3i B    = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  )*beta;
        
        float ityA =               ity0 +   (ity2-ity0)*alpha;
        float ityB = second_half ? ity1 +   (ity2-ity1)*beta : ity0 +   (ity1-ity0)*beta;
        
        Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
        Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
        
        float disA = dis0 + (dis2 - dis0) * alpha;
        float disB = second_half ? dis1 + (dis2 - dis1) * beta : dis0 + (dis1 - dis0) * beta;
        if (A.x>B.x) {
            std::swap(A, B);
            std::swap(ityA, ityB);
        }
        
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(B.x-A.x);
            
            Vec3i    P = Vec3f(A) +  Vec3f(B-A)*phi;
            float ityP =    ityA  + (ityB-ityA)*phi;
            ityP = std::min(1.f, std::abs(ityP)+0.01f);
            Vec2i uvP = uvA + (uvB - uvA) * phi;
            float disP = disA + (disB - disA) * phi;
            int idx = P.x+P.y*width;
            
            if (P.x>=width || P.y>=height || P.x<0 || P.y<0) {
                continue;
            }
            
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse(uvP);
//                // 纹理颜色加光照
                image.set(P.x, P.y,
                          TGAColor(color.bgra[2],
                                   color.bgra[1],
                                   color.bgra[0]
                                   )
                          );
                
//                image.set(P.x, P.y, TGAColor(255,255,255)* ityP); // 光照
//                image.set(P.x, P.y, color); // 纹理
//                image.set(P.x, P.y, color *ityP ); // 纹理 + 光照 + 距离
                image.set(P.x, P.y, color * ityP * (20.f/std::pow(disP, 2.f))); // 纹理+光照+距离
                
            }
        }
    }
}

/// 根据z-buffer进行back-face culling
/// - Parameters:
///   - pts: 三角形顶点
///   - zbuffer: zbuffer缓存，与屏幕尺寸相同
///   - image: 图像
///   - color: 颜色
static void triangle(Vec3i *pts,
                     Vec2i *uvs,
                     float *intensity,
                     float *distance,
                     float *zBuffer,
                     TGAImage &image,
                     int width = 800,
                     int height = 800) {

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
    
    for (int i = min_x; i <= max_x; i++) {
        for (int j = min_y; j <= max_y; j++) {
            
            Vec3i P(i, j, 0);
            Vec3f baryCoord = barycentric(pts, P);
            
            if (baryCoord.x < -0.01 ||
                baryCoord.y < -0.01 ||
                baryCoord.z < -0.01) {
                continue; // 如果像素再三角形外则跳过
            }
                
            /*
             重心坐标插值： V = αV₁ + βV₂ + γV₃
             */
            
            // 插值z-buffer
            float z_interpolation =
            baryCoord.x * pts[0].z +
            baryCoord.y * pts[1].z +
            baryCoord.z * pts[2].z;
            
            // 插值光照强度
            float ityp =
            baryCoord.x * intensity[0] +
            baryCoord.y * intensity[1] +
            baryCoord.z * intensity[2];
            
            // 插值距离
            float dis =
            baryCoord.x * distance[0] +
            baryCoord.y * distance[1] +
            baryCoord.z * distance[2];
            
            // 插值uv
            Vec2i uvP =
            uvs[0] * baryCoord.x +
            uvs[1] * baryCoord.y +
            uvs[2] * baryCoord.z;
            
            int zbufferIndex = int(P.x + P.y * width);
            
            if (z_interpolation > zBuffer[zbufferIndex]) {
                zBuffer[zbufferIndex] = z_interpolation;
//                image.set(P.x, P.y, TGAColor(255,255,255) * ityp); // 光照
                
                TGAColor color = g_model->diffuse(uvP);
//                image.set(P.x, P.y, color); // 纹理
//                image.set(P.x, P.y, color * ityp); // 纹理+光照强度
                image.set(P.x, P.y, color * ityp * (20.f/std::pow(dis, 2.f))); // 纹理+光照+距离
                 
            }
        }
    }
}

static void draw_model_projection() {
    TGAImage image(width, height, TGAImage::RGB);
    
    Matrix model_ = modelMatrix();
//    Matrix view_ = viewMatrix();
    Matrix view_ = lookat(eye, center,  Vec3f(0,1,0));
    Matrix projection_ = Matrix::identity(4);
    projection_[3][2] = -1.f / (eye - center).magnitude();
    
    Matrix viewport_ = viewportMatrix(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    
    // 定义屏幕尺寸相同大小的z-buffer
    int zBufferSize = width*height;
    float *zBuffer = new float[zBufferSize];
    for (int i=0; i<zBufferSize; i++) {
        // 初始化z-buffer为-∞
        zBuffer[i] = -std::numeric_limits<float>::max();
    }
    
    Vec3f light_dir = Vec3f(0, 1, 1).normalize(); // 定义点光源，这里应该是右手系
    
    for (int i=0; i< g_model->nfaces(); i++) {
        Vec3i screenCoords[3];  // 存贮第i个面三个顶点的屏幕坐标
        Vec2i uvs[3];           // 存储第i个面三个顶点的纹理坐标
        float intensity[3];     // 存储第i个面三个顶点光照强度
        float distance[3];      // 存储第i个面三个顶点的深度值
        
        std::vector<Vec3i> face = g_model->face(i);//获取模型的第i个面
        for (int j = 0; j < 3; j++) {
            // 分别取出第i个面第j顶点坐标
            int vertexIndex = face[j][0];
            Vec3f v = g_model->vert(vertexIndex);
            Matrix m_v = view_ * model_ * local2homo(v);
            Vec3f tmp = homo2vertices(viewport_ * projectionDivision(projection_ * m_v));
            
            screenCoords[j] = tmp;
            uvs[j] = g_model->uv(i, j);
            
            // 对lmabert模型做简化光照，不考虑光源距离
            Vec3f normal = g_model->normal(i, j);
            Vec3f tmpx = normal.normalize(); // 归一化
            intensity[j] = tmpx * light_dir;
            
            // 计算顶点到摄像机的距离
            Vec3f new_v = homo2vertices(m_v);
            distance[j] = std::pow(
                                   (std::pow (new_v.x - eye.x, 2.0f) +
                                    std::pow (new_v.y - eye.y, 2.0f) +
                                    std::pow (new_v.z - eye.z, 2.0f)),
                                   0.5f);
        }
        
        // 在三角形内部进行更深入的背面剔除
        triangle(screenCoords,
                 uvs,
                 intensity,
                 distance,
                 zBuffer,
                 image);
//
//        triangle_line_sweeping(screenCoords[0], screenCoords[1], screenCoords[2],
//                               intensity[0], intensity[1], intensity[2],
//                               uvs[0], uvs[1], uvs[2],
//                               distance[0], distance[1], distance[2],
//                               image, zBuffer);
    }
    
    image.flip_vertically();
    image.write_tga_file("lesson4_draw_model_projection.tga");
}

void lesson4() {
    draw_model_projection();
}
