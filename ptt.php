<?php
require_once __DIR__ . '/src/Client.inc';

function my_substr($line, &$i, $count) {
    $str = mb_substr($line, $i, $count);
    $i += $count;
    return $str;
}

function parse_line($line) {
    $i = 1;
    $number = my_substr($line, $i, 6);
    $i += 2; // skip
    $push_number = my_substr($line, $i, 1);
    if($push_number !== '爆') {
        $push_number .= my_substr($line, $i, 1);
    }
    $date = my_substr($line, $i , 5);
    $i += 1; // skip
    $author = my_substr($line, $i , 13);
    $title = mb_substr($line, $i);

    return [
        "number"      => trim($number),
        "push_number" => trim($push_number),
        "date"        => trim($date),
        "author"      => trim($author),
        "title"       => trim($title),
    ];
}

function parse_article_url($screen) {
    $lines = explode(PHP_EOL, $screen);
    $hash = '';
    $board = '';
    $url = '';

    foreach($lines as $line) {
        $begin_part = mb_substr($line, 0, 12);

        if(mb_strpos($begin_part, "文章代碼(AID):") !== false) {
            $hash = mb_substr($line, 12, 10);
            $after = trim(mb_substr($line, 22));
            $board = mb_substr($after, 1, mb_strpos($after, ')') - 1);
            continue;
        }
        if(mb_strpos($begin_part, " 文章網址: ") !== false) {
            $url_pattern = '#\bhttps?://[^\s()<>]+(?:\([\w\d]+\)|([^[:punct:]\s]|/))#';
            preg_match($url_pattern, $line, $match);
            $url = $match;
            break;
        }
    }

    return [
        'hash'  => trim($hash),
        'board' => trim($board),
        'url'   => $url[0],
    ];
}

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
	echo $client->getScreen();
	sleep(1);
}


$client->exec('Z100');
echo $client->getScreen();

// find latest post
$client->exec(TermInput::KEY_END, false);
$screen = $client->getScreen();

$lines = explode(PHP_EOL, $screen);
$pos = $client->getCurrentPos();

// get article numbe in the list
$parsedData = parse_line($lines[$pos['cur_y']]);

$end = $parsedData['number'];
$start = $end - 20;

echo "fetch article from {$start} to {$end}\n";

$result = [];

for($i = $start; $i <= $end; $i++) {
    $client->exec(strval($i), true);

    $screen = $client->getScreen();
    $lines = explode(PHP_EOL, $screen);
    $pos = $client->getCurrentPos();
    $line = $lines[$pos['cur_y']];
    $parsedData = parse_line($line);

    if($parsedData['author'] === '-') {
        continue;
    }

    $client->exec('Q', false);
    $screen = $client->getScreen();
    $article_info = parse_article_url($screen);

    $merged = array_merge($parsedData, $article_info);

    echo $line . PHP_EOL;
    print_r($merged);

    $result[] = $merged;
    sleep(1);
    
    // exit detail status
    $client->exec(' ', false);
}

$client->disconnect();

print_r($result);

