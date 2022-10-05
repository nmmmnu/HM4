#ifndef MY_PM_ALLOCATOR_H
#define MY_PM_ALLOCATOR_H

#include "baseallocator.h"

namespace MyAllocator{

	struct PMAllocator{
		const char *getName(){
			return getName_();
		}

		void *xallocate(std::size_t const size){
			return allocate_(size);
		}

		void xdeallocate(void *p){
			return deallocate_(p);
		}

		bool owns(const void *p) const{
			return owns_(p);
		}

		bool reset(){
			return reset_();
		}

		bool need_deallocate() const{
			return need_deallocate_();
		}

		bool knownMemoryUsage() noexcept{
			return knownMemoryUsage_();
		}

		std::size_t getFreeMemory() const{
			return getFreeMemory_();
		}

		std::size_t getUsedMemory() const{
			return getUsedMemory_();
		}

		virtual ~PMAllocator(){}

	private:
		virtual const char *getName_() const = 0;

		virtual void *allocate_(std::size_t) = 0;
		virtual void deallocate_(void *p) = 0;
		virtual bool reset_() = 0;

		virtual bool owns_(const void *) const = 0;

		virtual bool need_deallocate_() const = 0;
		virtual bool knownMemoryUsage_() const = 0;

		virtual std::size_t getFreeMemory_() const = 0;
		virtual std::size_t getUsedMemory_() const = 0;
	};



	template<typename T>
	inline auto wrapInSmartPtr(PMAllocator &allocator, T *p) noexcept{
		auto deleter = [&](void *p){
			allocator.xdeallocate(p);
		};

		return std::unique_ptr<T, decltype(deleter)>{
			p,
			deleter
		};
	}



	template<class Allocator>
	struct PMOwnerAllocator : PMAllocator{

		template<class ...Args>
		PMOwnerAllocator(Args &&...args) : allocator( std::forward<Args>(args)... ){}

	private:
		void *allocate_(std::size_t const size) override final{
			return allocator.xallocate(size);
		}

		void deallocate_(void *p) override final{
			return allocator.xdeallocate(p);
		}

		bool owns_(const void *p) const override final{
			return allocator.owns(p);
		}

		bool need_deallocate_() const override final{
			return allocator.need_deallocate();
		}

		bool reset_() override final{
			return allocator.reset();
		}

		bool knownMemoryUsage_() const override final{
			return allocator.knownMemoryUsage();
		}

		std::size_t getFreeMemory_() const override final{
			return allocator.getFreeMemory();
		}

		std::size_t getUsedMemory_() const override final{
			return allocator.getUsedMemory();
		}

		const char *getName_() const override final{
			return allocator.getName();
		}

	private:
		Allocator allocator;
	};



	template<class Allocator>
	struct PMLinkedAllocator : PMAllocator{
		PMLinkedAllocator(Allocator &allocator) : PMLinkedAllocator( &allocator){}

		PMLinkedAllocator(Allocator *allocator) : allocator(allocator){}

	private:
		void *allocate_(std::size_t const size) override final{
			return allocator->allocate(size);
		}

		void deallocate_(void *p) override final{
			return allocator->deallocate_(p);
		}

		bool owns_(const void *p) const override final{
			return allocator->owns(p);
		}

		bool need_deallocate_() const override final{
			return allocator->need_deallocate_();
		}

		bool knownMemoryUsage_() const override final{
			return allocator->getFreeMemory();
		}

		std::size_t getFreeMemory_() const override final{
			return allocator->getFreeMemory();
		}

		std::size_t getUsedMemory_() const override final{
			return allocator->getUsedMemory();
		}

		const char *getName_() const override final{
			return allocator->getName();
		}

	private:
		Allocator *allocator;
	};

} // namespace MyAllocator

#endif

