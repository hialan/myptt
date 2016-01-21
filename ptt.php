<?php
require_once 'Telnet.class.php';

$config = parse_ini_file("config.ini", true);
$config = $config['global'];

define('REMOTE_ENCODING', 'BIG5');
define('LOCAL_ENCODING', 'UTF-8');
define('ESC', chr(27));


function process_input($buf) {
	$bytes = unpack('C*', $buf);
	$length = count($bytes);
	$result = '';	
	$state = 'text';
	$ch1 = '';
	$ch2 = '';
	for($i = 1;$i<=$length;$i++) {
		$c = $bytes[$i];
		if($state == 'text') {
			if($c == 27) { 
				$state = 'esc';
				$result .= chr($c);
				//error_log(PHP_EOL . chr($c), 3, './log.txt');
			} else if ($c >= 129 && $c <= 254) {
				$state = 'chinese';
				$ch1 = $c;
			} else {
				$result .= chr($c);
			}
		} else if($state == 'chinese') {
			$control = '';
			if($c == 27) {
				while(chr($c) != 'm') {
					$control .= chr($c);
					$i++ ;
					$c = $bytes[$i];
				}
				$control .= chr($c);
				$i++;
				$c = $bytes[$i];
			}
			$state = 'text';
			$ch2 = $c;
			$bin = pack('C*', $ch1, $ch2);
			$utf8 = iconv(REMOTE_ENCODING, LOCAL_ENCODING, $bin);
			$result .= $utf8;
		} else if($state == 'esc') {
			if($c == ord('m') || $c == ord('K') || $c == ord('H')) {
				$state = 'text';
			}
			//error_log(chr($c), 3, './log.txt');
			$result .= chr($c);
		}
	}
	return $result;
}

$client = new Telnet('ptt.cc', 23, 10, '');
$data = $client->exec('', false);
echo process_input($data);

$data = $client->exec($config['user']);
echo process_input($data);

$data .= $client->exec($config['password']);
echo process_input($data);


$data = $client->exec('');
echo process_input($data);
sleep(1);

$data = $client->exec('n');
echo process_input($data);
sleep(1);

$data = $client->exec('F');
echo process_input($data);
sleep(1);


$data = $client->exec("8\r");
echo process_input($data);
sleep(1);

//error_log('==========> start here' . PHP_EOL, 3, './log.txt');
$data = $client->exec(" ");
// $bytes = unpack('C*', $buf);
echo process_input($data);
sleep(1);

