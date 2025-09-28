import numpy
import scipy
import scipy.signal
import scipy.optimize
import sys
import matplotlib.pyplot as plt

def DesignElliptic(fpass, fstop, rpass, rstop, name):
	# 滤波器设计
	order, wn = scipy.signal.ellipord(fpass, fstop, rpass, rstop, True)
	b, a = scipy.signal.ellip(order, rpass, rstop, wn, analog=True)
	w, mag, phase = scipy.signal.bode(scipy.signal.TransferFunction(b, a), n=32768*16)
	
	idx = numpy.where(mag < -rstop)[0][0]
	# 实际上应该用elliptic-degree求精准k值?
	fstop = w[idx]

	# 转并行滤波器
	residue_coeffs, residue_poles, residue_direct = scipy.signal.residue(b, a)
	if len(residue_direct) > 0:
		raise RuntimeError(f'无法处理滤波器系数,拥有非一阶滤波器残差 {residue_direct}')
	
	plt.plot(w, mag)
	plt.xlabel('Frequency')
	plt.ylabel('Magnitude')
	plt.title(f'Magnitude Response of {name}')
	plt.show()

	# 导出系数
	real_indices = numpy.where(numpy.abs(residue_poles.imag) < 1e-6)
	real_poles = residue_poles[real_indices]
	complex_indices = numpy.where(residue_poles.imag >= 1e-6)
	complex_poles = residue_poles[complex_indices]

	# Calculate the coefficients for direct synthesis of the output
	real_coeffs_direct = residue_coeffs[real_indices]*1
	complex_coeffs_direct = residue_coeffs[complex_indices]*2 # doubled since we're only using one and taking the real part

	return (fpass, fstop, order, real_coeffs_direct, real_poles, complex_coeffs_direct, complex_poles, name)


def DesignChebyshev1(fpass, fstop, rpass, rstop, name):
	# 滤波器设计
	order, wn = scipy.signal.cheb1ord(fpass, fstop, rpass, rstop, True)
	# 阻止偶数阶切比雪夫1在DC处不为0
	if order % 2 == 0:
		order += 1
	b, a = scipy.signal.cheby1(order, rpass, wn, analog=True)
	w, mag, phase = scipy.signal.bode(scipy.signal.TransferFunction(b, a), n=32768*16)
	
	idx = numpy.where(mag < -rstop)[0][0]
	# 实际上应该用elliptic-degree求精准k值?
	fstop = w[idx]

	# 转并行滤波器
	residue_coeffs, residue_poles, residue_direct = scipy.signal.residue(b, a)
	if len(residue_direct) > 0:
		raise RuntimeError(f'无法处理滤波器系数,拥有非一阶滤波器残差 {residue_direct}')
	
	plt.plot(w, mag)
	plt.xlabel('Frequency')
	plt.ylabel('Magnitude')
	plt.title(f'Magnitude Response of {name}')
	plt.show()
	
	# 导出系数
	real_indices = numpy.where(numpy.abs(residue_poles.imag) < 1e-6)
	real_poles = residue_poles[real_indices]
	complex_indices = numpy.where(residue_poles.imag >= 1e-6)
	complex_poles = residue_poles[complex_indices]

	# Calculate the coefficients for direct synthesis of the output
	real_coeffs_direct = residue_coeffs[real_indices]*1
	complex_coeffs_direct = residue_coeffs[complex_indices]*2 # doubled since we're only using one and taking the real part

	return (fpass, fstop, order, real_coeffs_direct, real_poles, complex_coeffs_direct, complex_poles, name)

t = []
t.append(DesignElliptic(20000, 25000, 0.1, 97.5, 'Fast'))
t.append(DesignElliptic(20000, 21700, 0.1, 200, 'Best'))
t.append(DesignChebyshev1(20000, 23000, 0.1, 120, 'Median'))

output_code ='''#pragma once
#include <array>
#include <complex>

namespace qwqdsp::fx::coeff{'''
for (fpass, fstop, order, real_coeffs, real_poles, complex_coeffs, complex_poles, name) in t:
	cppCode = """
template<typename Sample>
struct %sCoeffs {
	using TSample = Sample;
	static constexpr size_t complexCount = %i;
	static constexpr size_t realCount = %i;
	static constexpr size_t filterOrder = %i;
	static constexpr Sample fpass = Sample(%f);
	static constexpr Sample fstop = Sample(%f);
	static constexpr std::array<std::complex<Sample>, complexCount> complexPoles{{
		%s
	}};
	static constexpr std::array<Sample, realCount> realPoles{{
		%s
	}};
	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
	static constexpr std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
		%s
	}};
	static constexpr std::array<Sample, realCount> realCoeffsDirect{{
		%s
	}};
};
	"""%(
		name,
		len(complex_poles),
		len(real_poles),
		order,
		fpass,
		fstop,
		",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_poles]),
		",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_poles]),
		",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_coeffs]),
		",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_coeffs]),
	)
	output_code += cppCode


output_code += "}\n"

with open("resample_coeffs.h", 'w') as file:
	file.write(output_code)

# print("\nDesigning BLEP filters", file=sys.stderr)

# #------- Create the highpass filter

# (hp_z, hp_p, hp_k) = scipy.signal.butter(blep_order, highpass_freq, btype="highpass", analog=True, output='zpk');
# (hp_b, hp_a) = scipy.signal.zpk2tf(hp_z, hp_p, hp_k)
# hp_coeffs, hp_poles, hp_direct = scipy.signal.residue(hp_b, hp_a)

# #------- Create the impulse filter (elliptic lowpass)

# (impulse_z, impulse_p, impulse_k) = scipy.signal.ellip(filter_order, filter_ripple_db, filter_stop_db, lowpass_freq, analog=True, output='zpk')

# (impulse_b, impulse_a) = scipy.signal.zpk2tf(impulse_z, impulse_p, impulse_k)
# impulse_coeffs, impulse_poles, impulse_direct = scipy.signal.residue(impulse_b, impulse_a)
# if len(impulse_direct) > 0:
# 	exit(1)

# #------- Create the direct-output filter

# # filter_z = numpy.append(impulse_z, hp_z)
# # filter_p = numpy.append(impulse_p, hp_p)
# # filter_k = impulse_k*hp_k

# (filter_b, filter_a) = scipy.signal.zpk2tf(impulse_z, impulse_p, impulse_k)
# residue_coeffs, residue_poles, residue_direct = scipy.signal.residue(filter_b, filter_a)
# if len(residue_direct) > 0:
# 	exit(1)

# real_indices = numpy.where(numpy.abs(residue_poles.imag) < 1e-6)
# real_poles = residue_poles[real_indices]
# complex_indices = numpy.where(residue_poles.imag >= 1e-6)
# complex_poles = residue_poles[complex_indices]

# # Calculate the coefficients for direct synthesis of the output
# real_coeffs_direct = residue_coeffs[real_indices]*1
# complex_coeffs_direct = residue_coeffs[complex_indices]*2 # doubled since we're only using one and taking the real part

# #------- Subtract direct-output filter from the highpass to get the BLEP residue filter

# for i in range(len(hp_poles)):
# 	# Find the pole from the direct filter which matches this highpass one
# 	pole_distances = numpy.abs(residue_poles - hp_poles[i])
# 	pole_index = numpy.argmin(pole_distances)
# 	residue_coeffs[pole_index] -= hp_coeffs[i]

# # Calculate the coefficients for the aliasing-cancellation signal
# real_coeffs_blep = residue_coeffs[real_indices]*1
# complex_coeffs_blep = residue_coeffs[complex_indices]*2 # doubled since we're only using one and taking the real part

# #------- Add 0s into the impulse coeffs since it doesn't include the highpass

# impulse_coeffs_padded = residue_coeffs*0;
# for i in range(len(impulse_poles)):
# 	# Find the pole from the direct filter which matches this highpass one
# 	pole_distances = numpy.abs(residue_poles - impulse_poles[i])
# 	pole_index = numpy.argmin(pole_distances)
# 	impulse_coeffs_padded[pole_index] = impulse_coeffs[i]

# # Calculate the coefficients for the aliasing-cancellation signal
# real_coeffs_impulse = impulse_coeffs_padded[real_indices]*1
# complex_coeffs_impulse = impulse_coeffs_padded[complex_indices]*2 # doubled since we're only using one and taking the real part

# print("\tdone")

# #------- Generate C++
# print("\nWriting C++ code: out/coeffs.h")

# cppCode = """template<typename Sample>
# struct EllipticBlepCoeffs {
# 	static constexpr size_t maxIntegrals = %i;
# 	static constexpr size_t complexCount = %i;
# 	static constexpr int realCount = %i;
# 	std::array<std::complex<Sample>, complexCount> complexPoles{{
# 		%s
# 	}};
# 	std::array<Sample, realCount> realPoles{{
# 		%s
# 	}};
# 	// Coeffs for direct bandlimited synthesis of a polynomial-segment waveform
# 	std::array<std::complex<Sample>, complexCount> complexCoeffsDirect{{
# 		%s
# 	}};
# 	std::array<Sample, realCount> realCoeffsDirect{{
# 		%s
# 	}};
# 	// Coeffs for cancelling the aliasing from discontinuities in an existing waveform
# 	std::array<std::complex<Sample>, complexCount> complexCoeffsBlep{{
# 		%s
# 	}};
# 	std::array<Sample, realCount> realCoeffsBlep{{
# 		%s
# 	}};
# 	// Coeffs for adding an impulse in "residue" (not direct) mode
# 	std::array<std::complex<Sample>, complexCount> complexCoeffsImpulse{{
# 		%s
# 	}};
# 	std::array<Sample, realCount> realCoeffsImpulse{{
# 		%s
# 	}};
# 	// Allpass to make the phase approximately linear
# 	static constexpr int allpassLinearDelay = %i;
# 	static constexpr int allpassOrder = %i;
# 	std::array<Sample, allpassOrder> allpassCoeffs{{
# 		%s
# 	}};
# };"""%(
# 	blep_order,
# 	len(complex_poles),
# 	len(real_poles),
# 	",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_poles]),
# 	",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_poles]),
# 	",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_coeffs_direct]),
# 	",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_coeffs_direct]),
# 	",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_coeffs_blep]),
# 	",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_coeffs_blep]),
# 	",\n\t\t".join(["{Sample(%s), Sample(%s)}"%(p.real.astype(str), p.imag.astype(str)) for p in complex_coeffs_impulse]),
# 	",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in real_coeffs_impulse]),
# 	linear_delay,
# 	len(linear_coeffs) - 1,
# 	",\n\t\t".join(["Sample(%s)"%(p.real.astype(str)) for p in linear_coeffs[1:]]),
# )
# with open("out/coeffs.h", 'w') as file:
# 	file.write(cppCode)
