#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "workerdefs.h"

namespace net{
namespace worker{

template<class PROTOCOL, class DB_ADAPTER>
class KeyValueWorker{
public:
	KeyValueWorker(DB_ADAPTER &db) : db_(db){}

	template<class CONNECTION>
	WorkerStatus operator()(CONNECTION &buffer);

private:
	PROTOCOL	protocol_;
	DB_ADAPTER	&db_;
};


} // namespace worker
} // namespace


// ==================================

#include "keyvalueworker_impl.h"

#endif
