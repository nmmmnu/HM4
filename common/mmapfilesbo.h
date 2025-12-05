#ifndef MMAP_FILE_SBO_H_
#define MMAP_FILE_SBO_H_

#include "mmapfile.h"
#include "slaballocator.h"

#include <cstdint>
#include <variant>

struct MMAPFileSBO{
	using value_type = void;
	using size_type  = size_t;

	constexpr static size_t SmallSize = 2048;

	using VMAllocator = MyAllocator::SlabAllocator<SmallSize>;
	struct NoVMAllocator{};

	bool open(VMAllocator &allocator, std::string_view filename, MMAPFile::Advice advice = MMAPFile::Advice::NORMAL);

	bool open(NoVMAllocator,          std::string_view filename, MMAPFile::Advice advice = MMAPFile::Advice::NORMAL){
		impl_.emplace<MMAPFile>();

		return std::get<MMAPFile>(impl_).open(filename, advice);
	}

	bool open(VMAllocator *allocator, std::string_view filename, MMAPFile::Advice advice = MMAPFile::Advice::NORMAL){
		if (allocator)
			return open(*allocator,      filename, advice);
		else
			return open(NoVMAllocator{}, filename, advice);
	}

	void close(){
		impl_.emplace<ClosedFile>();
	}

public:
	constexpr operator bool() const{
		auto visitor = [](auto const &x) -> bool{
			return x;
		};

		return std::visit(visitor, impl_);
	}

	constexpr size_t size() const{
		auto visitor = [](auto const &x){
			return x.size();
		};

		return std::visit(visitor, impl_);
	}

	constexpr const void *data() const{
		auto visitor = [](auto const &x){
			return x.data();
		};

		return std::visit(visitor, impl_);
	}

private:
	struct ClosedFile{
		constexpr void close(){
		}

		constexpr operator bool() const noexcept{
			return false;
		}

		constexpr const void *data() const noexcept{
			return nullptr;
		}

		constexpr size_t size() const noexcept{
			return 0;
		}
	};

	struct SBOFile{
		SBOFile(char *data, size_t size, VMAllocator &allocator) :
						data_		(data		),
						size_		(size		),
						allocator_	(& allocator	){}

		SBOFile(SBOFile &&other) noexcept :
				data_		( std::move(other.data_		)),
				size_		( std::move(other.size_		)),
				allocator_	( std::move(other.allocator_	)){

			other.data_		= nullptr;
			other.allocator_	= nullptr;
		}

		~SBOFile(){
			deallocate_();
		}

		void close(){
			if (deallocate_()){
				data_		= nullptr;
				allocator_	= nullptr;
			}
		}

		constexpr operator bool() const noexcept{
			return allocator_ && data_;
		}

		constexpr const void *data() const noexcept{
			return data_;
		}

		constexpr size_t size() const noexcept{
			return size_;
		}

	private:
		bool deallocate_(){
			if (allocator_ && data_){
				MyAllocator::deallocate(allocator_, data_);
				return true;
			}

			return false;
		}

	private:
		char		*data_		= nullptr;
		size_t		size_		= 0;
		VMAllocator	*allocator_	= nullptr;
	};

	using Implementation = std::variant<
		ClosedFile,
		SBOFile,
		MMAPFile
	>;

private:
	Implementation	impl_{ std::in_place_type<ClosedFile> };
};

#endif

