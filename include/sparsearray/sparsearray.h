#ifndef MY_SPARSE_ARRAY_
#define MY_SPARSE_ARRAY_

#include <vector>
#include <limits>
#include <cstdint>
#include <cassert>

namespace mysparsearray{

	template<typename K, typename Controller, template<typename> typename Vector = std::vector>
	class SparseArray{
		static_assert(
			std::is_same_v<K, uint8_t>  ||
			std::is_same_v<K, uint16_t> ||
			std::is_same_v<K, uint32_t> ||
			std::is_same_v<K, uint64_t>
		);

		using size_type			= K;
		using key_type			= K;
		using mapped_type		= typename Controller::mapped_type;
		using value_type		= typename Controller::value_type;

		constexpr static key_type	MAX_SIZE = std::numeric_limits<key_type>::max();

		Vector<key_type		>	sparse_;
		Vector<mapped_type	>	dense_;
		size_type			capacity_; // we do not know if Vector::capacity() works

	public:
		constexpr SparseArray(size_type size, size_type capacity = 0) :
					capacity_(capacity ? capacity : size){

			assert(size <= MAX_SIZE);

			sparse_.assign (size, MAX_SIZE);
			dense_ .reserve(capacity_);
		}

	public:
		#if 0
		void print() const{
			for(size_type i = 0; i < sparse_.size(); ++i)
				if (sparse_[i] < dense_.size())
					printf("Sparse %10zu %10zu\n", size_t{i}, size_t{sparse_[i]});

			for(size_type i = 0; i < dense_.size(); ++i)
					printf("Dense  %10zu %10zu\n", size_t{i}, size_t{Controller::getKey(dense_[i])});
		}
		#endif

		constexpr size_type domain() const{
			return static_cast<size_type>(sparse_.size());
		}

		constexpr size_type capacity() const{
			return capacity_;
		}

	public:
		constexpr size_type size() const{
			return static_cast<size_type>(dense_.size());
		}

		constexpr auto const *data() const{
			return dense_.data();
		}

		constexpr auto       *data(){
			return dense_.data();
		}

		constexpr auto const &operator[](uint64_t index) const{
			return dense_[index];
		}

		constexpr auto       &operator[](uint64_t index){
			return dense_[index];
		}

	public:
		constexpr auto begin() const{
			return std::begin(dense_);
		}

		constexpr auto end() const{
			return std::end(dense_);
		}

		constexpr auto begin(){
			return std::begin(dense_);
		}

		constexpr auto end(){
			return std::end(dense_);
		}

	public:
		[[nodiscard]]
		constexpr bool exists(key_type key) const{
			if (key >= domain())
				return false;

			auto const index = sparse_[key];

			return index < size(); // index < MAX
		}

		[[nodiscard]]
		constexpr const auto *find(key_type key) const{
			if (!exists(key))
				return nullptr;

			auto const index = sparse_[key];

			return & Controller::getVal(dense_[index]);
		}

		[[nodiscard]]
		constexpr value_type *find(key_type key){
			if (!exists(key))
				return nullptr;

			auto const index = sparse_[key];

			return & Controller::getVal(dense_[index]);
		}

	public:
		template<typename... Ts>
		constexpr bool insert(key_type key, Ts &&...ts){
			return insertT_(key, std::forward<Ts>(ts)...);
		}

		constexpr bool insert(mapped_type const &value){
			return insertF_(value);
		}

		constexpr bool insert(mapped_type &&value){
			return insertF_(std::move(value));
		}

	public:
		constexpr bool remove(key_type key) {
			if (!exists(key))
				return false;

			auto const index = sparse_[key];

			if (index != size() - 1){
				// replace with the last element
				auto const keyLast = Controller::getKey(dense_.back());

				dense_ [index  ] = std::move(dense_.back());
				sparse_[keyLast] = index;
			}

			sparse_[key] = MAX_SIZE;

			dense_.pop_back();

			return true;
		}

	private:
		template<typename... Ts>
		constexpr bool insertT_(key_type key, Ts &&...ts){
			if (size() >= capacity())
				return false;

			if (key >= domain())
				return false;

			auto const index = sparse_[key];

			if (index < size()){ // index < MAX
				// update in place

				dense_[index] = { key, std::forward<Ts>(ts)... };

				return true;
			}else{
				// add new

				sparse_[key] = size();
				dense_.emplace_back(key, std::forward<Ts>(ts)...);

				return true;
			}
		}

		template<typename u_mapped_type>
		constexpr bool insertF_(u_mapped_type &&value){
			if (size() >= capacity())
				return false;

			auto const key = Controller::getKey(value);

			if (key >= domain())
				return false;

			auto const index = sparse_[key];

			if (index < size()){ // index < MAX
				// update in place

				dense_[index] = std::forward<u_mapped_type>(value);

				return true;
			}else{
				// add new

				sparse_[key] = size();
				dense_.emplace_back(std::forward<u_mapped_type>(value));

				return true;
			}
		}
	};

} // namespace mysparcemap

#endif

