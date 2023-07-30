#pragma once

#include "Enviroment.h"
#include "Global.h"
#include "Grid.h"
#include "ProfileBuffer.h"
#include "Spectrum.h"
#include <iostream>

namespace WaterWavelets {
    // 可以得到非负数的余数结果  对于处理循环或周期性问题非常有用
    constexpr int pos_modulo(int n, int d) { return (n % d + d) % d; }

    constexpr Real tau = 6.28318530718; // https://tauday.com/tau-manifesto

    /**
    WaveGrid 代表水面的主类
    @section Usage


    @section Discretization

    explain that \theta_0 = 0.5*dtheta

      The location is determined by four numbers: x,y,theta,zeta
      x [-size,size] 第一个空间坐标
      y [-size,size] 第二个空间坐标
      $theta \in [0,2 \pi)$  - direction of wavevector, theta==0 corresponds to
     wavevector in +x direction
      $zeta \in [\log_2(minWavelength),\log_2(maxWavelength)]$ - zeta is log2 of
     wavelength
     
     使用 zeta 代替波数或波长的原因是波长的性质，它具有更好的离散化特性。我们希望拥有波长呈指数增长的良好级联波。
      
    */

    class WaveGrid {
    public:
        using Idx = std::array<int, 4>;

        enum Coord { X = 0, Y = 1, Theta = 2, Zeta = 3 };

    public:
        /**
           @brief Settings to set up @ref WaveGrid

           结果域的物理大小：
           [-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta]

           最终网格分辨率: n_x*n_x*n_theta*n_zeta
         */
        struct Settings {
            /** 域的空间大小 [-size,size]x[-size,size] */
            Real size = 50;
            /** Maximal zeta to simulate. */
            Real max_zeta = 0.01;
            /** Minimal zeta to simulate. */
            Real min_zeta = 10;

            /** 每个空间维度的节点数. */
            int n_x = 100;
            /** 离散波方向的数量 */
            int n_theta = 16;
            /** zeta中的节点数，决定了波数的分辨率 @see
             * @ref 离散化. */
            int n_zeta = 1;

            /** 设置初始时间。默认值为 100，因为在时间为零时您会得到
             * 特殊的一致性模式 */
            Real initial_time = 100;

            /** 选择频谱类型. Currently only PiersonMoskowitz is supported. */
            enum SpectrumType {
                LinearBasis,
                PiersonMoskowitz
            } spectrumType = PiersonMoskowitz;
        };

    public:
        /*
        根据WaveGrid进行构造   参数是一个Settings
        s 用来初始化 WaveGrid
        */
        WaveGrid(Settings s) : m_spectrum(10), m_enviroment(s.size) {

            // 目前m_amplitude是一个n_x * n_x * n_theta * n_zeta的Array数组
            //std::cout << "s.n_x: " << s.n_x << "s.n_x: " << s.n_x << "s.n_theta: " << s.n_theta << "s.zeta: " << s.n_zeta << std::endl;
            
            // s.n_x:100      s.n_x:100      s.n_theta:8      s.n_zeta:1
            // 最终它的大小为：100 * 100 * 8 * 1
            m_amplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);
            // s.n_x:100      s.n_x:100      s.n_theta:8      s.n_zeta:1
            // 最终它的大小为：100 * 100 * 8 * 1
            m_newAmplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);
            
            // 
            Real zeta_min = m_spectrum.minZeta();   // -5.05889
            Real zeta_max = m_spectrum.maxZeta();   // 3.32193

            m_xmin = { -s.size, -s.size, 0.0, zeta_min };   // -50   -50   0   -5.05889
            m_xmax = { s.size, s.size, tau, zeta_max };     // 50    50  6.28319  3.32193

            /*
            通过计算步长（m_dx）和步长的倒数（m_idx），可以获得在每个维度上进行离散化计算时的间隔大小，
            以及方便进行从网格索引到实际坐标的转换。这些值可能在后续的波格点计算、插值或其他计算中被使用。
            */
            // (最大值 - 最小值) / 该维总数 = 平均步长
            for (int i = 0; i < 4; i++) {
                m_dx[i] = (m_xmax[i] - m_xmin[i]) / m_amplitude.dimension(i);
                std::cout << m_dx[i] << std::endl;
                m_idx[i] = 1.0 / m_dx[i];
            }

            m_time = s.initial_time;
            // 轮廓缓冲只有一个，因为s.n_zeta = 1
            m_profileBuffers.resize(s.n_zeta);
            // 计算波群速度
            precomputeGroupSpeeds();
        }
        /*
        执行一次步骤
        dt是时间步长  
        fullUpdate 为true则执行标准时间步长，如果为false，则仅更新配置文件缓冲区
        一个时间步骤包括进行平流步骤、扩散步骤和剖面缓冲区的计算
        为了选择合理的dt，提供一个函数 cflTimeStep()
        */
        void timeStep(const Real dt, bool fullUpdate = true) 
        {
            {
                if (fullUpdate) {
                    advectionStep(dt);
                    diffusionStep(dt);
                }
                precomputeProfileBuffers();
                m_time += dt;
            }
        }
        /*
        水面位置及法线
        pos 你想知道的位置和法线的位置
        返回一个pair：位置和法线

        返回位置不仅是垂直位移，而且因为使用了Gerstner waves，我们也得到了水平位移
        */
        std::pair<Vec3, Vec3> waterSurface(Vec2 pos) const 
        {
            Vec3 surface = { 0, 0, 0 };
            Vec3 tx = { 0, 0, 0 };
            Vec3 ty = { 0, 0, 0 };

            for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {
                Real  zeta = idxToPos(izeta, Zeta);
                auto& profile = m_profileBuffers[izeta];

                int  NUM = gridDim(Theta);
                int  N = 4 * NUM;
                Real da = 1.0 / N;
                Real dx = NUM * tau / N;
                for (Real a = 0; a < 1; a += da) {

                    Real angle = a * tau;
                    Vec2 kdir = Vec2{ cosf(angle), sinf(angle) };
                    Real kdir_x = kdir * pos;

                    Vec4 wave_data =
                        dx * amplitude({ pos[X], pos[Y], angle, zeta }) * profile(kdir_x);

                    surface +=
                        Vec3{ kdir[0] * wave_data[0], kdir[1] * wave_data[0], wave_data[1] };

                    tx += kdir[0] * Vec3{ wave_data[2], 0, wave_data[3] };
                    ty += kdir[1] * Vec3{ 0, wave_data[2], wave_data[3] };
                }
            }

            Vec3 normal = normalized(cross(tx, ty));

            return { surface, normal };
        }

        /*
        基于CFL条件的时间步长
        返回最快波移动一个网格单元的时间，在timeStep中设置合理的时间步很有用
        */
        Real cflTimeStep() const {
            return std::min(m_dx[X], m_dx[Y]) / groupSpeed(gridDim(Zeta) - 1);
        }

        /*
        使用默认值扩展离散网格
        返回一个函数 signature(Int × Int × Int × Int -> Real)

        我们将振幅存储在大小为(n_x*n_x*n_theta*n_zeta)4D网格上（Settings），
        有时，不必担心网格边界和获取网格外点的默认值或者它包装了theta坐标

        它返回一个接受四个整数并返回一个幅度的函数，不必担心界限

        */
        auto extendedGrid() const
        {

            return [this](int ix, int iy, int itheta, int izeta) {
                // 环绕角度
                itheta = pos_modulo(itheta, gridDim(Theta));

                // 对于域外的波数返回零
                if (izeta < 0 || izeta >= gridDim(Zeta)) {
                    return 0.0f;
                }

                // 返回模拟框之外的点的默认值
                if (ix < 0 || ix >= gridDim(X) || iy < 0 || iy >= gridDim(Y)) {
                    return defaultAmplitude(itheta, izeta);
                }

                // 如果该点在域中，则返回网格的实际值
                return m_amplitude(ix, iy, itheta, izeta);
            };
        }


        /*
        在网格上执行线性插值

        返回一个函数（Vec4 -> Real）：接受物理坐标中的点并返回插值幅度amplitude

        该函数在所有四个坐标的计算网格上执行线性插值，它返回一个接受Vec4的函数并
        返回插值幅度 任何输入点都是有效的，因为插值是在extendedGrid()上完成的
        
        创建一个插值函数，用于根据输入的位置向量，在扩展的网格上进行插值
        计算，获取插值后的振幅值。
        这种插值方法可以在离散的网格上进行平滑的近似计算，以获取介于网格
        点之间位置的振幅值。
        */

        auto interpolatedAmplitude() const {
            auto extended_grid = extendedGrid();

            // 该函数指示那些网格点在域中，那些不在域中
            // 使用成员函数 inDomain 和 nodePosition 来判断网格点是否位于定义域内
            auto domain = [this](int ix, int iy, int itheta, int izeta) -> bool {
                return m_enviroment.inDomain(nodePosition(ix, iy));
            };

            // 定义插值方法，使用线性插值和常数插值
            auto interpolation = InterpolationDimWise(
                // 三次插值    三次插值
                LinearInterpolation, LinearInterpolation, LinearInterpolation,
                ConstantInterpolation);

            // 用于处理定义域内外的网络插值
            auto interpolated_grid =
                DomainInterpolation(interpolation, domain)(extended_grid);

            return [interpolated_grid, this](Vec4 pos4) mutable {
                // 将输入的位置向量 pos4 转换为网格坐标 ipos4
                Vec4 ipos4 = posToGrid(pos4);
                return interpolated_grid(ipos4[X], ipos4[Y], ipos4[Theta], ipos4[Zeta]);
            };
        }

        /*
        某点的振幅 
        pos4 物理坐标中的位置
        返回给定点的插值幅度

        点pos4必须是物理坐标，在盒子[-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta]中
        设置默认值是defaultAmplitude() 点在盒子外的返回值
        */

        Real amplitude(Vec4 pos4) const {
            return interpolatedAmplitude()(pos4);
        }

       /*
       网格处的振幅值，而不是idx4获取振幅的节点索引   返回给定节点处的振幅值

       如果idx4位于离散网格之外，{0,...,n_x-1}x{0,...,n_x-1}x{0,...,n_theta}x{0,...,n_zeta}
       返回默认值 defaultAmplitude()
       */
        Real gridValue(Idx idx4) const {
            return extendedGrid()(idx4[X], idx4[Y], idx4[Theta], idx4[Zeta]);
        
        }

        /*
        波轨迹
        pos4 起始位置
        长度 轨迹长度
        从位置pos4 开始的波轨迹

        该方法主要用于调试，主要检查Boudary反射是否正常工作

        */
        std::vector<Vec4> trajectory(Vec4 pos4, Real length) const {
            std::vector<Vec4> trajectory;
            Real              dist = 0;

            for (Real dist = 0; dist <= length;) {

                trajectory.push_back(pos4);

                Vec2 vel = groupVelocity(pos4);
                Real dt = dx(X) / norm(vel);

                pos4[X] += dt * vel[X];
                pos4[Y] += dt * vel[Y];

                pos4 = boundaryReflection(pos4);

                dist += dt * norm(vel);
            }
            trajectory.push_back(pos4);
            return trajectory;
        }

        /*
        增加点扰动
        pos 物理坐标中的干扰位置
        val 扰动强度

        该函数基本上增加了某个点所有方向的幅度
        */
        void addPointDisturbance(Vec2 pos, Real val) {
            // 寻找网格上距离点 pos 最近的点
            int ix = posToIdx(pos[X], X);
            int iy = posToIdx(pos[Y], Y);
            if (ix >= 0 && ix < gridDim(X) && iy >= 0 && iy < gridDim(Y)) {

                for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
                    m_amplitude(ix, iy, itheta, 0) += val;
                }
            }
        }

    public:

        /*
        预成型平流步骤
        dt 一步的时间
        */
        
        void advectionStep(Real dt) {
            // 线性插值幅度
            // 返回的是插值后的振幅值
            auto amplitude = interpolatedAmplitude();
            int sum = 0;
#pragma omp parallel for collapse(2)
            for (int ix = 0; ix < gridDim(X); ++ix) {
                for (int iy = 0; iy < gridDim(Y); ++iy) {

                    Vec2 pos = nodePosition(ix, iy);
                    
                    // update only points in the domain
                    if (m_enviroment.inDomain(pos)) {
                        sum++;
                        for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
                            for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

                                Vec4 pos4 = idxToPos({ ix, iy, itheta, izeta });
                                Vec2 vel = groupVelocity(pos4);

                                // 在半拉格朗日中追踪
                                Vec4 trace_back_pos4 = pos4;
                                trace_back_pos4[X] -= dt * vel[X];
                                trace_back_pos4[Y] -= dt * vel[Y];

                                // 关注边界
                                trace_back_pos4 = boundaryReflection(trace_back_pos4);

                                m_newAmplitude(ix, iy, itheta, izeta) = amplitude(trace_back_pos4);
                            }
                        }
                    }
                }
            }
            std::cout <<"总共多少在域中：" << sum << std::endl;
            std::swap(m_newAmplitude, m_amplitude);
        }
        /*
        预成型扩散步骤
        dt 一步的时间
        */
        void diffusionStep(Real dt) {

            auto grid = extendedGrid();

#pragma omp parallel for collapse(2)
            for (int ix = 0; ix < gridDim(X); ix++) {
                for (int iy = 0; iy < gridDim(Y); iy++) {

                    float ls = m_enviroment.levelset(nodePosition(ix, iy));

                    for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
                        for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

                            Vec4 pos4 = idxToPos({ ix, iy, itheta, izeta });
                            Real gamma = 2 * 0.025 * groupSpeed(izeta) * dt * m_idx[X];

                            // do diffusion only if you are 2 grid nodes away from boudnary
                            if (ls >= 4 * dx(X)) {
                                m_newAmplitude(ix, iy, itheta, izeta) =
                                    (1 - gamma) * grid(ix, iy, itheta, izeta) +
                                    gamma * 0.5 *
                                    (grid(ix, iy, itheta + 1, izeta) +
                                        grid(ix, iy, itheta - 1, izeta));
                            }
                            else {
                                m_newAmplitude(ix, iy, itheta, izeta) = grid(ix, iy, itheta, izeta);
                            }
                            // auto dispersion = [](int i) { return 1.0; };
                            // Real delta =
                            //     1e-5 * dt * pow(m_dx[3], 2) * dispersion(waveNumber(izeta));
                            // 0.5 * delta *
                            //     (m_amplitude(ix, iy, itheta, izeta + 1) +
                            //      m_amplitude(ix, iy, itheta, izeta + 1));
                        }
                    }
                }
            }
            std::swap(m_newAmplitude, m_amplitude);
        }
        /*
        预先计算配置文件缓冲区

        该函数的 参数 是预先计算配置文件缓冲区的内部时间 m_time

        */
        void precomputeProfileBuffers() {

            for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

                Real zeta_min = idxToPos(izeta, Zeta) - 0.5 * dx(Zeta);
                Real zeta_max = idxToPos(izeta, Zeta) + 0.5 * dx(Zeta);

                // define spectrum

                m_profileBuffers[izeta].precompute(m_spectrum, m_time, zeta_min, zeta_max);
            }
        }
        /*
        预先计算的组速度

        基本计算当前选择的波谱的 预期群速度
        */
        void precomputeGroupSpeeds() {
            // 根据zeta的长度来规定groupSpeeds的个数
            m_groupSpeeds.resize(gridDim(Zeta));
            // 
            for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

                Real zeta_min = idxToPos(izeta, Zeta) - 0.5 * dx(Zeta);
                //std::cout << "izeta: " << izeta << std::endl;
                //std::cout << "Zeta: " << Zeta << std::endl;
                //std::cout << " 0.5 * dx(Zeta): " << 0.5 * dx(Zeta) << std::endl;
                //std::cout << "zeta_min: " << zeta_min << std::endl;

                Real zeta_max = idxToPos(izeta, Zeta) + 0.5 * dx(Zeta);

                auto result = integrate(100, zeta_min, zeta_max, [&](Real zeta) -> Vec2 {
                    // 计算波长 
                    Real waveLength = pow(2, zeta);
                    // 计算波数
                    Real waveNumber = tau / waveLength;
                    // 计算波浪的相速度 cg
                    Real cg = 0.5 * sqrt(9.81 / waveNumber);
                    // 计算波浪密度
                    Real density = m_spectrum(zeta);
                    return { cg * density, density };
                    });

                m_groupSpeeds[izeta] =
                    3 /*the 3 should not be here !!!*/ * result[0] / result[1];
            }
        
        }

        /*
        边界检查

        pos4 待检查点
        如果输入点位于边界内部，则此函数返回反射点

        如果点pos4 不在边界内，则返回它 

        如果该点位于边界内部，则它会被反射，这意味着空间位置和波方向都会被反射到边界

        */
        Vec4 boundaryReflection(Vec4 pos4) const {
            Vec2 pos = Vec2{ pos4[X], pos4[Y] };
            Real ls = m_enviroment.levelset(pos);
            if (ls >= 0) // 如果点在域内，则不需要反射
                return pos4;

            // 边界法线由水平集梯度近似
            Vec2 n = m_enviroment.levelsetGrad(pos);

            Real theta = pos4[Theta];
            Vec2 kdir = Vec2{ cosf(theta), sinf(theta) };

            // 边界周围的反射点和波失方向
            // 这里我们依赖“ls”等于距边界的有符号距离
            pos = pos - 2.0 * ls * n;
            kdir = kdir - 2.0 * (kdir * n) * n;

            Real reflected_theta = atan2(kdir[Y], kdir[X]);

            // 我们假设经过一次反思后您又回到了域中。 如果你的边界不是那么疯狂，这个假设是有效的。
            // 这个断言测试这个假设。
            //assert(m_enviroment.inDomain(pos));

            return Vec4{ pos[X], pos[Y], reflected_theta, pos4[Zeta] };
        
        }

    public:
        Real idxToPos(int idx, int dim) const {
            // m_xmin: 指定维度上的最小位置
            // idx + 0.5: 为了将索引位置转换为对应位置的中心点
            // m_dx[dim]: 步长 
            return m_xmin[dim] + (idx + 0.5) * m_dx[dim];
        }
        Vec4 idxToPos(Idx idx) const {
            return Vec4{ idxToPos(idx[X], X), idxToPos(idx[Y], Y),
              idxToPos(idx[Theta], Theta), idxToPos(idx[Zeta], Zeta) };
        }

        Real posToGrid(Real pos, int dim) const {
            return (pos - m_xmin[dim]) * m_idx[dim] - 0.5;
        }
        // 位置转网格位置
        Vec4 posToGrid(Vec4 pos4) const {
            return Vec4{ posToGrid(pos4[X], X), posToGrid(pos4[Y], Y),
              posToGrid(pos4[Theta], Theta), posToGrid(pos4[Zeta], Zeta) };
        }

        int posToIdx(Real pos, int dim) const {
            return round(posToGrid(pos, dim));
        }
        Idx posToIdx(Vec4 pos4) const {
            return Idx{ posToIdx(pos4[X], X), posToIdx(pos4[Y], Y),
             posToIdx(pos4[Theta], Theta), posToIdx(pos4[Zeta], Zeta) };
        }

        // 索引转坐标
        Vec2 nodePosition(int ix, int iy) const {
            return Vec2{ idxToPos(ix, 0), idxToPos(iy, 1) };
        }

        Real waveLength(int izeta) const {
            Real zeta = idxToPos(izeta, Zeta);
            return pow(2, zeta);
        }
        Real waveNumber(int izeta) const {
            return tau / waveLength(izeta);
        }

        Real dispersionRelation(Real knum) const {
            const Real g = 9.81;
            return sqrt(knum * g);
        }
        Real dispersionRelation(Vec4 pos4) const {
            Real knum = waveNumber(pos4[Zeta]);
            return dispersionRelation(knum);
        }
        Real groupSpeed(int izeta) const {
            return m_groupSpeeds[izeta];
        }
        // Real groupSpeed(Vec4 pos4) const;

        // Vec2 groupVelocity(int izeta) const;
        Vec2 groupVelocity(Vec4 pos4) const {
            int  izeta = posToIdx(pos4[Zeta], Zeta);
            Real cg = groupSpeed(izeta);
            Real theta = pos4[Theta];
            return cg * Vec2{ cosf(theta), sinf(theta) };
        }
        Real defaultAmplitude(int itheta, int izeta) const {
            if (itheta == 5 * gridDim(Theta) / 16)
                return 0.1;
            return 0.0;
        }

        // 返回维度
        int  gridDim(int dim) const {
            return m_amplitude.dimension(dim);
        }
        Real dx(int dim) const {
            return m_dx[dim];
        }

    public:
        // 存放两个网格大小的振幅值
        Grid     m_amplitude, m_newAmplitude;
        // 频谱
        Spectrum m_spectrum;

        // 存放轮廓缓冲   其实它就一个
        std::vector<ProfileBuffer> m_profileBuffers;

        std::array<Real, 4> m_xmin;     // -50   -50   0    -5.05889
        std::array<Real, 4> m_xmax;     // 50   50   6.28319    3.32193
        std::array<Real, 4> m_dx;       // 1    1   0.785398    8.38082
        std::array<Real, 4> m_idx;      // m_dx的导数

        std::vector<Real> m_groupSpeeds;

        Real m_time;

        Environment m_enviroment;
    };

} // namespace WaterWavelets
