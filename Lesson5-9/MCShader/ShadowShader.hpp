//
//  ShadowShader.hpp
//  MCShader
//
//  Created by 苗超 on 2023/2/16.
//

#ifndef ShadowShader_hpp
#define ShadowShader_hpp

#include <stdio.h>
#include "Shader.hpp"

struct ShadowShader : public IShader {
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()
    mat<4,4,float> uniform_Mshadow; // transform framebuffer screen coordinates to shadowbuffer screen coordinates
    mat<2,3,float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<3,3,float> varying_tri; // triangle coordinates before Viewport transform, written by VS, read by FS
    float *shadowbuffer;

    ShadowShader(Matrix M, Matrix MIT, Matrix MS) :
    uniform_M(M),
    uniform_MIT(MIT),
    uniform_Mshadow(MS),
    varying_uv(),
    varying_tri(),
    shadowbuffer(new float[g_width * g_height]{}){  }

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, g_model->uv(iface, nthvert));
        Vec4f gl_Vertex = Viewport*Projection*ModelView*embed<4>(g_model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, proj<3>(gl_Vertex/gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec4f sb_p = uniform_Mshadow*embed<4>(varying_tri*bar); // corresponding point in the shadow buffer
        sb_p = sb_p/sb_p[3];
        int idx = int(sb_p[0]) + int(sb_p[1])*g_width; // index in the shadowbuffer array
//        float shadow = .3+.7*(shadowbuffer[idx]<sb_p[2]); // magic coeff to avoid z-fighting
        float shadow = .3+.7*(shadowbuffer[idx]<sb_p[2]+43.34); // magic coeff to avoid z-fighting

        Vec2f uv = varying_uv*bar;                 // interpolate uv for the current pixel
        Vec3f n = proj<3>(uniform_MIT*embed<4>(g_model->normal(uv))).normalize(); // normal
        Vec3f l = proj<3>(uniform_M  *embed<4>(g_light_dir )).normalize(); // light vector
        Vec3f r = (n*(n*l*2.f) - l).normalize();   // reflected light
        float spec = pow(std::max(r.z, 0.0f), g_model->specular(uv));
        float diff = std::max(0.f, n*l);
        TGAColor c = g_model->diffuse(uv);
        for (int i=0; i<3; i++) color[i] = std::min<float>(20 + c[i]*shadow*(1.2*diff + .6*spec), 255);
        return false;
    }
};

struct DepthShader : public IShader {
    mat<3,3,float> varying_tri;

    DepthShader() : varying_tri() {}

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(g_model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
        varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec3f p = varying_tri*bar;
        color = TGAColor(255, 255, 255)*(p.z/255);
        return false;
    }
};

struct ZShader : public IShader {
    mat<4,3,float> varying_tri;
    
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = Projection*ModelView*embed<4>(g_model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor &color) {
        color = TGAColor(0, 0, 0);
        return false;
    }
};


#endif /* ShadowShader_hpp */
