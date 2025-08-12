#ifndef POLICY_BUFFER_VIEW_
#define POLICY_BUFFER_VIEW_

#include <type_traits>

namespace MyBuffer{

	namespace AccessorPolicies{

		struct Standard{
			template<typename T>
			constexpr static T get(T const &x){
				return x;
			}

			template<typename T>
			constexpr static void set(T &x, const T &val){
				x = val;
			}
		};

	} // namespace Accessors



	template<typename T, typename AccessorPolicy = AccessorPolicies::Standard>
	struct PolicyBufferView{
		using value_type	= T;
		using size_type		= std::size_t;

		template<typename U,
				std::enable_if_t<
						// T is const		and
						// T is same as U	and
						// U may not be const
						std::is_const_v<T> &&
						std::is_same_v<T, U const>,
				int> = 0
		>
		constexpr PolicyBufferView(PolicyBufferView<U, AccessorPolicy> const &buffer) noexcept :
					PolicyBufferView( buffer.data_, buffer.size_ ){}

		constexpr PolicyBufferView(value_type *data, size_type size) noexcept :
					data_(data),
					size_(size){}

		constexpr operator bool() const noexcept{
			return data_;
		}

		constexpr auto operator[](size_type const index) const noexcept{
			return Proxy<T const>{ data_[index] };
		}

		constexpr
		auto operator[](size_type const index) noexcept{
			return Proxy<T>{ data_[index] };
		}

		constexpr size_type size() const noexcept{
			return size_;
		}

	private:
		template<typename U, typename UAccessor>
		friend class PolicyBufferView;

	private:
		template<typename U>
		class Proxy{
			U		&ref;
			AccessorPolicy	ap;

		public:
			constexpr Proxy(U &ref) : ref(ref){}

			constexpr operator U() const noexcept{
				return ap.get(ref);
			}

			constexpr Proxy &operator=(U value) noexcept {
				ap.set(ref, value);
				return *this;
			}
		};

	private:
		value_type	*data_	= nullptr;
		size_type	size_	= 0;
	};

} // namespace MyBuffer



#endif	// ifndef POLICY_BUFFER_VIEW_


