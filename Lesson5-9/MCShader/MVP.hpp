//
//  MVP.hpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#ifndef MVP_hpp
#define MVP_hpp

#include <stdio.h>
#include "common.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

void lookat(Vec3f eye, Vec3f center, Vec3f up);
void projection(float coeff=0.f); // coeff = -1/c
void viewport(int x, int y, int w, int h);

#endif /* MVP_hpp */
