#pragma once

#include "ArrayAlgebra.h"

// 数值积分函数（参数：积分节点数、积分区间的上下限、一个函数对象）
template <typename Fun>
auto integrate(int integration_nodes, double x_min, double x_max, Fun const& fun) {
    // 计算积分小区间
    double dx = (x_max - x_min) / integration_nodes;
    // 每次递进0.5 * dx个步长
    double x = x_min + 0.5 * dx;

    auto result = dx * fun(x); // the first integration node
    for (int i = 1; i < integration_nodes; i++) { // proceed with other nodes, notice `int i= 1`
        x += dx;
        result += dx * fun(x);
    }

    return result;
}
