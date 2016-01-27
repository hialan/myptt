<?php
class TermChar {
    public $ch;
    public function __construct($char) {
        $this->ch = $char;
    }
    public function isChinese() {
        return (strlen($this->ch) == mb_strlen($this->ch));
    }
}

class TermBuf {
    private $cols, $rows;

    private $cur_x = 0;
    private $cur_y = 0;
    private $cur_x_sav = -1;
    private $cur_y_sav = -1;
    private $scrollStart = 0;
    private $scrollEnd;

    private $lines= array();

    public function __construct($cols, $rows) {
        $this->cols = $cols;
        $this->rows = $rows;
        $this->scrollEnd = $rows - 1;

        for($j=0;$j<$rows;$j++){
            $this->lines[] = $this->getEmptyLine();
        }
    }

    private function getEmptyLine() {
        $line = array();
        for($i=0;$i<$this->cols;$i++) {
            $line[] = new TermChar('');
        }
        return $line;
    }

    public function putc($ch) {
        switch($ch) {
            case "\b":
                $this->back();
                return;
            case "\r":
                $this->carriageReturn();
                return;
            case "\n":
            case "\f":
            case "\v":
                $this->lineFeed();
                return;
            case "\0":
                return;
        }

        if($this->cur_x >= $this->cols) {
            $this->lineFeed();
            $this->cur_x = 0;
        }

        switch($ch) {
            case "\t": 
                $this->tab();
                return;
            default:
                $line = $this->lines[$this->cur_y];
                $line[$this->cur_x]->ch = $ch;
                $isChinese = $line[$this->cur_x]->isChinese();
                $this->cur_x++;
                if($isChinese && $this->cur_x < $this->cols) {
                    $line[$this->cur_x]->ch = '';
                    $this->cur_x++;
                }
                $this->lines[$this->cur_y] = $line;
                break;
        }
    }

    public function gotoPos($x, $y) {
        $x = min($x, $this->cols - 1);
        $x = max(0, $x);

        $y = min($y, $this->rows - 1);
        $y = max(0, $y);

        $this->cur_x = $x;
        $this->cur_y = $y;
    }

    public function deleteLine($param) {
        $scrollStart = $this->scrollStart;
        $this->scrollStart = $this->cur_y;
        $this->scroll(false, $param);
        $this->scrollStart = $scrollStart;
    }

    public function insertLine($param) {
        $scrollStart = $this->scrollStart;
        if($this->cur_y < $this->scrollEnd) {
            $this->scrollStart = $this->cur_y;
            $this->scroll(true, $param);
        }
        $this->scrollStart = $scrollStart;
    }

    public function scroll($up, $n) {
        $scrollStart = $this->scrollStart;
        $scrollEnd = $this->scrollEnd;
        
        if($scrollEnd <= $scrollStart) {
            $scrollStart = 0;
            if($scrollEnd < 1) $scrollEnd = $this->rows - 1;
        }

        if($n >= $this->rows) {
            $this->clear(2);
        } else if($n >= $scrollEnd-$scrollStart+1) {
            for($row=$scrollStart;$row<=$scrollEnd;$row++) {
                $this->lines[$row] = $this->getEmptyLine();
            }
        } else {
            if($up == true) {
                
            } else {
                for($i=0;$i<$scrollStart;$i++) {
                    $this->lines[] = array_shift($this->lines);
                }
                while(--$n >= 0) {
                    array_shift($this->lines);
                    $line =  $this->getEmptyLine();
                    array_splice($this->lines, $scrollEnd - $scrollStart, 0, $line);
                }
                for($i=0;$i<$scrollStart;$i++) {
                    $line = array_pop($this->lines);
                    array_unshift($this->lines, $line);
                }
            }
        }
    }

    public function tab($param = null) {
        $mod = $this->cur_x % 4;
        $this->cur_x += 4 - $mod;
        if(!is_null($param) && $param > 1) {
            $this->cur_x += 4 * ($param - 1);
        }
        if($this->cur_x > $this->cols) {
            $this->cur_x = $this->cols - 1;
        }
    }

    public function back() {
        $this->cur_x = max(0, $this->cur_x - 1);
    }

    public function carriageReturn() {
        $this->cur_x = 0;
    }

    public function lineFeed() {
        if($this->cur_y + 1 < $this->rows) {
            $this->cur_y++;
        } else {
            $this->scroll(false, 1);
        }
    }
}
?>
