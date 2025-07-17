#ifndef HM4_VERSION_H_
#define HM4_VERSION_H_

namespace hm4{
	namespace version{
		using V = short unsigned int;

		constexpr const char *str = "1.3.12";

		constexpr int major	= 1;
		constexpr int minor	= 3;
		constexpr int revision	= 12;
		constexpr int build	= 0;

		constexpr int num	=	major		* 1000000	+
						minor		* 10000		+
						revision	* 100		+
						build		* 1
		;
	}
}

#endif

