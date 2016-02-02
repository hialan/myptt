<?php

class notify_slack {
    private $slack_webhook_url = '';
    public function __construct($slack_webhook_uri) {
        $this->slack_webhook_url = $slack_webhook_uri;
    }

    ///// Slack
    private function http_post($url, $data) {
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

    private function getSlackPayload($article) {
        $article['push_number'] = trim($article['push_number']);
        if(empty($article['push_number'])) {
            $article['push_number'] = 0;
        }
        $data = [
            'username' => 'Ptt 爆掛 Bot',
            'text' => "`{$article['push_number']}` {$article['date']} {$article['author']} <{$article['url']}|{$article['title']}> ({$article['board']} {$article['hash']})",
        ];
        return $data;
    }

    public function notify(array $articles) {
        foreach($articles as $article) {
            $data = $this->getSlackPayload($article);
            $post_data['payload'] = json_encode($data);
            $resp = $this->http_post($this->slack_webhook_url, $post_data);
            echo "Slack send: {$article['title']} ==> " . print_r($resp, true) . "\n";
        }
    }

    public function send($msg) {
        $data = [
            'username' => 'Ptt 爆掛 Bot',
            'text' => $msg
        ];
        $post_data['payload'] = json_encode($data);
        $resp = $this->http_post($this->slack_webhook_url, $post_data);
    }
}

?>
