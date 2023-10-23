<?php
/*
 * HM4 Helper for PHP
 *
 * Version	: 1.2.5
 * Date		: 2020-05-26
 *
 * Version	: 1.3.0
 * Date		: 2023-10-21
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

class HM4Helper{
	private $redis;

	const MAX_ACCUMULATE = 10000;

	function __construct($redis){
		$this->redis = $redis;
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

	// ==================

	function getx($key, $row_count, $prefix = NULL){
		return $this->collectKeyValues_("getx", $key, $row_count, $prefix ? $prefix : $key);
	}

	function xnget($key, $row_count, $prefix){
		return $this->collectKeyValues_("xnget", $key, $row_count, $prefix);
	}

	function xuget($key, $row_count){
		return $this->collectKeyValues_("xuget", $key, $row_count);
	}

	function xngetkeys($key, $row_count, $prefix){
		return $this->collectKeyValues_("xngetkeys", $key, $row_count, $prefix);
	}

	function xugetkeys($key, $row_count){
		return $this->collectKeyValues_("xugetkeys", $key, $row_count);
	}

	// ==================

	function count($prefix, $iterations){
		return $this->accumulate_("count", $prefix, $iterations);
	}

	function sum($prefix, $iterations){
		return $this->accumulate_("sum",   $prefix, $iterations);
	}

	// ==================

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

	// ==================

	private static function transformKeyValues__(array & $data){
		$result = array();

		// requires 1.3.7.5 where last key is always present!!!

		$size = count($data) - 1;

		for ($i = 0; $i < $size; ++$i)
			$result[$data[$i]] = $data[++$i];

		return $result;
	}

	private function collectKeyValues_(){
		$args = func_get_args();

		$data = call_user_func_array( [ $this, "rawCommand" ], $args );

		// requires 1.3.7.5 where last key is always present!!!

		return [ self::transformKeyValues__($data), end($data) ];
	}

	// ==================

	private function accumulate_($func, $prefix, $iterations){
		$i = 0;

		$key = $prefix;

		$acc = 0;

		while($key){
			list($value, $last) = $this->rawCommand($func, $key, self::MAX_ACCUMULATE, $prefix);
			$acc += $value;
			$key  = $last;

			if ($iterations && ++$i >= $iterations)
				break;
		}

		return [ $acc, $key ];
	}
}

