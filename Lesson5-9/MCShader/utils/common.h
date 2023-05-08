//
//  common.h
//  MCTiny
//
//  Created by 苗超 on 2023/2/2.
//

#ifndef common_h
#define common_h

#include "model.h"
#include "tgaimage.h"
#include "geometry.h"
#include "MVP.hpp"

#define African_Man "/Users/miaochao/Desktop/CG/tiny/MCTiny/MCTiny/utils/obj/african_head.obj"
#define African_Texture "/Users/miaochao/Desktop/CG/tiny/MCTiny/MCTiny/utils/obj/african_head_diffuse.tga"


#define MODEL_PATH African_Man


const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

#endif /* common_h */
