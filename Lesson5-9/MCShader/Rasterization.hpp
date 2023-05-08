//
//  Rasterization.hpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#ifndef Rasterization_hpp
#define Rasterization_hpp

#include <stdio.h>
#include "common.h"
#include "Shader.hpp"

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);

#endif /* Rasterization_hpp */
