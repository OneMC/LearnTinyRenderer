//
//  MVP.cpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#include "MVP.hpp"
#include <cmath>
#include <limits>
#include <cstdlib>
#include <algorithm>

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

/// 将投影后并归一化的坐标转换到屏幕坐标
/// 这里的depth作用不大
/// 视口变换只有W和H
/// - Parameters:
///   - x: x坐标
///   - y: y坐标
///   - w: 视口宽
///   - h: 视口高
void viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity();
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = 255.0 / 2.f;
    
    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = 255.0 / 2.f; // 这里的255意义不大0xff
    Viewport = m;
}

/// 透视投影变换矩阵
/// 这里的投影矩阵与game101不同
/// 这里只调整了matrix[3][2]，没有前后视平面
void projection(float coeff) {
    Matrix projection = Matrix::identity();
    projection[3][2] = coeff;
    Projection = projection;
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
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
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
    Vec3f x = cross(up,z).normalize();
    Vec3f y = cross(z, x).normalize();
    
    /*
     矩阵的第四列是用于平移的。因为观察位置从原点变为了center，所以需要将物体平移-center
     正交矩阵的逆 = 正交矩阵的转置
     矩阵的第一行即是现在的x
     矩阵的第二行即是现在的y
     矩阵的第三行即是现在的z
     矩阵的三阶子矩阵是当前视线旋转矩阵的逆矩阵
     */
    Matrix rotation = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        rotation[0][i] = x[i];
        rotation[1][i] = y[i];
        rotation[2][i] = z[i];
        rotation[i][3] = -center[i];
    }
    //这样乘法的效果是先平移物体，再旋转
    Matrix translation = Matrix::identity();
    ModelView = rotation * translation;
}
