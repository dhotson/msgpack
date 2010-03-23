<?php

class MessagePack_Decode
{
	private $_input;
	private $_peeked = '';

	/**
	 * @param $input A file/stream like resource
	 */
	public function __construct($input)
	{
		$this->_input = $input;
	}

	public static function decode($string)
	{
		$decode = new self(fopen('data:application/octet-stream;base64,'.base64_encode($string), 'r'));
		return $decode->readAny();
	}

	public function readAny()
	{
		$val = $this->peek1();

		if ($val === 0xC0)
		{
			$this->read1();
			return null;
		}
		elseif ($val === 0xC2)
		{
			$this->read1();
			return false;
		}
		elseif ($val === 0xC3)
		{
			$this->read1();
			return true;
		}

		// todo - decode all other types

	}

	public function read($length)
	{
		if ($length < strlen($this->_peeked))
		{
			$result = substr($this->_peeked, 0, $length);
			$this->_peeked = substr($this->_peeked, $length);
			$length = 0;
		}
		else
		{
			$result = $this->_peeked;
			$this->_peeked = '';
			$length = $length - strlen($result);
		}


		if ($length > 0)
		{
			$tmp = fread($this->_input, $length - strlen($this->_peeked));
			$result .= $tmp;
		}

		return $result;
	}

	public function peek($length)
	{
		if ($length <= strlen($this->_peeked))
		{
			return substr($this->_peeked, 0, $length);
		}
		else
		{
			$readBytes = fread($this->_input, $length - strlen($this->_peeked));

			if ($readBytes)
				$this->_peeked .= $readBytes;

			return $this->_peeked;
		}
	}

	public function peek1()
	{
		return array_shift(unpack('C', $this->peek(1)));
	}

	public function read1()
	{
		return array_shift(unpack('C', $this->read(1)));
	}

	public function read2()
	{
		return array_shift(unpack('n', $this->read(2)));
	}

	public function read4()
	{
		return array_shift(unpack('N', $this->read(4)));
	}

	public function readString($length)
	{
		return $this->read($length);
	}


	//
	public function readFixnum()
	{
		$this->read1();
		return true;
	}



	private function _fail($str)
	{
		throw new Exception($str);
	}
}
