//
//  Shader.hpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#ifndef Shader_hpp
#define Shader_hpp

#include <stdio.h>
#include "common.h"

extern Model *g_model;
extern Vec3f g_light_dir;
extern const int g_width;
extern const int g_height;


/// 一个面对应一个shader
/// 每绘制一个三角形要先把shader的值重新计算
struct IShader {
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

/*
 Flat/Gouraud/Phong 不同点在于着色频率
 Flat: 三角形用一种光照强度
 Gouraud: 三角形中每个像素的光照强度使用顶点的光照强度做插值计算
 Phong: 挨个计算三角形中每个像素的光照强度
 */

struct FlatShader : public IShader {
    
    mat<3, 3, float> varying_tri;

    virtual ~FlatShader() {}

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(g_model->vert(iface, nthvert));
        gl_Vertex = Projection * ModelView * gl_Vertex;
        varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        gl_Vertex = Viewport * gl_Vertex;
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {

        Vec3f n = cross(varying_tri.col(1) - varying_tri.col(0), varying_tri.col(2) - varying_tri.col(0)).normalize();
        float intensity = n * g_light_dir;
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

struct GouraudShader : public IShader {
    /*
     'varying'是OpenGL中的一个关键字，用于传递值。这里模拟OpenGL
     */
    
    // 用一个3维向量存储每个顶点的光照强度，便于后面插值计算
    Vec3f varying_intensity;
    
    //  2 x 3 矩阵: 每一列代表顶点的uv值，存储了三个顶点的uv值
    mat<2, 3, float> varying_uv;
    
    virtual Vec4f vertex(int iface, int nthvert) {
        
        // 顶点
        Vec3f vertexValue = g_model->vert(iface, nthvert);
        Vec4f gl_Vertex = embed<4>(vertexValue); // 3维扩为4维便于齐次坐标变化
        
        // 纹理
        Vec2f uvValue = g_model->uv(iface, nthvert);
        varying_uv.set_col(nthvert, uvValue);
        
        // MVP
        Vec4f ret = Viewport * Projection * ModelView * gl_Vertex;
        
        // 光照强度
        float intensity = g_model->normal(iface, nthvert) * g_light_dir;
        varying_intensity[nthvert] = std::max(0.f, intensity); // get diffuse lighting intensity
        
        /*
        // Debug
        printf("%d-%d: (%.2f,%.2f,%.2f,%.2f)->(%.2f,%.2f,%.2f,%.2f),(%.2f,%.2f)\n",iface,nthvert,
               gl_Vertex[0],gl_Vertex[1],gl_Vertex[2],gl_Vertex[3],
               ret[0],ret[1],ret[2],ret[3],
               uvValue[0],uvValue[1]);
         */
        
        // 返回屏幕坐标
        return ret;
    }
    
    
    /// 像素着色器
    /// - Parameters:
    ///   - bar: 重心坐标
    ///   - color: 外部传入的颜色，函数内做修改
    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec2f uv = varying_uv * bar; // 插值纹理坐标
        TGAColor c = g_model->diffuse(uv);
        float intensity = varying_intensity * bar; // 插值光照强度
        color = c * intensity; // 计算光照强度
        return false;
    }
};

struct PhongShader : public IShader {
    mat<2, 3, float> varying_uv;
    mat<4, 4, float> uniform_M = Projection * ModelView;
    mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();
    
    virtual Vec4f vertex(int iface, int nthvert) {
        // 顶点
        Vec3f vertexValue = g_model->vert(iface, nthvert);
        Vec4f gl_Vertex = embed<4>(vertexValue); // 3维扩为4维便于齐次坐标变化
        
        // 纹理
        Vec2f uvValue = g_model->uv(iface, nthvert);
        varying_uv.set_col(nthvert, uvValue);
        
        /* phong 模型不需要计算顶点的光照强度，会在像素着色器中计算每个像素的光照强度 */
        
        // MVP: transform it to screen coordinates
        Vec4f ret = Viewport * Projection * ModelView * gl_Vertex;
        /*
        // Debug
        printf("%d-%d: (%.2f,%.2f,%.2f,%.2f)->(%.2f,%.2f,%.2f,%.2f),(%.2f,%.2f)\n",iface,nthvert,
               gl_Vertex[0],gl_Vertex[1],gl_Vertex[2],gl_Vertex[3],
               ret[0],ret[1],ret[2],ret[3],
               uvValue[0],uvValue[1]);
         */
        return ret;
    }
    
    virtual bool fragment(Vec3f bar, TGAColor& color) {
        
        Vec2f uv = varying_uv * bar; // 插值纹理坐标
        
        // 根据纹理坐标找到对应的法向量
        Vec3f uv_normal = g_model->normal(uv);
        Vec4f gl_uv_normal = embed<4>(uv_normal);
        Vec4f vec4Dim = uniform_MIT * gl_uv_normal;
        Vec3f n = proj<3>(vec4Dim).normalize();
        
        
        Vec4f l_tmp = uniform_M * embed<4>(g_light_dir);
        Vec3f l = proj<3>(l_tmp).normalize();
        
        Vec3f r = (n * (n * l * 2.f) - l).normalize();   // reflected light
        
        /*
         phong 模型中未考虑摄像机与反射光的角度，只考虑了光源与法线的夹角
         blinn-phong模型中才使用了半程向量计算了摄像机与反向光的角度
         */
        
        /*
         获取高光贴图
         specularValue控制表面的反光程度，不会随摄像机角度改变
         比如，某人鼻子就是油光发亮，不会受观察者影响。
         */
        float specularValue = g_model->specular(uv);
        float spec = pow(std::max(r.z, 0.0f), specularValue);
        
        // diffuse light
        float diff = std::max(0.f, n * l);
        
        TGAColor c = g_model->diffuse(uv); // 纹理颜色
        color = c;
        for (int i = 0; i < 3; i++) {
            // 分别计算BGR的值
            // ambient + diffuse + specular
            color[i] = std::min<float>(5 + c[i] * (diff + .6 * spec), 255);
        }
//        printf("uv:(%d,%d,%d),diff:%.2f, spec_ori:%.2f, spec_des:%.2f\n",c[0],c[1],c[2],diff, specularValue,spec);
        return false;
    }
};

struct ToonShader : public IShader {
    mat<3, 3, float> varying_tri;
    Vec3f          varying_ity;

    virtual ~ToonShader() {}

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(g_model->vert(iface, nthvert));
        gl_Vertex = Projection * ModelView * gl_Vertex;
        varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));

        varying_ity[nthvert] = g_model->normal(iface, nthvert) * g_light_dir;

        gl_Vertex = Viewport * gl_Vertex;
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {
        float intensity = varying_ity * bar;
        if (intensity > .85) intensity = 1;
        else if (intensity > .60) intensity = .80;
        else if (intensity > .45) intensity = .60;
        else if (intensity > .30) intensity = .45;
        else if (intensity > .15) intensity = .30;
        color = TGAColor(255, 155, 0) * intensity;
        return false;
    }
};

#endif /* Shader_hpp */
