<?php

class RedisHelperBF{
	private $redis;

	function __construct($redis){
		$this->redis = $redis;
	}

	private function rawCommand_Predis($args){
		return $this->redis->executeRaw($args);
	}

	private function rawCommand_PHPRedis($args){
		return call_user_func_array( [ $this->redis, "rawCommand" ], $args );
	}

	private function rawCommand_MagicMethod($args){
		$cmd  = $args[0];
		$args = array_slice($args, 1);

		return call_user_func_array( [ $this->redis, $cmd ], $args );
	}

	function rawCommandArray($args){
		switch( get_class($this->redis) ){
		case "Predis\Client"	: return $this->rawCommand_Predis($args);
		case "Redis"		: return $this->rawCommand_PHPRedis($args);
		case "TinyRedisClient"	:
		default			: return $this->rawCommand_MagicMethod($args);
		}
	}

	function rawCommand(){
		$args = func_get_args();

		return $this->rawCommandArray($args);
	}

	private function getLambda_(){
		$data = array_reverse(
			func_get_args()
		);

		return function() use ($data){
			$args = func_get_args();

			$cmd  = $args[0];
			array_shift($args);

			foreach($data as $d)
				array_unshift($args, $d);

			array_unshift($args, $cmd);

			return $this->rawCommandArray($args);
		};
	}

	function getBF($key, $bits, $hf){
		return $this->getLambda_($key, $bits, $hf);
	}

	function getCMS($key, $bits, $hf, $counter_type){
		return $this->getLambda_($key, $bits, $hf, $counter_type);
	}
}

/*
$redis = new Redis();
$redis->connect("127.0.0.1");

$rh = new RedisHelperBF($redis);

$bf = $rh->getBF("a", 4096, 2);

$bf("BFADD", "London");
$bf("BFADD", "Sofia");

print_r(
	$bf("BFMEXISTS", "London", "Sofia", "Berlin")
);

$cms = $rh->getCMS("b", 4096, 4, 8);

$cms("CMSADD", "London", 1);
$cms("CMSADD", "Sofia", 1, "London", 9);

print_r(
	$cms("CMSMCOUNT", "London", "Sofia", "Berlin")
);
*/
