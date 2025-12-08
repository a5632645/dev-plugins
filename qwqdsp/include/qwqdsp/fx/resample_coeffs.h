#pragma once
#include <array>
#include <complex>

namespace qwqdsp_fx::coeff{
template<typename Sample>
struct FastCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 5;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 11;
	static constexpr Sample fpass = Sample(2.513274);
	static constexpr Sample fstop = Sample(3.036339);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-0.6989393418128478), Sample(0.9703203676211847)},
		{Sample(-0.4947031692628868), Sample(1.715334445368665)},
		{Sample(-0.29507596478690645), Sample(2.181556071363739)},
		{Sample(-0.14798067048083013), Sample(2.431693842101818)},
		{Sample(-0.04421305844716346), Sample(2.5374315271134855)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-0.7914296514145094)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-2.065658324507031), Sample(-0.9078908583339848)},
		{Sample(0.979015516727601), Sample(1.1971531974398337)},
		{Sample(-0.10562470173803516), Sample(-0.8528557589105944)},
		{Sample(-0.1915314395049395), Sample(0.32207556607792426)},
		{Sample(0.09482629615230087), Sample(-0.03918540488002405)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(1.2893132536794139)
	}};
};
	
template<typename Sample>
struct BestCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 12;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 25;
	static constexpr Sample fpass = Sample(2.895477);
	static constexpr Sample fstop = Sample(3.116319);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-0.449841370316435), Sample(0.5700147335937048)},
		{Sample(-0.4049147364045492), Sample(1.0995221733462006)},
		{Sample(-0.3424414811978542), Sample(1.5589800721865674)},
		{Sample(-0.2746918450081236), Sample(1.9348964872200827)},
		{Sample(-0.21108213502673095), Sample(2.2280671510490717)},
		{Sample(-0.15672148344516706), Sample(2.4482120426368112)},
		{Sample(-0.11307192807807198), Sample(2.6085945453086588)},
		{Sample(-0.07938429749470799), Sample(2.722428255485527)},
		{Sample(-0.053775001103282316), Sample(2.800792519009313)},
		{Sample(-0.03444640346565042), Sample(2.852775127680546)},
		{Sample(-0.018917713153412785), Sample(2.8838774109201255)},
		{Sample(-0.006141637786219206), Sample(2.8987130416992755)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-0.46629604027505667)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-1.7256407693789952), Sample(-0.4614078972654235)},
		{Sample(1.3860631829387793), Sample(0.808390724913999)},
		{Sample(-0.9371872760474355), Sample(-0.9743554343905493)},
		{Sample(0.49330666406306806), Sample(0.9583950359761884)},
		{Sample(-0.13716661488257498), Sample(-0.8090462117543589)},
		{Sample(-0.09551116316420961), Sample(0.5930070550305229)},
		{Sample(0.20787536772377396), Sample(-0.3695496327953412)},
		{Sample(-0.2251169432839332), Sample(0.17916954912983377)},
		{Sample(0.1791931627959512), Sample(-0.04432972540228242)},
		{Sample(-0.10364878727895342), Sample(-0.027025027087609708)},
		{Sample(0.03435922742842181), Sample(0.03743760158527816)},
		{Sample(-0.0029283057297882094), Sample(-0.01409042764748796)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(0.9264022936314068)
	}};
};
	
template<typename Sample>
struct MedianCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = 15;
	static constexpr size_t realCount = 1;
	static constexpr size_t filterOrder = 31;
	static constexpr Sample fpass = Sample(2.731820);
	static constexpr Sample fstop = Sample(3.122579);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		{Sample(-0.22634013932749542), Sample(0.2773303738168163)},
		{Sample(-0.22285031628275645), Sample(0.5518149627096891)},
		{Sample(-0.2170737474331995), Sample(0.8206371833945857)},
		{Sample(-0.209069708661684), Sample(1.0810385553734105)},
		{Sample(-0.1989203164753688), Sample(1.330347014898734)},
		{Sample(-0.18672995172068274), Sample(1.5660043120546343)},
		{Sample(-0.17262159172200614), Sample(1.785591881258153)},
		{Sample(-0.15675300733310138), Sample(1.9868608678821662)},
		{Sample(-0.13922867760628455), Sample(2.1677203716401476)},
		{Sample(-0.12042779679874738), Sample(2.3264132464815215)},
		{Sample(-0.1000110003346909), Sample(2.4610407687259532)},
		{Sample(-0.07931358472014319), Sample(2.5707522059170658)},
		{Sample(-0.056669494843696655), Sample(2.6537012114192398)},
		{Sample(-0.034710233132946855), Sample(2.7096375283128786)},
		{Sample(-0.01144123876518588), Sample(2.7377957604523786)}
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		Sample(-0.2275074062549286)
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		{Sample(-0.9929817778695934), Sample(-0.1575750433287008)},
		{Sample(0.9389201830003855), Sample(0.3072611915340411)},
		{Sample(-0.8513870346951454), Sample(-0.4413486518403211)},
		{Sample(0.7342788076317839), Sample(0.5525029704372728)},
		{Sample(-0.5931353898262084), Sample(-0.6339991996986816)},
		{Sample(0.4352210290430343), Sample(0.6800246950487017)},
		{Sample(-0.2696380329844943), Sample(-0.6860826282271187)},
		{Sample(0.10731248272712221), Sample(0.649639815820781)},
		{Sample(0.03856147620927125), Sample(-0.5708656499550461)},
		{Sample(-0.15422130462080583), Sample(0.454194918424052)},
		{Sample(0.2229426098747146), Sample(-0.3108762187602678)},
		{Sample(-0.23302757942745528), Sample(0.15895995656575948)},
		{Sample(0.18153501928218618), Sample(-0.032741250608135056)},
		{Sample(-0.08828374758794753), Sample(-0.029093132802625522)},
		{Sample(0.018274238820460486), Sample(0.02010869929273181)}
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		Sample(0.5056290298243583)
	}};
};
	}
