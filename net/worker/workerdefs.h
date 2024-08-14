#ifndef _WORKER_DEFS_H
#define _WORKER_DEFS_H

#include <cstdint>

namespace net::worker{

	enum class WorkerStatus : uint8_t{
		PASS,
		READ,
		WRITE,
		DISCONNECT,
		DISCONNECT_ERROR,
		SHUTDOWN
	};



	struct ResultErrorMessages_EN{
		constexpr static std::string_view SYS_PROTOCOL_BREAK	= "General Bad Request / Not Implemented";

		constexpr static std::string_view SYS_NOT_IMPLEMENTED	= "Not Implemented";
		constexpr static std::string_view SYS_EMPTY_COMMAND	= "The command can not be empty";
		constexpr static std::string_view SYS_UNHANDLED		= "Unspecified error";



		// Duplication, but is very fast later.
		constexpr static std::string_view NEED_EXACT_PARAMS[]	= {
			"The command needs no parameters",
			"The command needs exactly 1 parameter",
			"The command needs exactly 2 parameters",
			"The command needs exactly 3 parameters",
			"The command needs exactly 4 parameters",
			"The command needs exactly 5 parameters",
			"The command needs exactly 6 parameters",
			"The command needs exactly 7 parameters",
			"The command needs exactly 8 parameters"
		};

		constexpr static std::string_view NEED_EXACT_PARAMS_0	= NEED_EXACT_PARAMS[0];
		constexpr static std::string_view NEED_EXACT_PARAMS_1	= NEED_EXACT_PARAMS[1];
		constexpr static std::string_view NEED_EXACT_PARAMS_2	= NEED_EXACT_PARAMS[2];
		constexpr static std::string_view NEED_EXACT_PARAMS_3	= NEED_EXACT_PARAMS[3];
		constexpr static std::string_view NEED_EXACT_PARAMS_4	= NEED_EXACT_PARAMS[4];
		constexpr static std::string_view NEED_EXACT_PARAMS_5	= NEED_EXACT_PARAMS[5];
		constexpr static std::string_view NEED_EXACT_PARAMS_6	= NEED_EXACT_PARAMS[6];
		constexpr static std::string_view NEED_EXACT_PARAMS_7	= NEED_EXACT_PARAMS[7];
		constexpr static std::string_view NEED_EXACT_PARAMS_8	= NEED_EXACT_PARAMS[8];



		constexpr static std::string_view NEED_EXACT_PARAMS_12	= "The command needs exactly 1 or 2 parameters";
		constexpr static std::string_view NEED_EXACT_PARAMS_23	= "The command needs exactly 2 or 3 parameters";
		constexpr static std::string_view NEED_EXACT_PARAMS_34	= "The command needs exactly 3 or 4 parameters";
		constexpr static std::string_view NEED_EXACT_PARAMS_45	= "The command needs exactly 4 or 5 parameters";
		constexpr static std::string_view NEED_EXACT_PARAMS_67	= "The command needs exactly 6 or 7 parameters";
		constexpr static std::string_view NEED_EXACT_PARAMS_234	= "The command needs exactly 2, 3 or 4 parameters";

		constexpr static std::string_view NEED_MORE_PARAMS_1	= "The command needs 1 or more parameters";
		constexpr static std::string_view NEED_MORE_PARAMS_2	= "The command needs 2 or more parameters";
		constexpr static std::string_view NEED_MORE_PARAMS_3	= "The command needs 3 or more parameters";
		constexpr static std::string_view NEED_MORE_PARAMS_4	= "The command needs 4 or more parameters";

		constexpr static std::string_view NEED_LESS_PARAMS_5	= "The command needs 5 or less parameters";



		constexpr static std::string_view NEED_GROUP_PARAMS[]	= {
			SYS_UNHANDLED,
			SYS_UNHANDLED,
			"The command needs 2 or more parameters in groups",
			"The command needs 3 or more parameters in groups",
			"The command needs 4 or more parameters in groups",
			"The command needs 5 or more parameters in groups",
			"The command needs 6 or more parameters in groups"
		};

		constexpr static std::string_view NEED_GROUP_PARAMS_2	= NEED_GROUP_PARAMS[2];
		constexpr static std::string_view NEED_GROUP_PARAMS_3	= NEED_GROUP_PARAMS[3];
		constexpr static std::string_view NEED_GROUP_PARAMS_4	= NEED_GROUP_PARAMS[4];
		constexpr static std::string_view NEED_GROUP_PARAMS_5	= NEED_GROUP_PARAMS[5];
		constexpr static std::string_view NEED_GROUP_PARAMS_6	= NEED_GROUP_PARAMS[6];



		constexpr static std::string_view EMPTY_KEY		= "The key can not be empty";
		constexpr static std::string_view EMPTY_VAL		= "The value can not be empty";
		constexpr static std::string_view EMPTY_NAME		= "The name can not be empty";
		constexpr static std::string_view EMPTY_PREFIX		= "The prefix can not be empty";
		constexpr static std::string_view EMPTY_ENDCOND		= "The end condition can not be empty";

		constexpr static std::string_view CONTAINER_CAPACITY	= "Container capacity error";
		constexpr static std::string_view INTERNAL_ERROR	= "Internal error";
		constexpr static std::string_view INVALID_PARAMETERS	= "Invalid parameters";
		constexpr static std::string_view INVALID_KEY_SIZE	= "Invalid key size";
	};

	using ResultErrorMessages = ResultErrorMessages_EN;



	template<class Protocol, class Buffer>
	WorkerStatus emitError(Protocol &protocol, Buffer &buffer, std::string_view const error){
		protocol.response_error(buffer, error);
		return WorkerStatus::WRITE;
	}

	template<class Protocol, class Buffer>
	WorkerStatus emitInternalError(Protocol &, Buffer &){
	//	return error_(protocol, buffer, "Internal Error");

		// here we should disconnect,
		// because the client is already broken and
		// will continue to push data
		return WorkerStatus::DISCONNECT_ERROR;
	}

} // namespace

#endif

