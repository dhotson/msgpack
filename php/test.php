<?php

require_once 'MessagePack/Encode.php';

$msgpack = MessagePack_Encode::encode(null);

foreach (unpack('C*', $msgpack) as $byte)
{
	echo str_pad(decbin($byte), 8, '0', STR_PAD_LEFT) ." ";
}


echo "\n";

foreach (unpack('C*', $msgpack) as $byte)
{
	echo str_pad(dechex($byte), 8, ' ', STR_PAD_LEFT) ." ";
}

echo "\n";

foreach (unpack('C*', $msgpack) as $byte)
{
	echo str_pad($byte, 8, ' ', STR_PAD_LEFT) ." ";
}

echo "\n";


