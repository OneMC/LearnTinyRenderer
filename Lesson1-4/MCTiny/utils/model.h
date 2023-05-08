#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

/// 该类只能解析自带的obj文件
/// 因为obj格式有很多参数是可选的，在实现中未对这些可选参数做处理
class Model {
private:
    /*
     顶点坐标v x,y,z,[w]
     示例中w是没有的，通常也不会有，所以用了三维向量
     e.g: v -0.117277 -0.973564 0.306907
     */
	std::vector<Vec3f> verts_;
    /*
     纹理坐标vt u,[v,w]
     示例模型中第三维全部是0，所以用了二维向量
     e.g: vt  0.412 0.975 0.000
     */
    std::vector<Vec2f> uv_;     //
    
    /*
     顶点法线vn i,j,k
     e.g: vn  0.296 -0.070 0.953
     */
    std::vector<Vec3f> norms_;  //
    
    /*
     面坐标 v[/vt/vn]
     示例模型中face包含v/vt/vn，所以用了三维向量
     e.g: f 24/1/24 25/2/25 26/3/26
     */
    std::vector<std::vector<Vec3i>> faces_;;
    TGAImage diffuseMap_;
    void load_texture(std::string filename, const char *suffix, TGAImage &image);

public:
	Model(const char *filename);
	~Model();
    
	int nverts(); // 顶点数
	int nfaces(); // 面数
    
	Vec3f vert(int i);   // 获取第i个顶点，索引从0开始
    Vec3f normal(int iface, int nvert); // 获取第'iface'面的'nvert'顶点的法线向量
    std::vector<Vec3i> face(int idx); // 获取第i个面，索引从0开始
    
    
    // 获取第'iface'面的'nvert'顶点的uv映射到纹理图片中对应像素的坐标
    // 因为是像素坐标，所以是整数
    Vec2i uv(int iface, int nvert);
    TGAColor diffuse(Vec2i uv); // 获取'uv'坐标下的颜色值
};

#endif //__MODEL_H__
