<?php
require_once __DIR__ . '/vendor/autoload.php';
require_once __DIR__ . '/src/Client.inc';
require_once __DIR__ . '/bbs/ptt_helper.inc';

use Carbon\Carbon;

$helper = new ptt_helper();


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
$line = $client->getCurrentLine();

// get article numbe in the list
$parsedData = $helper->parse_line($line);

$end = $parsedData['number'];
$start = $end - 10;

$result = $helper->parse_board_article_list($client, $start, $end);

// exit ptt

$client->exec(TermInput::KEY_LEFT . TermInput::KEY_LEFT . TermInput::KEY_LEFT . TermInput::KEY_LEFT, true);
$client->exec('y');
echo $client->getScreen();

$client->disconnect();

/// insert db
$db = new SQLite3('articles.sqlite');

$sql_create =<<<EOT
CREATE TABLE IF NOT EXISTS articles (
    hashid TEXT PRIMARY KEY, 
    board TEXT, 
    date TEXT, 
    author TEXT, 
    title TEXT, 
    url TEXT, 
    push_number TEXT, 
    created_time TEXT, 
    updated_time TEXT
)
EOT;

$db->exec($sql_create);

$stmt_select = $db->prepare('SELECT * FROM articles WHERE hashid = :hashid;');

$new_articles = [];
$update_articles = [];

foreach($result as $article) {
    $hash = $article['hash'];
    $stmt_select->bindValue(':hashid', $hash);
    $result = $stmt_select->execute();
    $row = $result->fetchArray();
    $result->finalize();

    if($row === false && !isset($new_articles[$hash])) {
        $new_articles[$hash] = $article;
    } else if(isset($row['push_number']) && $article['push_number'] == '爆' && $row['push_number'] != $article['push_number']) {
        $update_articles[$hash] = $article;
    }
}

$stmt_select->close();

if( count($new_articles) > 0) {
    $now = Carbon::now();

    $db->exec('BEGIN');
    if(count($new_articles) > 0) {
        $stmt_insert = $db->prepare(
            'INSERT INTO articles (hashid, board, date, author, title, url, push_number, updated_time, created_time) ' . 
            'VALUES (:hashid, :board, :date, :author, :title, :url, :push_number, :updated_time, :created_time)');
        foreach($new_articles as $article) {
            $stmt_insert->clear();
            $stmt_insert->bindValue(':hashid', $article['hash']);
            $stmt_insert->bindValue(':board',  $article['board']);
            $stmt_insert->bindValue(':date',   $article['date']);
            $stmt_insert->bindValue(':author', $article['author']);
            $stmt_insert->bindValue(':title', $article['title']);
            $stmt_insert->bindValue(':url', $article['url']);
            $stmt_insert->bindValue(':push_number', $article['push_number']);
            $stmt_insert->bindValue(':updated_time', $now->toIso8601String());
            $stmt_insert->bindValue(':created_time', $now->toIso8601String());
            $stmt_insert->execute();
        }
        $stmt_insert->close();
    }

    if(count($update_articles) > 0) {
        $stmt_update = $db->prepare(
            'UPDATE articles SET title=:title, url=:url, push_number=:push_number, updated_time=:updated_time ' . 
            'WHERE hashid=:hashid');
        foreach($update_articles as $article) {
            $stmt_update->clear();
            $stmt_update->bindValue(':title', $article['title']);
            $stmt_update->bindValue(':url', $article['url']);
            $stmt_update->bindValue(':push_number', $article['push_number']);
            $stmt_update->bindValue(':updated_time', $now->toIso8601String());
            $stmt_update->bindValue(':hashid', $article['hash']);
            $stmt_update->execute();
        }
        $stmt_update->close();
    }

    $db->exec('COMMIT');
}

echo 'new' . PHP_EOL;
print_r($new_articles);
echo 'update ' . PHP_EOL;
print_r($update_articles);

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

function getSlackPayload($article) {
    $data = [
        'username' => 'Ptt 爆掛 Bot',
        'text' => "`{$article['push_number']}` {$article['date']} {$article['author']} <{$article['url']}|{$article['title']}> ({$article['board']} {$article['hash']})",
    ];
    return $data;
}

if( !empty($config['slack_webhook']) && count($new_articles) > 0) {
    $slack_url = $config['slack_webhook'];

    if(count($new_articles) > 0) {
        foreach($new_articles as $article) {
            $data = getSlackPayload($article);
            $post_data['payload'] = json_encode($data);
            $resp = http_post($slack_url, $post_data);
            echo "Slack send: {$article['title']} ==> " . print_r($resp, true) . "\n";
        }
    }

    if(count($update_articles) > 0) {
        foreach($update_articles as $article) {
            $data = getSlackPayload($article);
            $post_data['payload'] = json_encode($data);
            $resp = http_post($slack_url, $post_data);
            echo "Slack send: {$article['title']} ==> " . print_r($resp, true) . "\n";
        }
    }
}
