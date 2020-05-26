<?php
/*
 * Redis Helper for PHP
 *
 * Version	: 1.2.5
 * Date		: 2020-05-26
 *
 * It allows:
 * - Isolate $redis->rawCommand / $redis->executeRaw from underline redis class.
 * - Fix / Tansform results in key => value way.
 * - Accumulate count / sum / min / max into single command.
 * - Make PHP code more readable
 *
 * Currently supported Redis classes:
 * - PHP Redis		https://github.com/phpredis/phpredis
 * - Predis		https://github.com/nrk/predis
 * - TinyRedisClient	https://github.com/ptrofimov/tinyredisclient
 *
 */

namespace AppBundle\AppBundle\Helper;

class RedisHelper{
	private $redis;

	const MAX_ACCUMULATE = 10000;

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

	function getx($key, $page, $prefix = NULL){
		if (! $prefix)
			$prefix = $key;

		$data = $this->rawCommand("getx", $key, $page, $prefix);

		return [ self::transformData($data, 1), end($data) ];
	}

	function count($prefix, $iterations){
		return $this->accumulate_("count", $prefix, $iterations);
	}

	function sum($prefix, $iterations){
		return $this->accumulate_("sum",   $prefix, $iterations);
	}

	function min($prefix, $iterations){
		return $this->accumulate_("min",   $prefix, $iterations);
	}

	function max($prefix, $iterations){
		return $this->accumulate_("max",   $prefix, $iterations);
	}

	private function accumulate_($func, $prefix, $iterations){
		$i = 0;

		$key = $prefix;

		$acc = 0;

		while($key){
			$data = $this->rawCommand($func, $key, self::MAX_ACCUMULATE, $prefix);
			$acc += $data[0];
			$key  = $data[1];

			if ($iterations && ++$i >= $iterations)
				break;
		}

		return [ $acc, $key ];
	}
}

