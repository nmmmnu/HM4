#include "base.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter>
	struct cmd_SAVE : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			db.save();

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};

	template<class Protocol, class DBAdapter>
	struct cmd_RELOAD : cmd_base<Protocol, DBAdapter>{
		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			db.reload();

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};



}
}

