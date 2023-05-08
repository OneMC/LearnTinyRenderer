//
//  mian.cpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "MVP.hpp"
#include "Shader.hpp"
#include "ShadowShader.hpp"
#include "Rasterization.hpp"

// 全局变量
Model *g_model = NULL;
Vec3f g_light_dir(1,1,0);
const int g_width  = 800;
const int g_height = 800;


void shader(IShader &shader , const char * imageName) {
    TGAImage image (g_width, g_height, TGAImage::RGB);
    TGAImage zbuffer(g_width, g_height, TGAImage::GRAYSCALE);
    for(int i=0; i<g_model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j=0; j<3; j++) {
            Vec4f tmp = shader.vertex(i,j);
            screen_coords[j] = tmp;
        }
        triangle(screen_coords, shader, image, zbuffer);
    }
    
    image.flip_vertically(); // to place the origin in the bottom left corner of the image
    zbuffer.flip_vertically();
    
    static std::string imgSuffix = std::string(".tga");
    static std::string zbufferSuffix = std::string("_zbuffer.tga");
    image.write_tga_file((std::string(imageName) + imgSuffix).c_str());
    zbuffer.write_tga_file((std::string(imageName) + zbufferSuffix).c_str());
}

int main(int argc, char** argv) {
    g_model = new Model("/Users/miaochao/Desktop/CG/tiny/MCShader/MCShader/utils/obj/african_head/african_head.obj");
    Vec3f eye(1,0.5,1.5);
    Vec3f center(0,0,0);
    Vec3f up(0,1,0);
    
    lookat(eye, center, up);
    viewport(g_width/8, g_height/8, g_width*3/4, g_height*3/4);
    projection(-1.f / (eye-center).norm());
    g_light_dir.normalize();
    
    GouraudShader gouraud; shader(gouraud, "gouraud_shader");
    PhongShader phong; shader(phong, "phong_shader");
    ToonShader toon; shader(toon, "toon_shader");
    
    g_model = new Model("/Users/miaochao/Desktop/CG/tiny/MCShader/MCShader/utils/obj/diablo3_pose/diablo3_pose.obj");
    DepthShader depth; shader(depth, "depth_shader");
    Matrix M = Viewport*Projection*ModelView;
    ShadowShader shaderx(ModelView, (Projection*ModelView).invert_transpose(), M*(Viewport*Projection*ModelView).invert());
    shader(shaderx, "shadow_shader");
    
    delete g_model;
    return 0;
}
