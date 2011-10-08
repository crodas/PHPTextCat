--TEST--
getInfo()
--FILE--
<?php 
$tc = new TextCat();
$info = $tc->getInfo();
var_dump(is_array($info));
var_dump(count($info) > 0);
?>
--EXPECT--
bool(true)
bool(true)
