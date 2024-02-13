
	struct OutputBlob{
		constexpr static size_t ContainerSize	= 0xFFFF;

		using Container		= StaticVector<std::string_view		, ContainerSize>;	// 1024 KB, if string_view is 16 bytes
		using PairContainer	= StaticVector<const hm4::Pair *	, ContainerSize>;	//  514 KB
		using BufferContainer	= StaticVector<to_string_buffer_t	, ContainerSize>;	// 4096 KB

		constexpr static size_t ArenaBufferSize	=
						8 + sizeof(Container		) +
						8 + sizeof(PairContainer	) +
						8 + sizeof(BufferContainer	)
		;

		using ArenaBuffer	= MyBuffer::AllocatedByteBufferOwned<>;
		using ArenaAllocator	= MyAllocator::ArenaAllocator;

		void resetAllocator(){
			allocator_.reset();
		}

		auto &getAllocator(){
			return allocator_;
		}

		auto const &getAllocator() const{
			return allocator_;
		}

		auto &container(){
			return allocateStandardContainer_<Container>();
		}

		auto &pcontainer(){
			return allocateStandardContainer_<PairContainer>();
		}

		auto &bcontainer(){
			return allocateStandardContainer_<BufferContainer>();
		}

	private:
		template<class T>
		T &allocateStandardContainer_(){
			// When using *normally* the allocation *should* always succeed.
			auto &container = *MyAllocator::allocate<T>(allocator_);
			container.clear();
			return container;
		}

	private:
		ArenaBuffer	buffer_{ ArenaBufferSize };
		ArenaAllocator	allocator_{ buffer_ };

		static_assert(
			ArenaBufferSize >=
				8 + sizeof(Container		) +
				8 + sizeof(PairContainer	) +
				8 + sizeof(BufferContainer	)
		);
	};


