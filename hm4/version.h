#ifndef HM4_VERSION_H_
#define HM4_VERSION_H_

namespace hm4{
	namespace version{
		constexpr const char *str = "1.2.15";

		constexpr int major	= 1;
		constexpr int minor	= 2;
		constexpr int revision	= 15;
		constexpr int build	= 1;

		constexpr int num	=	major		* 100000	+
						minor		* 1000		+
						revision	* 10		+
						build		* 1
		;
	}
}

#endif

