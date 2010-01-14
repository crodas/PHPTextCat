--TEST--
Testing parse() method
--FILE--
<?php 
error_reporting(E_ALL);
$tc = new TextCat();
$ngrams = $tc->parse("Hello everybody, this is a simple text");
var_dump(is_array($ngrams));
?>
--EXPECT--
bool(true)
