#include "base.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter>
	struct cmd_ : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
		}
	};



}
}

