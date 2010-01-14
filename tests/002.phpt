--TEST--
getInfo()
--FILE--
<?php 
$tc = new TextCat();
$info = $tc->getInfo();
var_dump(is_array($info));
?>
--EXPECT--
bool(true)
