#ifndef MY_BUFFER_ADVICE_
#define MY_BUFFER_ADVICE_

namespace MyBuffer{

	template<typename Buffer>
	constexpr void adviceNeeded(Buffer const &, bool){
	}

	template<typename Buffer>
	struct AdviceNeededGuard{
		constexpr AdviceNeededGuard(Buffer &buffer) : buffer(buffer){
			adviceNeeded(buffer, true);
		}

		~AdviceNeededGuard(){
			adviceNeeded(buffer, false);
		}

	private:
		Buffer &buffer;
	};

} // namespace MyBuffer

#endif

