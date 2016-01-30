<?php
require_once 'Telnet.class.php';
require_once 'ByteStream.inc';
require_once 'Term.inc';
require_once 'ansi_parser.inc';

class Client {
    private $telnet;
    private $term;
    private $parser;

    public function __construct($host, $port) {
        $this->term = new TermBuf(80, 24);
        $this->parser = new ansi_parser($this->term);
        $this->telnet = new Telnet($host, $port, 10, '');
    }

    public function exec($str, $doReturn = true) {
        $data = $this->telnet->exec($str, $doReturn);
        $this->parser->feed($data);
    }

    public function getScreen() {
        return $this->term->show();
    }
    
    public function disconnect() {
        return $this->telnet->disconnect();
    }
}

?>