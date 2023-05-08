#include "common.h"

extern void lesson1();
extern void lesson2();
extern void lesson3();
extern void lesson4();

Model *g_model = NULL;
/*
 使用说明:
 1. 修改model_path为电脑本地文件地址。E.g: "/Users/abc/Desktop/CG/tiny/obj/african_head.obj"
 2. 保证‘african_head_diffuse.tga’ 和 ‘african_head.obj’ 在同一个文件夹中（默认在相同文件夹）；
 3. [xcode] 修改`Use custom working directory`，主要是查看生成图片。如果不用xcode可省略。
    修改方法参见：https://docs.vapor.codes/zh/getting-started/xcode/
 */
const char *model_path = "<african_head.obj local path>";

int main(int argc, char** argv) {
    g_model = new Model("/Users/miaochao/Desktop/CG/tiny/MCTiny/MCTiny/utils/obj/african_head.obj");
    lesson4();
    lesson3();
    lesson2();
    lesson1();
    return 0;
}
