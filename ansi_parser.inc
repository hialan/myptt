<?php
class ansi_parser {
    const STATE_TEXT = 0;
    const STATE_ESC = 1;
    const STATE_CSI = 2;
    const STATE_C1 = 3;

    private $term;
    private $state = self::STATE_TEXT;
    private $esc = '';
    public function __construct($term) {
        $this->term = $term;
    }

    public function feed($data) {
        $s = '';
        $n = strlen($data);
        for($i=0;$i<$n;$i++) {
            $ch = $data[$i];
            switch($this->state) {
                case self::STATE_TEXT:
                    switch($ch) {
                        case "\x1b":
                            if(!empty($s)) {
                                $this->term->puts($s);
                                $s = '';
                            }
                            $this->state = self::STATE_ESC;
                            break;
                        default:
                            $s .= $ch;
                    }
                    break;
                case self::STATE_CSI:
                    if( ($ch >= '`' && $ch <= 'z') || ($ch >= '@' && $ch <='Z') ) {
                        $params = explode(';', $this->esc);
                        $firstChar = '';
                        if (!empty($params[0])) {
                            if ($params[0][0] < '0' || $params[0][0] > '9') {
                                $firstChar = $params[0][0];
                                $params[0] = substr($params[0], 1);
                            }
                        }
                        if(!empty($firstChar) && $ch != 'h' && $ch != 'l') {
                            $this->state = self::STATE_TEXT;
                            $this->esc = '';
                            break;
                        }
                        for($j=0;$j<count($params);$j++) {
                            if(!empty($params[$j])){
                                $params[$j] = intval($params[$j]);
                            } else {
                                $params[$j] = 0;
                            }
                        }
                        switch($ch) {
                            case 'm':
                                break;
                            case '@':
                                $this->term->insert($params[0]>0 ? $params[0] :1);
                                break;
                            case 'A':
                                $this->term->gotoPos($this->term->cur_x, $this->term->cur_y-(empty($params[0])?$params[0]:1));
                                break;
                            case 'B':
                            case 'e':
                                $this->term->gotoPos($this->term->cur_x, $this->term->cur_y+(empty($params[0])?$params[0]:1));
                                break;
                            case 'C':
                            case 'e':
                                $this->term->gotoPos($this->term->cur_x+(empty($params[0])?$params[0]:1), $this->term->cur_y);
                                break;
                            case 'D':
                                $this->term->gotoPos($this->term->cur_x-(empty($params[0])?$params[0]:1), $this->term->cur_y);
                                break;
                            case 'E':
                                $this->term->gotoPos(0, $this->term->cur_y+(empty($params[0])?$params[0]:1));
                                break;
                            case 'F':
                                $this->term->gotoPos(0, $this->term->cur_y-(empty($params[0])?$params[0]:1));
                                break;
                            case 'G':
                            case '`':
                                $this->term->gotoPos($params[0]>0?$params[0]-1:0 , $this->term->cur_y);
                                break;
                            case 'I':
                                $this->term->tab($params[0]?$params[0]:1);
                                break;
                            case 'd':
                                $this->term->gotoPos($this->term->cur_x, $params[0]>0?$params[0]-1:0);
                                break;
                            case 'J':
                                $this->term->clear(!empty($params[0])?$params[0]-1:0);
                                break;
                            case 'H':
                            case 'f':
                                if(count($params) < 2) {
                                    $this->term->gotoPos(0, 0);
                                } else {
                                    if($params[0] > 0) {
                                        $params[0]--;
                                    }
                                    if($params[1] > 0) {
                                        $params[1]--;
                                    }
                                    $this->term->gotoPos($params[1], $params[0]);
                                }
                                break;
                            case 'K':
                                $this->term->eraseLine(!empty($params[0])?$params[0]:0);
                                break;
                            case 'L':
                                $this->term->insertLine($params[0] > 0?$params[0]: 1);
                                break;
                            case 'M':
                                $this->term->deleteLine($params[0] > 0?$params[0]: 1);
                                break;
                            case 'P':
                                $this->term->del($params[0] > 0?$params[0]: 1);
                                break;
                            case 'r': // scroll range
                                if(count($params) < 2) {
                                    $this->term->scrollStart = 0;
                                    $this->term->scrollEnd = $this->term->rows - 1;
                                } else {
                                    if($params[0] > 0) {
                                        $params[0]--;
                                    }
                                    if($params[1] > 0) {
                                        $params[1]--;
                                    }
                                    $this->term->scrollStart = $params[0];
                                    $this->term->scrollEnd = $params[1];
                                }
                                break;
                            case 's':
                                $this->term->cur_x_sav = $this->term->cur_x;
                                $this->term->cur_y_sav = $this->term->cur_y;
                                break;
                            case 'u':
                                if($this->term->cur_x_sav < 0 || $this->term->cur_y_sav < 0) break;
                                $this->cur_x = $this->cur_x_sav;
                                $this->cur_y = $this->cur_y_sav;
                                break;
                            case 'S':
                                $this->term->scroll(false, ($params[0] > 0?$params[0]: 1));
                                break;
                            case 'T':
                                $this->term->scroll(true, ($params[0] > 0?$params[0]: 1));
                                break;
                            case 'X':
                                $this->eraseChar($params[0] > 0?$params[0]: 1);
                                break;
                            case 'Z':
                                $this->backTab($params[0] > 0?$params[0]: 1);
                                break;
                            default:
                        }
                        $this->state = self::STATE_TEXT;
                        $this->esc = '';
                    } else {
                        $this->esc .= $ch;
                    }
                    break;
                case self::STATE_C1:
                    $this->esc = '';
                    $this->state = self::STATE_TEXT;
                    break;
                case self::STATE_ESC:
                    if($ch == '[') {
                        $this->state = self::STATE_CSI;
                    } else {
                        $this->state = self::STATE_C1;
                        --$i;
                    }
                    break;
            }
        } // for
        if(!empty($s)) {
            $this->term->puts($s);
            $s = '';
        }

        return;
    }
}
?>
