# Transitive FFT credit

AudioFFT.cpp contains the Ooura-derived portable radix4 transform, credited in
its source. Original FFT Package: Takuya Ooura,
https://www.kurims.kyoto-u.ac.jp/~ooura/fft.html (license checked2026-11-17).

> Copyright Takuya OOURA, 1996-2001
>
> You may use, copy, modify and distribute this code for any purpose (include
> commercial use) and without fee. Please refer to this package when you modify
> this code.

HiFi-LoFi's adaptation is distributed under the accompanying MIT COPYING.txt.
The six selected upstream files are pinned; Utilities.h has one documented empty-buffer memset guard for UBSan. Nonempty processing arithmetic is unchanged. No FFTW3 or
Apple Accelerate backend is enabled. FFTCONVOLVER_DONT_USE_SSE selects scalar
portable processing; AudioFFT's default Ooura implementation uses double FFT
work buffers internally (important for future M7 cost, not float-only math).
