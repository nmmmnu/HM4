<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$redis->del("a");

var_dump($redis->incr("a"));
var_dump($redis->incr("a"));
var_dump($redis->incr("a"));
var_dump($redis->incr("a"));
var_dump($redis->incr("a"));

$redis->del("a");

