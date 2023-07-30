#pragma once

#include "Enviroment.h"
#include "Global.h"
#include "Grid.h"
#include "ProfileBuffer.h"
#include "Spectrum.h"
#include <iostream>

namespace WaterWavelets {
    // ���Եõ��Ǹ������������  ���ڴ���ѭ��������������ǳ�����
    constexpr int pos_modulo(int n, int d) { return (n % d + d) % d; }

    constexpr Real tau = 6.28318530718; // https://tauday.com/tau-manifesto

    /**
    WaveGrid ����ˮ�������
    @section Usage


    @section Discretization

    explain that \theta_0 = 0.5*dtheta

      The location is determined by four numbers: x,y,theta,zeta
      x [-size,size] ��һ���ռ�����
      y [-size,size] �ڶ����ռ�����
      $theta \in [0,2 \pi)$  - direction of wavevector, theta==0 corresponds to
     wavevector in +x direction
      $zeta \in [\log_2(minWavelength),\log_2(maxWavelength)]$ - zeta is log2 of
     wavelength
     
     ʹ�� zeta ���沨���򲨳���ԭ���ǲ��������ʣ������и��õ���ɢ�����ԡ�����ϣ��ӵ�в�����ָ�����������ü�������
      
    */

    class WaveGrid {
    public:
        using Idx = std::array<int, 4>;

        enum Coord { X = 0, Y = 1, Theta = 2, Zeta = 3 };

    public:
        /**
           @brief Settings to set up @ref WaveGrid

           �����������С��
           [-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta]

           ��������ֱ���: n_x*n_x*n_theta*n_zeta
         */
        struct Settings {
            /** ��Ŀռ��С [-size,size]x[-size,size] */
            Real size = 50;
            /** Maximal zeta to simulate. */
            Real max_zeta = 0.01;
            /** Minimal zeta to simulate. */
            Real min_zeta = 10;

            /** ÿ���ռ�ά�ȵĽڵ���. */
            int n_x = 100;
            /** ��ɢ����������� */
            int n_theta = 16;
            /** zeta�еĽڵ����������˲����ķֱ��� @see
             * @ref ��ɢ��. */
            int n_zeta = 1;

            /** ���ó�ʼʱ�䡣Ĭ��ֵΪ 100����Ϊ��ʱ��Ϊ��ʱ����õ�
             * �����һ����ģʽ */
            Real initial_time = 100;

            /** ѡ��Ƶ������. Currently only PiersonMoskowitz is supported. */
            enum SpectrumType {
                LinearBasis,
                PiersonMoskowitz
            } spectrumType = PiersonMoskowitz;
        };

    public:
        /*
        ����WaveGrid���й���   ������һ��Settings
        s ������ʼ�� WaveGrid
        */
        WaveGrid(Settings s) : m_spectrum(10), m_enviroment(s.size) {

            // Ŀǰm_amplitude��һ��n_x * n_x * n_theta * n_zeta��Array����
            //std::cout << "s.n_x: " << s.n_x << "s.n_x: " << s.n_x << "s.n_theta: " << s.n_theta << "s.zeta: " << s.n_zeta << std::endl;
            
            // s.n_x:100      s.n_x:100      s.n_theta:8      s.n_zeta:1
            // �������Ĵ�СΪ��100 * 100 * 8 * 1
            m_amplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);
            // s.n_x:100      s.n_x:100      s.n_theta:8      s.n_zeta:1
            // �������Ĵ�СΪ��100 * 100 * 8 * 1
            m_newAmplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);
            
            // 
            Real zeta_min = m_spectrum.minZeta();   // -5.05889
            Real zeta_max = m_spectrum.maxZeta();   // 3.32193

            m_xmin = { -s.size, -s.size, 0.0, zeta_min };   // -50   -50   0   -5.05889
            m_xmax = { s.size, s.size, tau, zeta_max };     // 50    50  6.28319  3.32193

            /*
            ͨ�����㲽����m_dx���Ͳ����ĵ�����m_idx�������Ի����ÿ��ά���Ͻ�����ɢ������ʱ�ļ����С��
            �Լ�������д�����������ʵ�������ת������Щֵ�����ں����Ĳ������㡢��ֵ�����������б�ʹ�á�
            */
            // (���ֵ - ��Сֵ) / ��ά���� = ƽ������
            for (int i = 0; i < 4; i++) {
                m_dx[i] = (m_xmax[i] - m_xmin[i]) / m_amplitude.dimension(i);
                std::cout << m_dx[i] << std::endl;
                m_idx[i] = 1.0 / m_dx[i];
            }

            m_time = s.initial_time;
            // ��������ֻ��һ������Ϊs.n_zeta = 1
            m_profileBuffers.resize(s.n_zeta);
            // ���㲨Ⱥ�ٶ�
            precomputeGroupSpeeds();
        }
        /*
        ִ��һ�β���
        dt��ʱ�䲽��  
        fullUpdate Ϊtrue��ִ�б�׼ʱ�䲽�������Ϊfalse��������������ļ�������
        һ��ʱ�䲽���������ƽ�����衢��ɢ��������滺�����ļ���
        Ϊ��ѡ������dt���ṩһ������ cflTimeStep()
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
        ˮ��λ�ü�����
        pos ����֪����λ�úͷ��ߵ�λ��
        ����һ��pair��λ�úͷ���

        ����λ�ò����Ǵ�ֱλ�ƣ�������Ϊʹ����Gerstner waves������Ҳ�õ���ˮƽλ��
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
        ����CFL������ʱ�䲽��
        ������첨�ƶ�һ������Ԫ��ʱ�䣬��timeStep�����ú����ʱ�䲽������
        */
        Real cflTimeStep() const {
            return std::min(m_dx[X], m_dx[Y]) / groupSpeed(gridDim(Zeta) - 1);
        }

        /*
        ʹ��Ĭ��ֵ��չ��ɢ����
        ����һ������ signature(Int �� Int �� Int �� Int -> Real)

        ���ǽ�����洢�ڴ�СΪ(n_x*n_x*n_theta*n_zeta)4D�����ϣ�Settings����
        ��ʱ�����ص�������߽�ͻ�ȡ��������Ĭ��ֵ��������װ��theta����

        ������һ�������ĸ�����������һ�����ȵĺ��������ص��Ľ���

        */
        auto extendedGrid() const
        {

            return [this](int ix, int iy, int itheta, int izeta) {
                // ���ƽǶ�
                itheta = pos_modulo(itheta, gridDim(Theta));

                // ��������Ĳ���������
                if (izeta < 0 || izeta >= gridDim(Zeta)) {
                    return 0.0f;
                }

                // ����ģ���֮��ĵ��Ĭ��ֵ
                if (ix < 0 || ix >= gridDim(X) || iy < 0 || iy >= gridDim(Y)) {
                    return defaultAmplitude(itheta, izeta);
                }

                // ����õ������У��򷵻������ʵ��ֵ
                return m_amplitude(ix, iy, itheta, izeta);
            };
        }


        /*
        ��������ִ�����Բ�ֵ

        ����һ��������Vec4 -> Real�����������������еĵ㲢���ز�ֵ����amplitude

        �ú����������ĸ�����ļ���������ִ�����Բ�ֵ��������һ������Vec4�ĺ�����
        ���ز�ֵ���� �κ�����㶼����Ч�ģ���Ϊ��ֵ����extendedGrid()����ɵ�
        
        ����һ����ֵ���������ڸ��������λ������������չ�������Ͻ��в�ֵ
        ���㣬��ȡ��ֵ������ֵ��
        ���ֲ�ֵ������������ɢ�������Ͻ���ƽ���Ľ��Ƽ��㣬�Ի�ȡ��������
        ��֮��λ�õ����ֵ��
        */

        auto interpolatedAmplitude() const {
            auto extended_grid = extendedGrid();

            // �ú���ָʾ��Щ����������У���Щ��������
            // ʹ�ó�Ա���� inDomain �� nodePosition ���ж�������Ƿ�λ�ڶ�������
            auto domain = [this](int ix, int iy, int itheta, int izeta) -> bool {
                return m_enviroment.inDomain(nodePosition(ix, iy));
            };

            // �����ֵ������ʹ�����Բ�ֵ�ͳ�����ֵ
            auto interpolation = InterpolationDimWise(
                // ���β�ֵ    ���β�ֵ
                LinearInterpolation, LinearInterpolation, LinearInterpolation,
                ConstantInterpolation);

            // ���ڴ�����������������ֵ
            auto interpolated_grid =
                DomainInterpolation(interpolation, domain)(extended_grid);

            return [interpolated_grid, this](Vec4 pos4) mutable {
                // �������λ������ pos4 ת��Ϊ�������� ipos4
                Vec4 ipos4 = posToGrid(pos4);
                return interpolated_grid(ipos4[X], ipos4[Y], ipos4[Theta], ipos4[Zeta]);
            };
        }

        /*
        ĳ������ 
        pos4 ���������е�λ��
        ���ظ�����Ĳ�ֵ����

        ��pos4�������������꣬�ں���[-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta]��
        ����Ĭ��ֵ��defaultAmplitude() ���ں�����ķ���ֵ
        */

        Real amplitude(Vec4 pos4) const {
            return interpolatedAmplitude()(pos4);
        }

       /*
       ���񴦵����ֵ��������idx4��ȡ����Ľڵ�����   ���ظ����ڵ㴦�����ֵ

       ���idx4λ����ɢ����֮�⣬{0,...,n_x-1}x{0,...,n_x-1}x{0,...,n_theta}x{0,...,n_zeta}
       ����Ĭ��ֵ defaultAmplitude()
       */
        Real gridValue(Idx idx4) const {
            return extendedGrid()(idx4[X], idx4[Y], idx4[Theta], idx4[Zeta]);
        
        }

        /*
        ���켣
        pos4 ��ʼλ��
        ���� �켣����
        ��λ��pos4 ��ʼ�Ĳ��켣

        �÷�����Ҫ���ڵ��ԣ���Ҫ���Boudary�����Ƿ���������

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
        ���ӵ��Ŷ�
        pos ���������еĸ���λ��
        val �Ŷ�ǿ��

        �ú���������������ĳ�������з���ķ���
        */
        void addPointDisturbance(Vec2 pos, Real val) {
            // Ѱ�������Ͼ���� pos ����ĵ�
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
        Ԥ����ƽ������
        dt һ����ʱ��
        */
        
        void advectionStep(Real dt) {
            // ���Բ�ֵ����
            // ���ص��ǲ�ֵ������ֵ
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

                                // �ڰ�����������׷��
                                Vec4 trace_back_pos4 = pos4;
                                trace_back_pos4[X] -= dt * vel[X];
                                trace_back_pos4[Y] -= dt * vel[Y];

                                // ��ע�߽�
                                trace_back_pos4 = boundaryReflection(trace_back_pos4);

                                m_newAmplitude(ix, iy, itheta, izeta) = amplitude(trace_back_pos4);
                            }
                        }
                    }
                }
            }
            std::cout <<"�ܹ����������У�" << sum << std::endl;
            std::swap(m_newAmplitude, m_amplitude);
        }
        /*
        Ԥ������ɢ����
        dt һ����ʱ��
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
        Ԥ�ȼ��������ļ�������

        �ú����� ���� ��Ԥ�ȼ��������ļ����������ڲ�ʱ�� m_time

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
        Ԥ�ȼ�������ٶ�

        �������㵱ǰѡ��Ĳ��׵� Ԥ��Ⱥ�ٶ�
        */
        void precomputeGroupSpeeds() {
            // ����zeta�ĳ������涨groupSpeeds�ĸ���
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
                    // ���㲨�� 
                    Real waveLength = pow(2, zeta);
                    // ���㲨��
                    Real waveNumber = tau / waveLength;
                    // ���㲨�˵����ٶ� cg
                    Real cg = 0.5 * sqrt(9.81 / waveNumber);
                    // ���㲨���ܶ�
                    Real density = m_spectrum(zeta);
                    return { cg * density, density };
                    });

                m_groupSpeeds[izeta] =
                    3 /*the 3 should not be here !!!*/ * result[0] / result[1];
            }
        
        }

        /*
        �߽���

        pos4 ������
        ��������λ�ڱ߽��ڲ�����˺������ط����

        �����pos4 ���ڱ߽��ڣ��򷵻��� 

        ����õ�λ�ڱ߽��ڲ��������ᱻ���䣬����ζ�ſռ�λ�úͲ����򶼻ᱻ���䵽�߽�

        */
        Vec4 boundaryReflection(Vec4 pos4) const {
            Vec2 pos = Vec2{ pos4[X], pos4[Y] };
            Real ls = m_enviroment.levelset(pos);
            if (ls >= 0) // ����������ڣ�����Ҫ����
                return pos4;

            // �߽編����ˮƽ���ݶȽ���
            Vec2 n = m_enviroment.levelsetGrad(pos);

            Real theta = pos4[Theta];
            Vec2 kdir = Vec2{ cosf(theta), sinf(theta) };

            // �߽���Χ�ķ����Ͳ�ʧ����
            // ��������������ls�����ھ�߽���з��ž���
            pos = pos - 2.0 * ls * n;
            kdir = kdir - 2.0 * (kdir * n) * n;

            Real reflected_theta = atan2(kdir[Y], kdir[X]);

            // ���Ǽ��辭��һ�η�˼�����ֻص������С� �����ı߽粻����ô��������������Ч�ġ�
            // ������Բ���������衣
            //assert(m_enviroment.inDomain(pos));

            return Vec4{ pos[X], pos[Y], reflected_theta, pos4[Zeta] };
        
        }

    public:
        Real idxToPos(int idx, int dim) const {
            // m_xmin: ָ��ά���ϵ���Сλ��
            // idx + 0.5: Ϊ�˽�����λ��ת��Ϊ��Ӧλ�õ����ĵ�
            // m_dx[dim]: ���� 
            return m_xmin[dim] + (idx + 0.5) * m_dx[dim];
        }
        Vec4 idxToPos(Idx idx) const {
            return Vec4{ idxToPos(idx[X], X), idxToPos(idx[Y], Y),
              idxToPos(idx[Theta], Theta), idxToPos(idx[Zeta], Zeta) };
        }

        Real posToGrid(Real pos, int dim) const {
            return (pos - m_xmin[dim]) * m_idx[dim] - 0.5;
        }
        // λ��ת����λ��
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

        // ����ת����
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

        // ����ά��
        int  gridDim(int dim) const {
            return m_amplitude.dimension(dim);
        }
        Real dx(int dim) const {
            return m_dx[dim];
        }

    public:
        // ������������С�����ֵ
        Grid     m_amplitude, m_newAmplitude;
        // Ƶ��
        Spectrum m_spectrum;

        // �����������   ��ʵ����һ��
        std::vector<ProfileBuffer> m_profileBuffers;

        std::array<Real, 4> m_xmin;     // -50   -50   0    -5.05889
        std::array<Real, 4> m_xmax;     // 50   50   6.28319    3.32193
        std::array<Real, 4> m_dx;       // 1    1   0.785398    8.38082
        std::array<Real, 4> m_idx;      // m_dx�ĵ���

        std::vector<Real> m_groupSpeeds;

        Real m_time;

        Environment m_enviroment;
    };

} // namespace WaterWavelets
