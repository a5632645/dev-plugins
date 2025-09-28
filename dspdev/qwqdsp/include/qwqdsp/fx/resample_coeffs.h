#pragma once
#include <array>
#include <complex>

namespace qwqdsp::fx::coeff{
template<typename Sample>
struct FastCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 5;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 11;
	static constexpr Sample fpass = Sample(20000.000000);
	static constexpr Sample fstop = Sample(24162.255926);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-5561.982558545558), Sample(7721.564144482831)},
		{Sample(-3936.7227375705806), Sample(13650.197801811304)},
		{Sample(-2348.139919173165), Sample(17360.271619482097)},
		{Sample(-1177.5927594523114), Sample(19350.80475283362)},
		{Sample(-351.83634005467775), Sample(20192.238514865294)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-6297.9970566057245)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-16437.98665421065), Sample(-7224.765894589879)},
		{Sample(7790.757942544076), Sample(9526.642450543113)},
		{Sample(-840.5346697125525), Sample(-6786.810488747387)},
		{Sample(-1524.1587677384439), Sample(2562.99591951713)},
		{Sample(754.6036883901486), Sample(-311.82754418710147)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(10260.028875848706)
	}};
};
	
template<typename Sample>
struct BestCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 12;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 25;
	static constexpr Sample fpass = Sample(20000.000000);
	static constexpr Sample fstop = Sample(21519.444876);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-3107.20033188021), Sample(3937.2767519207428)},
		{Sample(-2796.8774913957623), Sample(7594.756479446776)},
		{Sample(-2365.3544452925835), Sample(10768.381294489338)},
		{Sample(-1897.3856301331662), Sample(13364.957940627544)},
		{Sample(-1458.0144993104516), Sample(15389.983964901507)},
		{Sample(-1082.507625108592), Sample(16910.579125449884)},
		{Sample(-781.1559131200345), Sample(18018.5367689135)},
		{Sample(-547.7046243450372), Sample(18804.09729515833)},
		{Sample(-373.4740738743549), Sample(19347.60842644775)},
		{Sample(-233.77919322514572), Sample(19702.644871990433)},
		{Sample(-135.28926799549754), Sample(19921.188456038082)},
		{Sample(-40.438395472254115), Sample(20022.29903816022)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-3220.8580773215253)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-11919.560816626928), Sample(-3187.094087204454)},
		{Sample(9573.988224127786), Sample(5583.81708412599)},
		{Sample(-6473.456636950629), Sample(-6730.189208126382)},
		{Sample(3407.4300575641755), Sample(6619.944984894173)},
		{Sample(-947.4387125888132), Sample(-5588.352305214937)},
		{Sample(-659.5593971973356), Sample(4096.061890356017)},
		{Sample(1436.828633360112), Sample(-2552.3776755275653)},
		{Sample(-1552.5358812051302), Sample(1240.0339328949058)},
		{Sample(1236.9831816533112), Sample(-299.0830011099097)},
		{Sample(-725.3145127398764), Sample(-185.6291417942369)},
		{Sample(238.53811357512163), Sample(245.88830839157475)},
		{Sample(-14.863693809439255), Sample(-95.41444051497662)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(6398.961287622787)
	}};
};
	
template<typename Sample>
struct MedianCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 15;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 31;
	static constexpr Sample fpass = Sample(20000.000000);
	static constexpr Sample fstop = Sample(22860.703061);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-1657.064991727588), Sample(2030.3709936736682)},
		{Sample(-1631.5155526840529), Sample(4039.9076334216516)},
		{Sample(-1589.22455609047), Sample(6007.98935371133)},
		{Sample(-1530.6259676390623), Sample(7914.42098391937)},
		{Sample(-1456.3210744583446), Sample(9739.639746935942)},
		{Sample(-1367.072303344283), Sample(11464.918812590095)},
		{Sample(-1263.7959846637345), Sample(13072.536002741135)},
		{Sample(-1147.5537515429705), Sample(14546.098345342754)},
		{Sample(-1019.4902800952609), Sample(15870.05342459448)},
		{Sample(-881.261296572322), Sample(17032.185897416777)},
		{Sample(-732.8349611763269), Sample(18017.257290243266)},
		{Sample(-579.9625583636853), Sample(18821.168970742823)},
		{Sample(-415.41300947022137), Sample(19427.835254646732)},
		{Sample(-253.85373389728466), Sample(19837.696087655913)},
		{Sample(-83.83083222809682), Sample(20043.733916645127)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-1665.6107015924313)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-7269.7460840146705), Sample(-1153.6269644681408)},
		{Sample(6873.9542615465725), Sample(2249.49832298613)},
		{Sample(-6233.112927451221), Sample(-3231.169694944071)},
		{Sample(5375.748825117238), Sample(4044.9447064582955)},
		{Sample(-4342.417812290342), Sample(-4641.588122002079)},
		{Sample(3186.3241606693873), Sample(4978.553313650463)},
		{Sample(-1973.9766005177996), Sample(-5022.861171159246)},
		{Sample(785.8873993487103), Sample(4756.269778694196)},
		{Sample(282.71569506386146), Sample(-4178.84598080273)},
		{Sample(-1128.9180148279086), Sample(3326.1903438907666)},
		{Sample(1631.5917883946736), Sample(-2275.2905592146767)},
		{Sample(-1707.0096964687405), Sample(1163.2285588293264)},
		{Sample(1328.988643728629), Sample(-240.92580418306122)},
		{Sample(-645.8144945563864), Sample(-213.67190644324154)},
		{Sample(134.01030057976817), Sample(147.25859917059154)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(3701.7745354960616)
	}};
};
	}
