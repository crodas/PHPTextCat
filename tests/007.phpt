--TEST--
Testing set directory
--FILE--
<?php 
error_reporting(E_ALL);
$tc = new TextCat();
var_dump($tc->setDirectory(getcwd()));
?>
--EXPECT--
bool(true)
