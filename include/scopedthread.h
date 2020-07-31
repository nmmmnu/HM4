#ifndef SCOPED_THREAD_H_
#define SCOPED_THREAD_H_

#include <thread>

class ScopedThread{
public:
	template<class ...Args>
	explicit ScopedThread(Args &&...args) : thread_(std::forward<Args>(args)...){}

	ScopedThread(ScopedThread &&other){
		thread_ = std::move(other.thread_);
	}

	ScopedThread &operator=(ScopedThread &&other){
		thread_ = std::move(other.thread_);
		return *this;
	}

	std::thread &operator*(){
		return thread_;
	}

	std::thread const &operator*() const{
		return thread_;
	}

	std::thread *operator->(){
		return & operator*();
	}

	std::thread const *operator->() const{
		return & operator*();
	}

	auto get_id() const{
		return thread_.get_id();
	}

	auto join(){
		if (thread_.joinable())
			thread_.join();
	}

	~ScopedThread(){
		join();
	}

private:
	std::thread thread_;
};

#endif

