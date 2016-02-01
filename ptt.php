<?php
require_once __DIR__ . '/vendor/autoload.php';
require_once __DIR__ . '/src/core/Client.inc';
require_once __DIR__ . '/src/ptt/ptt_helper.inc';
require_once __DIR__ . '/src/notifier/notify_slack.inc';
require_once __DIR__ . '/src/storage/storage_sqlite.inc';

function load_config() {
    $config = [];
    $ini = parse_ini_file("config.ini", true);
    $config['global'] = $ini['global'];

    $boards = explode(',', $ini['global']['boards']);
    foreach($boards as $board) {
        $config['boards'][$board] = $ini['board.' . $board];
    }
    return $config;
}

$config = load_config();

$client = new Client('ptt.cc', 23);
$helper = new ptt_helper($client);
$db = new storage_sqlite();

// do login
$helper->login($config['global']['user'], $config['global']['password']);

// go to main
$client->exec('');
echo $client->getScreen();
sleep(1);

//// start parse board

foreach($config['boards'] as $board => $setting) {
    $min_push_count = $setting['min_push_count'];
    $get_count = $setting['get_count'];
    $slack_url = $setting['slack_webhook'];

    echo "\n===> Process board {$board} min_push_count({$min_push_count}) get_count({$get_count})\n\n";

    $result = $helper->parse_board_by_push_count($board, $min_push_count, $get_count);
    list($new_articles, $update_articles) = $db->process_result($result);

    echo 'new' . PHP_EOL;
    print_r($new_articles);
    echo 'update ' . PHP_EOL;
    print_r($update_articles);

    ///// Slack
    if(!empty($slack_url)) {
        $slack = new notify_slack($slack_url);
        $articles = array_merge($new_articles, $update_articles);
        if(count($articles) > 0) {
            $slack->notify($articles);
        }
    }
}

//// exit bbs
$helper->exitbbs();
echo $client->getScreen();

$client->disconnect();

/// slack push done
if(!empty($config['global']['slack_webhook'])) {
    $slack = new notify_slack($config['global']['slack_webhook']);
    $msg = sprintf("[%s] done", date("D M j G:i:s T Y"));
    $slack->send($msg);
}
