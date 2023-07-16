#pragma once

#include "ArrayAlgebra.h"

// ��ֵ���ֺ��������������ֽڵ�������������������ޡ�һ����������
template <typename Fun>
auto integrate(int integration_nodes, double x_min, double x_max, Fun const& fun) {
    // �������С����
    double dx = (x_max - x_min) / integration_nodes;
    // ÿ�εݽ�0.5 * dx������
    double x = x_min + 0.5 * dx;

    auto result = dx * fun(x); // the first integration node
    for (int i = 1; i < integration_nodes; i++) { // proceed with other nodes, notice `int i= 1`
        x += dx;
        result += dx * fun(x);
    }

    return result;
}
