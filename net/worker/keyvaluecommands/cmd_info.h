#include "base.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter>
	struct cmd_INFO : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			protocol.response_string(buffer, db.info());

			return WorkerStatus::WRITE;
		}
	};



}
}

