--TEST--
Testing Exception
--FILE--
<?php 
error_reporting(E_ALL);
$tc = new TextCat();
try {
$result = $tc->save("spanish");
} catch (Exception $e) {
    die('done');
}
die('failed');
?>
--EXPECT--
done
