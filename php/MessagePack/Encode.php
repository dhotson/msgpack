<?php

class MessagePack_Encode
{
	private $_out;

	public function __construct($out)
	{
		$this->_out = $out;
	}

	public static function encode($obj)
	{
		$fp = fopen('php://memory', 'rw');
		$encode = new self($fp);
		$encode->_writeAny($obj);
		rewind($fp);
		return stream_get_contents($fp);
	}

	public function write($obj)
	{
		$this->_writeAny($obj);
	}

	// -- \

	private function _writeAny($obj)
	{
		if ($obj === true)
			return $this->_writeTrue($obj);
		elseif ($obj === false)
			return $this->_writeFalse($obj);
		elseif ($obj === null)
			return $this->_writeNull($obj);
		elseif (is_integer($obj))
			return $this->_writeInteger($obj);
		elseif (is_float($obj))
			return $this->_writeFloat($obj);
		elseif ($this->_isMap($obj))
			return $this->_writeMap($obj);
		elseif (is_array($obj))
			return $this->_writeArray($obj);
		elseif (is_string($obj))
			return $this->_writeRaw($obj);
		else
			$this->_fail($obj);
	}

	private function _write1($byte)
	{
		fwrite($this->_out, pack('C', $byte));
	}

	private function _write2($short)
	{
		fwrite($this->_out, pack('n', $short));
	}

	private function _write4($long)
	{
		fwrite($this->_out, pack('N', $long));
	}

	private function _writeString($string)
	{
		fwrite($this->_out, $string);
	}

	private function _writeTrue()
	{
		$this->_write1(0xc3);
	}

	private function _writeFalse()
	{
		$this->_write1(0xc2);
	}

	private function _writeNull()
	{
		$this->_write1(0xc0);
	}

	private function _writeInteger($num)
	{
		// encode 1 byte fixnum
		if ($num >= -32 && $num <= 127)
		{
			$this->_write1($num & 0xFF);
		}
		// encode unsigned int
		elseif ($num >= 0 && $num <= 255)
		{
			$this->_write1(0xCC);
			$this->_writeString(pack('C', $num));
		}
		elseif ($num >= 0 && $num <= 65535)
		{
			$this->_write1(0xCD);
			$this->_writeString(pack('n', $num));
		}
		elseif ($num >= 0 && $num <= 4294967295)
		{
			$this->_write1(0xCE);
			$this->_writeString(pack('N', $num));
		}
		elseif ($num >= 0 && $num <= PHP_INT_MAX)
		{
			$this->_write1(0xCF);
			$this->_write1(($num >> 56) & 0xFF);
			$this->_write1(($num >> 48) & 0xFF);
			$this->_write1(($num >> 40) & 0xFF);
			$this->_write1(($num >> 32) & 0xFF);
			$this->_write1(($num >> 24) & 0xFF);
			$this->_write1(($num >> 16) & 0xFF);
			$this->_write1(($num >> 8) & 0xFF);
			$this->_write1($num & 0xFF);
		}

		// encode signed int
		elseif ($num >= -128 && $num <= 128)
		{
			$this->_write1(0xD0);
			$this->_writeString(pack('c', $num));
		}
		elseif ($num >= -32768 && $num <= 32767)
		{
			$this->_write1(0xD1);
			$this->_write1(($num >> 8) & 0xFF);
			$this->_write1($num & 0xFF);
		}
		elseif ($num >= -2147483648 && $num <= 2147483647)
		{
			$this->_write1(0xD2);
			$this->_write1(($num >> 24) & 0xFF);
			$this->_write1(($num >> 16) & 0xFF);
			$this->_write1(($num >> 8) & 0xFF);
			$this->_write1($num & 0xFF);
		}
		else
		{
			$this->_write1(0xD3);
			$this->_write1(($num >> 56) & 0xFF);
			$this->_write1(($num >> 48) & 0xFF);
			$this->_write1(($num >> 40) & 0xFF);
			$this->_write1(($num >> 32) & 0xFF);
			$this->_write1(($num >> 24) & 0xFF);
			$this->_write1(($num >> 16) & 0xFF);
			$this->_write1(($num >> 8) & 0xFF);
			$this->_write1($num & 0xFF);
		}
	}

	private function _writeFloat($num)
	{
		$this->_write1(0xCB);

		// Check machine endianess
		if (bin2hex(pack('s', 0xFF00)) == '00ff')
			$this->_writeString(strrev(pack('d', $num)));
		else
			$this->_writeString(pack('d', $num));
	}

	private function _writeMap($arr)
	{
		$len = count($arr);

		if ($len < 16)
		{
			$this->_write1(128 | $len);
			foreach ($arr as $k => $v)
			{
				$this->_writeAny($k);
				$this->_writeAny($v);
			}
		}
		else if ($len < 65535)
		{
			$this->_write1(0xDE);
			$this->_write2($len);
			foreach ($arr as $k => $v)
			{
				$this->_writeAny($k);
				$this->_writeAny($v);
			}
		}
		else
		{
			$this->_write1(0xDF);
			$this->_write4($len);
			foreach ($arr as $k => $v)
			{
				$this->_writeAny($k);
				$this->_writeAny($v);
			}
		}
	}

	private function _writeArray($arr)
	{
		$len = count($arr);
		if ($len < 16)
		{
			$this->_write1(144 | $len);
			foreach ($arr as $v)
				$this->_writeAny($v);
		}
		else if ($len < 65535)
		{
			$this->_write1(0xDA);
			$this->_write2($len);
			foreach ($arr as $v)
				$this->_writeAny($v);
		}
		else
		{
			$this->_write1(0xDB);
			$this->_write4($len);
			foreach ($arr as $i)
				$this->_writeAny($v);
		}
	}

	private function _writeRaw($data)
	{
		$len = strlen($data);
		if ($len < 32)
		{
			$this->_write1(160 | $len);
			$this->_writeString($data);
		}
		else if ($len < 65535)
		{
			$this->_write1(0xDA);
			$this->_write2($len);
			$this->_writeString($data);
		}
		else
		{
			$this->_write1(0xDB);
			$this->_write4($len);
			$this->_writeString($data);
		}
	}

	private function _fail($str)
	{
		throw new Exception("Cannot encode to erlang external format: $str");
	}

	// Check if array is associative or not.. stolen from a comment on php.net
	private static function _isMap($array)
	{
		return (is_array($array) && 0 !== count(array_diff_key($array, array_keys(array_keys($array)))));
	}
}
