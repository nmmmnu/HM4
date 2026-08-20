#ifndef MY_FORCE_VECTORIZE_
#define MY_FORCE_VECTORIZE_

	#if defined(__clang__)
		#define FORCE_VECTORIZE _Pragma("clang loop vectorize(enable) interleave(enable)")
	#elif defined(__GNUC__)
		#define FORCE_VECTORIZE _Pragma("GCC ivdep")
//	#elif defined(_MSC_VER)
//		#define FORCE_VECTORIZE __pragma(loop(ivdep))
	#else
		#define FORCE_VECTORIZE
	#endif

#endif

