//
//  Rasterization.cpp
//  MCShader
//
//  Created by 苗超 on 2023/2/14.
//

#include "Rasterization.hpp"
#include "MVP.hpp"

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) { // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    } else {
        return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
    }
}

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
    
    // 获取矩形包围盒
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    
    for (int i=0; i<3; i++) {
//        printf("\n%d:%.2f,%.2f,%.2f,%.2f\n",i,pts[i][0],pts[i][1],pts[i][2],pts[i][3]);
        for (int j=0; j<2; j++) {
            /*
             不知道这里为什么要除以pts[i][3] 这里的值是0.77，相当于x&y轴都放大了
             */
            float tmp = pts[i][j] / pts[i][3];
            bboxmin[j] = std::min(bboxmin[j], tmp);
            bboxmax[j] = std::max(bboxmax[j], tmp);
//            printf("%.2f / %.2f = %.2f\n", pts[i][j], pts[i][3], tmp);
        }
    }
    
//    printf("min:(%.2f,%.2f)-max:(%.2f,%.2f)", bboxmin[0],bboxmin[1],bboxmax[0],bboxmax[1]);
    Vec2i P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            
            Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]), // 这里同样除以pts[i][3]？？
                                  proj<2>(pts[1] / pts[1][3]),
                                  proj<2>(pts[2] / pts[2][3]),
                                  proj<2>(P));
            
            float z_P =
            (pts[0][2] / pts[0][3]) * c.x +
            (pts[0][2] / pts[1][3]) * c.y +
            (pts[0][2] / pts[2][3]) * c.z;
            
            int frag_depth = std::max(0, std::min(255, int(z_P+.5)));
            
            if (c.x<0 || c.y<0 || c.z<0 || zbuffer.get(P.x, P.y)[0] >frag_depth) continue;
            
            TGAColor color;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                //zbuffer
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                image.set(P.x, P.y, color);
            }
        }
    }
}
