<?php
class ptt_helper {
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

    public function parse_board_article_list($client, $start, $end) {
        echo "fetch article from {$start} to {$end}\n";

        $result = [];
        for($i = $end; $i >= $start; $i--) {
            $client->exec(strval($i), true);

            $line = $client->getCurrentLine();
            $parsedData = $this->parse_line($line);

            if($parsedData['author'] === '-' || empty($parsedData['title'])) {
                continue;
            }

            $client->exec('Q', false);
            $screen = $client->getScreen();
            $article_info = $this->parse_article_url($screen);

            $merged = array_merge($parsedData, $article_info);

            echo $line . PHP_EOL;
            print_r($merged);

            $result[] = $merged;
            
            // exit detail status
            $client->exec(' ', false);
        }
        return $result;
    }
}
?>
