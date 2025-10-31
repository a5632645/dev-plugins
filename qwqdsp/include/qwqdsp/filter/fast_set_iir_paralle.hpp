#pragma once
#include <complex>
#include <cmath>
#include <array>

namespace qwqdsp::filter {
namespace fastset_coeff {
struct Order2_1e2 {
static constexpr double kSetSamples = 9.3;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -1.3536304394255},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.2603297729093867, 0.8597840766187602},
};
};
struct Order2_1e3 {
static constexpr double kSetSamples = 8;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -1.9287175087446249},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.4536575152927467, 0.6611091083421231},
};
};
struct Order2_1e4 {
static constexpr double kSetSamples = 7.7;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -2.523120476763981},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.5418513835308907, 0.5259146157247194},
};
};
struct Order2_1e5 {
static constexpr double kSetSamples = 5.4;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -3.1256648057416845},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.587616563215707, 0.43322091105133287},
};
};
struct Order2_1e6 {
static constexpr double kSetSamples = 4.1;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -3.732397238719539},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.6139881114898862, 0.3670099618120878},
};
};
struct Order2_1e7 {
static constexpr double kSetSamples = 3;
static constexpr std::complex<double> r[1]{
std::complex<double>{0.0, -4.341432858272444},
};
static constexpr std::complex<double> p[1]{
std::complex<double>{-1.6304413545893377, 0.3177901986749831},
};
};

struct Order4_1e2 {
static constexpr double kSetSamples = 4.2;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.2785881029365726, 2.3965845534040935},
std::complex<double>{-0.27858810293657227, -0.7028921719332359},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-1.787549679533831, -1.07875519886928},
std::complex<double>{-1.1927270524837361, -3.4423737180141343},
};
};
struct Order4_1e3 {
static constexpr double kSetSamples = 3.7;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.33373597257380994, 4.52170630444036},
std::complex<double>{-0.33373597257380944, -1.4146285423911684},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-2.2447875982800523, -1.0061127976146957},
std::complex<double>{-1.9166630183479183, -3.1385197393506186},
};
};
struct Order4_1e4 {
static constexpr double kSetSamples = 3.2;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.4054668638883086, 8.177168920568612},
std::complex<double>{-0.40546686388830633, -2.6342884115101683},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-2.6041909900514977, -0.9049106806522916},
std::complex<double>{-2.4139421708803424, -2.779675858443664},
};
};
struct Order4_1e5 {
static constexpr double kSetSamples = 2.7;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.4842956255395606, 13.856481070614082},
std::complex<double>{-0.4842956255395616, -4.527970409575525},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-2.863636888751113, -0.8047192775327514},
std::complex<double>{-2.747039481539855, -2.450128604764705},
};
};
struct Order4_1e6 {
static constexpr double kSetSamples = 2.2;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.5669809397882178, 22.084925039995447},
std::complex<double>{-0.5669809397882118, -7.2710204820091855},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-3.048236579154352, -0.7158897355279787},
std::complex<double>{-2.973002079559734, -2.168569688088111},
};
};
struct Order4_1e7 {
static constexpr double kSetSamples = 1.8;
static constexpr std::complex<double> r[2]{
std::complex<double>{0.6520541587666238, 33.39554366669324},
std::complex<double>{-0.6520541587664862, -11.041631737076418},
};
static constexpr std::complex<double> p[2]{
std::complex<double>{-3.180676355023657, -0.6402113148152138},
std::complex<double>{-3.1298900623960924, -1.933327429751671},
};
};

struct Order6_1e2 {
static constexpr double kSetSamples = 3;
static constexpr std::complex<double> r[3]{
std::complex<double>{0.7001144110237735, 3.6500655228101113},
std::complex<double>{-0.9524718480168819, -1.4497164431277294},
std::complex<double>{0.2523574369931075, -0.17070204227237118},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-2.2236374835962414, -1.1223992281936863},
std::complex<double>{-1.7371756581012576, -3.4623675932639553},
std::complex<double>{-0.7589829688030322, 5.9539137751643505},
};
};
struct Order6_1e3 {
static constexpr double kSetSamples = 2.6;
static constexpr std::complex<double> r[3]{
std::complex<double>{0.920972946544916, 6.570877679023769},
std::complex<double>{-1.2658324615880188, -2.85348110188901},
std::complex<double>{0.3448595150431072, -0.43374500606469263},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-2.6716123427810876, -1.104506607279806},
std::complex<double>{-2.3166604319108446, -3.397373479496726},
std::complex<double>{-1.6758632613199875, 5.862152365759769},
};
};
struct Order6_1e4 {
static constexpr double kSetSamples = 2.3;
static constexpr std::complex<double> r[3]{
std::complex<double>{1.3349398748059913, 12.843431058592401},
std::complex<double>{-1.8633058616825529, -5.874770770200777},
std::complex<double>{0.5283659868765489, -0.9945126205347045},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-3.1549690966433803, -1.0564896209363133},
std::complex<double>{-2.907872107201039, -3.228883079834015},
std::complex<double>{-2.475494617173092, 5.531757243223526},
};
};
struct Order6_1e5 {
static constexpr double kSetSamples = 2;
static constexpr std::complex<double> r[3]{
std::complex<double>{1.945312507476535, 24.825288043537743},
std::complex<double>{-2.7538657619084534, -11.714242074368334},
std::complex<double>{0.8085532544319396, -2.103249595547191},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-3.5817898319719808, -0.9925377062550289},
std::complex<double>{-3.4104981545393356, -3.0172282001247597},
std::complex<double>{-3.1112769841788213, 5.132907997377606},
};
};
struct Order6_1e6 {
static constexpr double kSetSamples = 1.8;
static constexpr std::complex<double> r[3]{
std::complex<double>{2.788203049151289, 46.26743819474346},
std::complex<double>{-3.99245983957501, -22.25045536743711},
std::complex<double>{1.2042567904238826, -4.137521599858839},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-3.9365782658644015, -0.9233485758979719},
std::complex<double>{-3.816230408724684, -2.796234297387559},
std::complex<double>{-3.6040390134181153, 4.731464023678084},
};
};
struct Order6_1e7 {
static constexpr double kSetSamples = 1.6;
static constexpr std::complex<double> r[3]{
std::complex<double>{3.907645113227289, 82.60724734654363},
std::complex<double>{-5.645431269961117, -40.202591611793295},
std::complex<double>{1.7377861567330837, -7.641481271661387},
};
static constexpr std::complex<double> p[3]{
std::complex<double>{-4.224440110164785, -0.8552971760428945},
std::complex<double>{-4.138180251190477, -2.583381601362224},
std::complex<double>{-3.9841446209303717, 4.354428267028399},
};
};

struct Order8_1e2 {
static constexpr double kSetSamples = 2.5;
static constexpr std::complex<double> r[4]{
std::complex<double>{1.3818891437926484, -6.07314030094024},
std::complex<double>{-2.235000170533547, 2.7824654228065313},
std::complex<double>{0.9961380842581393, -0.46268001930856073},
std::complex<double>{-0.14302705751723904, -0.019283634187995637},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-2.7265774537247074, 1.144995298708726},
std::complex<double>{-2.3491881395842, 3.4938273673810647},
std::complex<double>{-1.6553930857249757, 5.8779065495040905},
std::complex<double>{-0.39129400480437493, 8.223200567712611},
};
};
struct Order8_1e3 {
static constexpr double kSetSamples = 2.1;
static constexpr std::complex<double> r[4]{
std::complex<double>{1.6431763020822816, -9.123743789745507},
std::complex<double>{-2.6690010134453552, 4.535924474162801},
std::complex<double>{1.2117025899560556, -1.0256935089610493},
std::complex<double>{-0.18587787859298183, 0.06856748739542945},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-3.0277388073940092, 1.1456020214211735},
std::complex<double>{-2.7142659167670242, 3.496555652706341},
std::complex<double>{-2.146366031935891, 5.9329620465229125},
std::complex<double>{-1.2639856327122398, 8.47234110032037},
};
};
struct Order8_1e4 {
static constexpr double kSetSamples = 2;
static constexpr std::complex<double> r[4]{
std::complex<double>{2.504460037132579, -17.364290443227443},
std::complex<double>{-4.123475287113271, 9.12245825766364},
std::complex<double>{1.926748629957429, -2.35245726231968},
std::complex<double>{-0.3077333799767173, 0.22782551835289824},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-3.5264423672450214, 1.1230999793674772},
std::complex<double>{-3.2805327119377306, 3.4156527112443866},
std::complex<double>{-2.8343889538463642, 5.797258810489101},
std::complex<double>{-2.1929361704738284, 8.293780134500622},
};
};
struct Order8_1e5 {
static constexpr double kSetSamples = 1.8;
static constexpr std::complex<double> r[4]{
std::complex<double>{4.029635259824521, -34.79290802809896},
std::complex<double>{-6.733963712657421, 18.99911543097309},
std::complex<double>{3.241413331133505, -5.3023306970073065},
std::complex<double>{-0.5370848783006248, 0.5915228733600788},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-4.034543463481661, 1.085095750845288},
std::complex<double>{-3.8459412959159542, 3.2894856123644396},
std::complex<double>{-3.502424807026739, 5.569584791898575},
std::complex<double>{-3.0259535824412556, 7.947584797045394},
};
};
struct Order8_1e6 {
static constexpr double kSetSamples = 1.6;
static constexpr std::complex<double> r[4]{
std::complex<double>{6.50093835071286, -69.49883410665785},
std::complex<double>{-11.00930050965848, 38.97924634769506},
std::complex<double>{5.438863127557328, -11.45290119690518},
std::complex<double>{-0.9305009686116316, 1.3790377507763025},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-4.500938378816549, 1.0377999822947326},
std::complex<double>{-4.3572321144570125, 3.1383871102638694},
std::complex<double>{-4.094030765167233, 5.298274466915076},
std::complex<double>{-3.7338668859364437, 7.535534888313555},
};
};
struct Order8_1e7 {
static constexpr double kSetSamples = 1.4;
static constexpr std::complex<double> r[4]{
std::complex<double>{10.370012162828393, -135.62811019897032},
std::complex<double>{-17.741569009626645, 77.44985518746455},
std::complex<double>{8.939089740179604, -23.540602691728147},
std::complex<double>{-1.5675328933782433, 2.971616931427933},
};
static constexpr std::complex<double> p[4]{
std::complex<double>{-4.911909696825354, 0.9858348791755321},
std::complex<double>{-4.8015804612365205, 2.975947468071204},
std::complex<double>{-4.598541433983577, 5.01179376739515},
std::complex<double>{-4.322445596824106, 7.107469243773443},
};
};
}

// qwqfixme: 系数设置时间有误
/**
 * @brief 尽可能无过冲的快速跃阶响应,并行形式IIR滤波器
 * @ref https://vicanek.de/articles/FastSettlingFilters.pdf
 */
template<class TCoeff>
class FastSetIirParalle {
public:
    void Reset() noexcept {
        y_.fill(std::complex<float>{});
    }

    float Tick(float x) noexcept {
        for (size_t i = 0; i < kNumElements; ++i) {
            y_[i] = b_[i] * x - a_[i] * y_[i];
        }
        float sum = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
            sum += y_[i].real();
        }
        return sum;
    }

    /**
     * @brief 将跃阶响应拉伸scale倍
     */
    void Make(double scale) noexcept {
        auto const& pole_table = TCoeff::p;
        auto const& amp_table = TCoeff::r;
        for (size_t i = 0; i < kNumElements; ++i) {
            auto scale_p = pole_table[i] / scale;
            auto a = -std::exp(scale_p);
            auto b = -amp_table[i] * 2.0 / pole_table[i] * (1.0 + a);
            a_[i] = a;
            b_[i] = b;
        }
    }

    /**
     * @brief 设定滤波器在近似num_samples内达到目标值
     */
    void MakeFilter(double num_samples) noexcept {
        Make(num_samples / TCoeff::kSetSamples);
    }
private:
    static constexpr size_t kNumElements = std::size(TCoeff::r);

    std::array<std::complex<float>, kNumElements> b_{};
    std::array<std::complex<float>, kNumElements> a_{};
    std::array<std::complex<float>, kNumElements> y_{};
};
}