<?php
class TermInput {
    const KEY_UP="\x1b[A";
    const KEY_DOWN="\x1b[B";
    const KEY_LEFT="\x1b[D";
    const KEY_RIGHT="\x1b[C";

    const KEY_HOME="\x1b[1~";
    const KEY_END="\x1b[4~";
}

class TermChar {
    public $ch;
    public function __construct($char) {
        $this->ch = $char;
    }
    public function isChinese() {
        return (strlen($this->ch) == mb_strlen($this->ch));
    }

    public function copyFrom(TermChar $termChar) {
        $this->ch = $termChar->ch;
    }
}

class TermBuf {
    public $cols, $rows;

    public $cur_x = 0;
    public $cur_y = 0;
    public $cur_x_sav = -1;
    public $cur_y_sav = -1;
    public $scrollStart = 0;
    public $scrollEnd;

    private $lines= array();
    private $newChar;

    public function __construct($cols, $rows) {
        $this->cols = $cols;
        $this->rows = $rows;
        $this->scrollEnd = $rows - 1;
        $this->newChar = new TermChar(' ');

        for($j=0;$j<$rows;$j++){
            $this->lines[] = $this->getEmptyLine();
        }
    }

    public function getLine($row) {
        $line = '';
        for($i=0;$i<$this->cols;$i++) {
            $line .= $this->lines[$row][$i]->ch;
        }
        $line = iconv('BIG5', 'UTF-8', $line);
        return $line;
    }

    public function show() {
        $screen = '';
        for($i=0;$i<$this->rows;$i++) {
           $line = $this->getLine($i);
           $screen .= $line.PHP_EOL;
        }
        return $screen;
    }

    public function getCurrentLine() {
        return $this->getLine($this->cur_y);
    }

    public function getLastLine() {
        return $this->getLine($this->rows - 1);
    }

    private function getEmptyLine() {
        $line = array();
        for($i=0;$i<$this->cols;$i++) {
            $line[] = new TermChar(' ');
        }
        return $line;
    }

    public function puts($data) {
        $n = strlen($data);
        for($i = 0;$i < $n ;$i++) {
            $ch = $data[$i];
            $this->putc($ch);
        }
    }

    public function putc($ch) {
        switch($ch) {
            case "\x07":
                // FIXME: beep (1)Sound (2)AlertNotification (3)change icon
                error_log('[BEEP!]');
                continue;
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
                //echo "[{$this->cur_y}, {$this->cur_x}]\n";
                $line =& $this->lines[$this->cur_y];
                $line[$this->cur_x]->ch = $ch;
                $this->cur_x++;
                /*
                $isChinese = $line[$this->cur_x]->isChinese();
                if($isChinese && $this->cur_x < $this->cols) {
                    $line[$this->cur_x]->ch = '';
                    $this->cur_x++;
                }
                */
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

    public function eraseLine($param) {
        $line =& $this->lines[$this->cur_y];
        switch($param) {
            case 0: // erase to right
                for($col = $this->cur_x; $col < $this->cols; $col++) {
                    $line[$col]->copyFrom($this->newChar);
                }
                break;
            case 1: // erase to left
                for($col = 0; $col < $this->cur_x; $col++) {
                    $line[$col]->copyFrom($this->newChar);
                }
                break;
            case 2: // erase all
                for($col = 0;$col < $this->cols; $col++) {
                    $line[$col]->copyFrom($this->newChar);
                }
                break;
            default:
                break;
        }
        return;
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
            if($up == true) {  // move lines down
                for($i=0;$i < $this->rows-1-scrollEnd;$i++) {
                    $line = array_pop($this->lines);
                    array_unshift($this->lines, $line);
                }
                while( ($n - 1) >= 0) {
                    $n--;
                    $line = array_pop($this->lines);
                    $line = $this->getEmptyLine();
                    array_splice($this->lines, $this->rows - 1 - $scrollEnd + $scrollStart, 0, $line);
                }
                for($i=0;$i<$this->rows-1-$scrollEnd; $i++) {
                    $line = array_shift($this->lines);
                    $this->lines[] = $line;
                }
            } else { // move lines up
                for($i=0;$i<$scrollStart;$i++) {
                    $this->lines[] = array_shift($this->lines);
                }
                while(--$n >= 0) {
                    array_shift($this->lines);
                    $line = $this->getEmptyLine();
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

    public function clear($param) {
        switch($param) {
            case 0:
                $line =& $this->lines[$this->cur_y];
                for($col = $this->cur_x; $col < $this->cols; $col++) {
                    $line[$col]->copyFrom($this->newChar);
                }
                for($row = $this->cur_y + 1; $row < $this->rows; $row++) {
                    $line =& $this->lines[$row];
                    for($col = 0;$col < $this->cols; $col++) {
                        $line[$col]->copyFrom($this->newChar);
                    }
                }
                break;
            case 1:
                for($row = 0;$row < $this->cur_y;$row++) {
                    $line =& $this->lines[$row];
                    for($col = 0;$col < $this->cols;$col++) {
                        $line[$col].copyFrom($this->newChar);
                    }
                }
                $line =& $this->lines[$this->cur_y];
                for($col=0;$col<$this->cur_x;$col++) {
                    $line[$col].copyFrom($this->newChar);
                }
                break;
            case 2:
                for($row = 0;$row < $this->rows;$row++) {
                    $line =& $this->lines[$row];
                    for($col = 0;$col < $this->cols;$col++) {
                        $line[$col].copyFrom($this->newChar);
                    }
                }
                break;
        }
        $this->gotoPos(0, 0);
    }
}
?>
