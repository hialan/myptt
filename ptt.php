<?php
require_once 'Telnet.class.php';
require_once 'ByteStream.inc';
require_once 'Term.inc';

$config = parse_ini_file("config.ini", true);
$config = $config['global'];

class Processer {
    private $term;

    public function __construct() {
        $this->term = new TermBuf(80, 24);
    }

    public function process_input($buf) {
        $byteStream = new ByteStream($buf);
        $result = '';	
        $state = 'text';
        while(!$byteStream->eof()) {
            $c = $byteStream->getc();
            if($state == 'text') {
                if($c->isEsc()) {
                    $state = 'esc';
                    $result .= $c->getChar();
                } else if ($c->isBig5FirstByte()){
                    $state = 'chinese';
                    $ch1 = $c;
                } else {
                    $result .= $c->getChar();
                }
            } else if($state == 'chinese') {
                $control = '';
                if($c->isEsc()) {
                    while($c->getChar() != 'm') {
                        $control .= $c->getChar();
                        $c = $byteStream->getc();
                    }
                    $control .= $c->getChar();
                    $c = $byteStream->getc();
                }
                $state = 'text';
                $ch2 = $c;
                $utf8 = $ch1->toChinese($ch2->getByte());
                $result .= $utf8;
                if(!empty($control)) {
                    $result .= $control;
                }
            } else if($state == 'esc') {
                $b = $c->getByte();
                if($b == ord('m') || $b == ord('K') || $b == ord('H')) {
                    $state = 'text';
                }
                //error_log(chr($c), 3, './log.txt');
                $result .= $c->getChar();
            }
        }
        return $result;
    }
}

$proc = new Processer();

$client = new Telnet('ptt.cc', 23, 10, '');
$data = $client->exec('', false);
echo $proc->process_input($data);

$data = $client->exec($config['user']);
echo $proc->process_input($data);

$data .= $client->exec($config['password']);
echo $proc->process_input($data);


$data = $client->exec('');
echo $proc->process_input($data);
sleep(1);

//return;

/*
$data = $client->exec('n');
echo process_input($data);
sleep(1);
*/

$data = $client->exec('sGossiping');
$screen = $proc->process_input($data);
echo $screen;
sleep(1);

if(mb_strpos($screen, '請按任意鍵繼續')) {
	$data = $client->exec('');
	$screen = $proc->process_input($data);
	echo $screen;
	sleep(1);
}

$data = $client->exec('Z100');
echo $proc->process_input($data);
sleep(1);

$client->disconnect();
