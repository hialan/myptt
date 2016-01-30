<?php
class MyByte{
    const ICONV_FROM_ENCODING="BIG5";
    const ICONV_TO_ENCODING="UTF-8";

    private $byte;
    public function __construct($byte) {
        $this->byte = $byte;
    }
    public function getByte() {
        return $this->byte;
    }
    public function getChar() {
        return chr($this->byte);
    }
    public function isBig5FirstByte() {
        return ($this->byte >= 129 && $this->byte <= 254);
    }
    public function isEsc() {
        return ($this->byte == 27);
    }
    public function toChinese($lastByte) {
        $ch1 = $this->byte;
        $ch2 = $lastByte;
        $bin = pack('C*', $ch1, $ch2);
        return iconv(static::ICONV_FROM_ENCODING, static::ICONV_TO_ENCODING, $bin);
    }
}

class ByteStream {
    private $bytes;
	public function __construct($str) {
        $this->bytes = unpack('C*', $str);
	}
    public function getc() {
        $byte = array_shift($this->bytes);
        return new MyByte($byte);
    }
    public function eof() {
        return (count($this->bytes) <= 0);
    }
}

?>
