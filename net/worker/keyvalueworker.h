#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "workerdefs.h"
#include "iobuffer.h"

namespace net{
namespace worker{

template<class Protocol, class DBAdapter>
class KeyValueWorker{
public:
	KeyValueWorker(DBAdapter &db) : db_(db){}

	WorkerStatus operator()(IOBuffer &buffer);

private:
	Protocol	protocol_;
	DBAdapter	&db_;
};


} // namespace worker
} // namespace


// ==================================

#include "keyvalueworker.h.cc"

#endif
