#ifndef HM4_VERSION_H_
#define HM4_VERSION_H_

namespace hm4{
	namespace version{
		constexpr const char *str = "1.4.0";

		constexpr int major	= 1;
		constexpr int minor	= 4;
		constexpr int revision	= 0;
		constexpr int build	= 0;

		constexpr int num	=	major		* 1000000	+
						minor		* 10000		+
						revision	* 100		+
						build		* 1
		;
	}
}

#endif

