<?php
require_once __DIR__ . '/Client.inc';

$config = parse_ini_file("config.ini", true);
$config = $config['global'];


$client = new Client('ptt.cc', 23);
$client->exec('', false);
echo $client->getScreen();

$client->exec($config['user']);
echo $client->getScreen();

$client->exec($config['password']);
echo $client->getScreen();

$client->exec('');
echo $client->getScreen();
sleep(1);

$client->exec('sGossiping');
$screen = $client->getScreen();
echo $screen;
sleep(1);


if(mb_strpos($screen, '請按任意鍵繼續')) {
	$client->exec('');
	$screen = $client->getScreen();
	echo $screen;
	sleep(1);
}


$client->exec('Z100');
echo $client->getScreen();
sleep(1);

$client->exec('Q', false);
echo $client->getScreen();
sleep(1);

$client->disconnect();
