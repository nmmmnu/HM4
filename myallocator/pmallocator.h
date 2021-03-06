#ifndef MY_PM_ALLOCATOR_H
#define MY_PM_ALLOCATOR_H

#include <cstddef>
#include <memory>

namespace MyAllocator{

	struct PMAllocator{
		const char *getName(){
			return getName_();
		}

		void *allocate(std::size_t const size){
			return allocate_(size);
		}

		void deallocate(void *p){
			return deallocate_(p);
		}

		bool reset(){
			return reset_();
		}

		bool need_deallocate() const{
			return need_deallocate_();
		}

		std::size_t getFreeMemory() const{
			return getFreeMemory_();
		}

		std::size_t getUsedMemory() const{
			return getUsedMemory_();
		}

		virtual ~PMAllocator(){}

		template<typename T>
		auto wrapInSmartPtr(T *p) noexcept{
			auto deleter = [this](void *p){
				deallocate(p);
			};

			return std::unique_ptr<T, decltype(deleter)>{
				p,
				deleter
			};
		}

	private:
		virtual const char *getName_() const = 0;

		virtual void *allocate_(std::size_t) = 0;
		virtual void deallocate_(void *p) = 0;
		virtual bool reset_() = 0;

		virtual bool need_deallocate_() const = 0;

		virtual std::size_t getFreeMemory_() const = 0;
		virtual std::size_t getUsedMemory_() const = 0;
	};



	template<class Allocator>
	struct PMOwnerAllocator : PMAllocator{

		template<class ...Args>
		PMOwnerAllocator(Args &&...args) : allocator( std::forward<Args>(args)... ){}

	private:
		inline void *allocate_(std::size_t const size) override final{
			return allocator.allocate(size);
		}

		inline void deallocate_(void *p) override final{
			return allocator.deallocate(p);
		}

		inline bool need_deallocate_() const override final{
			return allocator.need_deallocate();
		}

		inline bool reset_() override final{
			return allocator.reset();
		}

		inline std::size_t getFreeMemory_() const override final{
			return allocator.getFreeMemory();
		}

		inline std::size_t getUsedMemory_() const override final{
			return allocator.getUsedMemory();
		}

		inline const char *getName_() const override final{
			return allocator.getName();
		}

	private:
		Allocator allocator;
	};



	template<class Allocator>
	struct PMNonOwnerAllocator : PMAllocator{
		PMNonOwnerAllocator(Allocator &allocator) : PMNonOwnerAllocator( &allocator){}

		PMNonOwnerAllocator(Allocator *allocator) : allocator(allocator){}

	private:
		inline void *allocate_(std::size_t const size) override final{
			return allocator->allocate(size);
		}

		inline void deallocate_(void *p) override final{
			return allocator->deallocate_(p);
		}

		inline bool need_deallocate_() const override final{
			return allocator->need_deallocate_();
		}

		inline std::size_t getFreeMemory_() const override final{
			return allocator->getFreeMemory();
		}

		inline std::size_t getUsedMemory_() const override final{
			return allocator->getUsedMemory();
		}

		inline const char *getName_() const override final{
			return allocator->getName();
		}

	private:
		Allocator *allocator;
	};

} // namespace MyAllocator

#endif

