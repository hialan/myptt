<?php
require_once __DIR__ . '/../../vendor/autoload.php';

use Carbon\Carbon;

class storage_sqlite {
    private $db;

    public function __construct() {
        $this->db = new SQLite3('articles.sqlite');
        $this->init_db();
    }

    private function init_db() {
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

        $this->db->exec($sql_create);
    }

    public function begin() {
        $this->db->exec('BEGIN');
    }

    public function commit() {
        $this->db->exec('COMMIT');
    }

    public function rollback() {
        $this->db->exec('ROLLBACK');
    }

    public function get_by_hashids(array $hashids) {
        $stmt_select = $this->db->prepare('SELECT * FROM articles WHERE hashid = :hashid;');
        $rows = [];
        foreach($hashids as $hashid) {
            $stmt_select->bindValue(':hashid', $hashid);
            $result = $stmt_select->execute();
            $row = $result->fetchArray();
            $result->finalize();

            if($row !== false) {
                $rows[$hashid] = $row;
            }
        }
        $stmt_select->close();
        return $rows;
    }

    public function insert_articles(array $articles) {
        $now = Carbon::now();

        if(count($articles) > 0) {
            $stmt_insert = $this->db->prepare(
                'INSERT INTO articles (hashid, board, date, author, title, url, push_number, updated_time, created_time) ' . 
                'VALUES (:hashid, :board, :date, :author, :title, :url, :push_number, :updated_time, :created_time)');
            foreach($articles as $article) {
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
    }

    public function update_articles(array $articles) {
        $now = Carbon::now();

        if(count($articles) > 0) {
            $stmt_update = $this->db->prepare(
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
    }

    public function process_result($result) {
        $hashids = array_map(function($article) {
            return $article['hash'];
        }, $result);

        $db_result = $this->get_by_hashids($hashids);

        $new_articles = [];
        $update_articles = [];

        foreach($result as $article) {
            $hash = $article['hash'];

            $row = isset($db_result[$hash]) ? $db_result[$hash] : false;

            if($row === false && !isset($new_articles[$hash])) {
                $new_articles[$hash] = $article;
            } else if(isset($row['push_number']) && $article['push_number'] == '爆' && $row['push_number'] != $article['push_number']) {
                $update_articles[$hash] = $article;
            }
        }

        if(count($new_articles) > 0 || count($update_articles) > 0) {
            $this->begin();
            $this->insert_articles($new_articles);
            $this->update_articles($update_articles);
            $this->commit();
        }

        return [$new_articles, $update_articles];
    }
}

