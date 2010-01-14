--TEST--
Testing Exception
--FILE--
<?php 
error_reporting(E_ALL);
$tc = new TextCat();
$tc->setDirectory('/tmp');
$tc->addSample("Buenos dias, esto es un ejemplo de texto");
$tc->save("spanish");
var_dump($tc->getCategory('Esto es un lindo texto'));
?>
--EXPECT--
string(7) "spanish"
