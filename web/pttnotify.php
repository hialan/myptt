<?php
$dbfile = '/home/hialan_liu/myptt/articles.sqlite';
$db = new SQLite3($dbfile);
$result = $db->query('SELECT * from articles ORDER BY updated_time DESC limit 40');
$rows = [];
while($row = $result->fetchArray()) {
        $rows[] = $row;
}
$result->finalize();
$db->close();
?>
<center>
<table border=1>
<?php
foreach($rows as $row) {
        echo <<<EOT
<tr>
        <td>{$row['board']}</td>
        <td>{$row['push_number']}</td>
        <td>{$row['date']}</td>
        <td>{$row['author']}</td>
        <td><a href="{$row['url']}">{$row['title']}</a></td>
</tr>
EOT;
}
?>
</table>
</center>
