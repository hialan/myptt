<?php
require_once __DIR__ . '/../core/Term.inc';

class ptt_helper {
    private $client;

    public function __construct($client) {
        $this->client = $client;
    }

    public function login($user, $password) {
        $this->client->exec('', false);
        echo $this->client->getScreen();

        $this->client->exec($user);
        $this->client->exec($password);

        echo $this->client->getScreen();

        return ;
    }

    private function my_substr($line, &$i, $count) {
        $str = mb_substr($line, $i, $count);
        $i += $count;
        return $str;
    }

    public function parse_line($line) {
        $i = 1;
        $number = $this->my_substr($line, $i, 6);
        $i += 2; // skip
        $push_number = $this->my_substr($line, $i, 1);
        if($push_number !== '爆') {
            $push_number .= $this->my_substr($line, $i, 1);
        }
        $date = $this->my_substr($line, $i , 5);
        $i += 1; // skip
        $author = $this->my_substr($line, $i , 13);
        $title = mb_substr($line, $i);

        return [
            "number"      => trim($number),
            "push_number" => trim($push_number),
            "date"        => trim($date),
            "author"      => trim($author),
            "title"       => trim($title),
        ];
    }

    public function parse_article_url($screen) {
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

    public function parse_board_article_list($start, $end) {
        echo "fetch article from {$start} to {$end}\n";

        $result = [];
        for($i = $end; $i >= $start; $i--) {
            $this->client->exec(strval($i), true);

            $line = $this->client->getTerm()->getCurrentLine();
            $parsedData = $this->parse_line($line);

            if($parsedData['author'] === '-' || empty($parsedData['title'])) {
                continue;
            }

            $this->client->exec('Q', false);
            $screen = $this->client->getScreen();
            $article_info = $this->parse_article_url($screen);

            $merged = array_merge($parsedData, $article_info);

            echo $line . PHP_EOL;
            print_r($merged);

            $result[] = $merged;
            
            // exit detail status
            $this->client->exec(' ', false);
        }
        return $result;
    }

    public function parse_board_by_push_count($board, $min_push_count, $get_count = 10) {
        $this->enter_board($board);
        echo $this->client->getScreen();

        $this->client->exec('Z' . $min_push_count);
        echo $this->client->getScreen();

        // find latest post
        $this->client->exec(TermInput::KEY_END, false);
        $line = $this->client->getTerm()->getCurrentLine();

        // get article numbe in the list
        $parsedData = $this->parse_line($line);

        $end = $parsedData['number'];
        $start = $end - $get_count;

        return $this->parse_board_article_list($start, $end);
    }

    public function is_anykey_continue() {
        $line = $this->client->getTerm()->getLastLine();
        if(mb_strpos($line, '請按任意鍵繼續')) {
            return true;
        }
        return false;
    }

    public function goto_main_menu() {
        while(mb_substr($this->client->getTerm()->getLine(0), 1, 4) != '主功能表' || $this->is_anykey_continue()) {
            $this->client->exec(TermInput::KEY_LEFT, false);
        }
    }

    public function enter_board($board) {
        $this->goto_main_menu();

        $this->client->exec('s' . $board, true);
        if($this->is_anykey_continue()) {
            $this->client->exec(' ', false);
        }
    }

    public function exitbbs() {
        $this->goto_main_menu();
        $this->client->exec('g', true);
        $this->client->exec('y', true);
    }
}
?>
