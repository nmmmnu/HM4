<?php

namespace AppBundle\AppBundle\Helper;

class RedisHelper{
	private $redis;

	function __construct($redis){
		$this->redis = $redis;
	}

	static function transformData(array & $data, $decr = 0){
		$result = array();

		$size = count($data) - $decr;

		for ($i = 0; $i < $size; ++$i)
			$result[$data[$i]] = $data[++$i];

		return $result;
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

	function rawCommand(){
		$args = func_get_args();

		switch( get_class($this->redis) ){
		case "Predis\Client"	: return $this->rawCommand_Predis($args);
		case "Redis"		: return $this->rawCommand_PHPRedis($args);
		case "TinyRedisClient"	:
		default			: return $this->rawCommand_MagicMethod($args);
		}
	}

	function getx($key, $page, $prefix = false){
		if (! $prefix)
			$prefix = $key;

		$data = $this->rawCommand("getx", $key, $page, $prefix);

		return [ self::transformData($data, 1), end($data) ];
	}

	function count($prefix, $page = 10){
		return $this->accumulate_("count", $prefix, $page);
	}

	function sum($prefix, $page = 10){
		return $this->accumulate_("sum",   $prefix, $page);
	}

	private function accumulate_($func, $prefix, $page){
		$key = $prefix;

		$acc = 0;

		while($key){
			$data = $this->rawCommand($func, $key, $page, $prefix);

			$acc += $data[0];
			$key  = $data[1];
		}

		return $acc;
	}
}

