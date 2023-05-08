//
//  matrix.hpp
//  MCTiny
//
//  Created by 苗超 on 2023/2/8.
//

#ifndef matrix_hpp
#define matrix_hpp

#include <vector>
#include <stdio.h>

const int DEFAULT_ALLOC=4;

class Matrix {
private:
    std::vector<std::vector<float>> m;
    int rows, cols;
public:
    Matrix(int r = DEFAULT_ALLOC, int c = DEFAULT_ALLOC);
    inline int nrows();
    inline int ncols();
    
    // 单位矩阵 E
    static Matrix identity(int dimensions);
    
    // 取出矩阵的第row行
    std::vector<float>& operator[](const int row);
    
    // 矩阵乘法
    Matrix operator*(const Matrix& a);
    
    // 转置矩阵Aᵀ： 行和列互换
    Matrix transpose();
    
    // 逆矩阵A⁻¹： A·A⁻¹=E
    Matrix inverse();
    
    friend std::ostream& operator<<(std::ostream& s, Matrix& m);
};

#endif /* matrix_hpp */
