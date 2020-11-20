#include "base.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter>
	struct cmd_EXIT : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) final{
			return WorkerStatus::DISCONNECT;
		}
	};

	template<class Protocol, class DBAdapter>
	struct cmd_SHUTDOWN : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) final{
			return WorkerStatus::SHUTDOWN;
		}
	};


}
}

