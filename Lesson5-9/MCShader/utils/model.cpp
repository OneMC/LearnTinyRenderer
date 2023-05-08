#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"
#include "common.h"

Model::Model(const char *filename) :
verts_(), faces_(), norms_(), uv_(),
diffusemap_(), normalmap_(), specularmap_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()){
        printf("model文件读取失败，请更改路径！%s\n",filename);
        assert(false);
        return;
    }
    
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts_.push_back(v);
            
        } else if(!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
            
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f normal;
            for (int i = 0; i < 3; i++) iss >> normal[i];
            norms_.push_back(normal);
            
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                // obj的索引是从1开始，所以要减1
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    load_texture(filename, "_diffuse.tga", diffusemap_);
    load_texture(filename, "_nm.tga", normalmap_);
    load_texture(filename, "_spec.tga", specularmap_);
}


Model::~Model() { }

void Model::load_texture(std::string filename, const char *suffix, TGAImage &image) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot!=std::string::npos) {
        texfile = texfile.substr(0,dot) + std::string(suffix);
        if (!image.read_tga_file(texfile.c_str())) {
            printf("纹理加载失败:%s\n",texfile.c_str());
            assert(false);
            return;
        }
        image.flip_vertically();
    }
}


int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<Vec3i> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::vert(int iface, int nvert) {
    int idx = faces_[iface][nvert][0];
    return verts_[idx];
}


/// 获取第'iface'面的'nvert'顶点的uv坐标
/// - Parameters:
///   - iface: 面索引
///   - nvert: 顶点索引
Vec2f Model::uv(int iface, int nvert) {
    int idx = faces_[iface][nvert][1];
    return uv_[idx];
}

Vec3f Model::normal(int iface, int nvert) {
    int idx = faces_[iface][nvert][2];
    return norms_[idx].normalize();
}

TGAColor Model::diffuse(Vec2f uvf) {
    Vec2i uv(uvf[0]*diffusemap_.get_width(),
             uvf[1]*diffusemap_.get_height());
    return diffusemap_.get(uv[0], uv[1]);
}

float Model::specular(Vec2f uvf) {
    Vec2i uv(uvf[0]*specularmap_.get_width(),
             uvf[1]*specularmap_.get_height());
    return specularmap_.get(uv[0], uv[1])[0]/1.f;
}

// 获取‘uv’的法向量
Vec3f Model::normal(Vec2f uvf) {
    Vec2i uv(uvf[0]*normalmap_.get_width(),
             uvf[1]*normalmap_.get_height());
    TGAColor c = normalmap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i=0; i<3; i++) {
        res[2-i] = (float)c[i]/255.f*2.f - 1.f;
    }
    return res;
}
