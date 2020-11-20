#include "base.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter>
	struct cmd_GET : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			const auto &p = protocol.getParams();

			if (p.size() != 2)
				return error::BadRequest(protocol, buffer);

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			protocol.response_string(buffer, db.get(key));

			return WorkerStatus::WRITE;
		}
	};



}
}

