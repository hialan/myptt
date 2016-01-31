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


$client->exec('Z80');
echo $client->getScreen();

// find latest post
$client->exec(TermInput::KEY_END, false);
$screen = $client->getScreen();

$lines = explode(PHP_EOL, $screen);
$pos = $client->getCurrentPos();

// get article numbe in the list
$parsedData = parse_line($lines[$pos['cur_y']]);

$end = $parsedData['number'];
$start = $end - 10;

echo "fetch article from {$start} to {$end}\n";

$result = [];

for($i = $start; $i <= $end; $i++) {
    $client->exec(strval($i), true);

    $screen = $client->getScreen();
    $lines = explode(PHP_EOL, $screen);
    $pos = $client->getCurrentPos();
    $line = $lines[$pos['cur_y']];
    $parsedData = parse_line($line);

    if($parsedData['author'] === '-' || empty($parsedData['board'])) {
        continue;
    }

    $client->exec('Q', false);
    $screen = $client->getScreen();
    $article_info = parse_article_url($screen);

    $merged = array_merge($parsedData, $article_info);

    echo $line . PHP_EOL;
    print_r($merged);

    $result[] = $merged;
//    sleep(1);
    
    // exit detail status
    $client->exec(' ', false);
}

// exit ptt

$client->exec(TermInput::KEY_LEFT . TermInput::KEY_LEFT . TermInput::KEY_LEFT . TermInput::KEY_LEFT, true);
$client->exec('y');
echo $client->getScreen();

$client->disconnect();

/// insert db
$db = new SQLite3('articles.sqlite');
$db->exec('CREATE TABLE IF NOT EXISTS articles (hashid TEXT PRIMARY KEY, board TEXT, date TEXT, author TEXT, title TEXT, url TEXT)');

$stmt_select = $db->prepare('SELECT * FROM articles WHERE hashid = :hashid;');

$new_articles = [];
foreach($result as $article) {
    $hash = $article['hash'];
    $stmt_select->bindValue(':hashid', $hash);
    $result = $stmt_select->execute();
    $row = $result->fetchArray();
    $result->finalize();

    if($row === false && !isset($new_articles[$hash])) {
        $new_articles[$hash] = $article;
    }

}

$stmt_select->close();

if( count($new_articles) > 0) {
    $stmt_insert = $db->prepare('INSERT INTO articles (hashid, board, date, author, title, url) VALUES (:hashid, :board, :date, :author, :title, :url)');

    $db->exec('BEGIN');
    foreach($new_articles as $article) {
        $stmt_insert->clear();
        $stmt_insert->bindValue(':hashid', $article['hash']);
        $stmt_insert->bindValue(':board',  $article['board']);
        $stmt_insert->bindValue(':date',   $article['date']);
        $stmt_insert->bindValue(':author', $article['author']);
        $stmt_insert->bindValue(':title', $article['title']);
        $stmt_insert->bindValue(':url', $article['title']);

        $stmt_insert->execute();
    }
    $db->exec('COMMIT');
    $stmt_insert->close();
}

///// Slack
function http_post($url, $data) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_HEADER, 0);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
    $out = curl_exec($ch);
    curl_close($ch);
    return $out;
}

if( count($new_articles) > 0) {
    $slack_url = $config['slack_webhook'];
    foreach($new_articles as $article) {
        $data = [
            'username' => 'Ptt 爆掛 Bot',
            'text' => "`{$article['push_number']}` {$article['date']} {$article['author']} <{$article['url']}|{$article['title']}> ({$article['board']} {$article['hash']})",
        ];
        $post_data['payload'] = json_encode($data);
        $resp = http_post($slack_url, $post_data);
        echo "Slack send: {$article['title']}\n";
        print_r($resp);
    }
}
