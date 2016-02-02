# BBS parser


## Dependency

* php > 5.6
* [composer](https://getcomposer.org/)

## Installation

```
$ composer install
```

## Usage

prepare config.ini


```
[global]
user = "login-user-name"
password = "login-password"

[slack]
; slack webhook
; https://slack.com/apps/A0F7XDUAZ-incoming-webhooks
test-channel="your-slack-webhook1"
test-channel-2="your-slack-webhook2"
channel-name="your-slack-webhook3"


; task # from 1 ~ n
[task.1]
board=Gossiping
min_push_count = 80
get_count = 3
slack=test-channel

[task.2]
board=Tech_Job
min_push_count = 10
get_count = 3
slack=test-channel
```

and run


```
$ php ptt.php
```

## Acknowledgement

* Term.inc & ansi_parser.inc is convert from PttChrome project: https://github.com/iamchucky/PttChrome
* Telnet.class.inc is modified from https://github.com/ngharo/Random-PHP-Classes/blob/master/Telnet.class.php
